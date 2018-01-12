/*
   Drawpile - a collaborative drawing program.

   Copyright (C) 2018 Calle Laakkonen

   Drawpile is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Drawpile is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Drawpile.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "renderer.h"

#include "../client/canvas/statetracker.h"
#include "../client/canvas/layerlist.h"
#include "../client/canvas/aclfilter.h"
#include "../client/core/layerstack.h"
#include "../client/ora/orawriter.h"
#include "../shared/record/reader.h"

#include <QImageWriter>
#include <QElapsedTimer>

bool saveImage(const DrawpileCmdSettings &settings, const paintcore::LayerStack &layers, int index)
{
	QString filename = settings.outputFilePattern;

	// Perform pattern subsitutions:
	// :idx: <-- image index number
	filename.replace(":idx:", QString::number(index));

	if(settings.verbose)
		fprintf(stderr, "[I] Writing %s...\n", qPrintable(filename));

	QString error;
	bool ok;
	if(filename.endsWith(".ora", Qt::CaseInsensitive)) {
		// Special case: Save as OpenRaster with all the layers intact
		ok = openraster::saveOpenRaster(filename, &layers, &error);

	} else {
		QImageWriter writer(filename);
		ok = writer.write(layers.toFlatImage(settings.mergeAnnotations));
		if(!ok)
			error = writer.errorString();
	}

	if(!ok)
		fprintf(stderr, "[E] %s: %s\n", qPrintable(filename), qPrintable(error));

	return ok;
}

QString prettyDuration(qint64 duration)
{
	const double msecs = duration / 1000000;
	if(msecs < 1000)
		return QStringLiteral("%1 ms").arg(msecs, 0, 'f', 1);
	const double secs = msecs / 1000;
	if(secs<60)
		return QStringLiteral("%1 s").arg(secs, 0, 'f', 2);
	return QStringLiteral("%1 m %2s").arg(secs/60, 0, 'f', 0).arg(secs - (60*int(secs)), 0, 'f', 2);
}

bool renderDrawpileRecording(const DrawpileCmdSettings &settings)
{
	// Open recording file
	recording::Reader reader(settings.inputFilename);
	recording::Compatibility compat = reader.open();

	if(compat == recording::CANNOT_READ) {
		fprintf(stderr, "[E] %s", qPrintable(reader.errorString()));
		return false;
	}

	if(compat != recording::COMPATIBLE && compat != recording::MINOR_INCOMPATIBILITY) {
		fprintf(stderr, "[E] Recording not compatible\n");
		return false;
	}

	// Initialize the paint engine
	paintcore::LayerStack image;
	canvas::LayerListModel layermodel;
	canvas::StateTracker statetracker(&image, &layermodel, 1);
	canvas::AclFilter aclfilter;

	aclfilter.reset(1, false);

	// Benchmarking
	QElapsedTimer renderTime;
	qint64 totalRenderTime = 0;

	// Read and execute commands
	int imageIndex = 1;
	int exportCounter = 0;
	recording::MessageRecord record;
	do {
		const qint64 offset = reader.filePosition();
		record = reader.readNext();

		if(record.status == recording::MessageRecord::OK) {
			protocol::MessagePtr msg(record.message);

			if(settings.acl && !aclfilter.filterMessage(*msg)) {
				if(settings.verbose)
					fprintf(stderr, "[A] Filtered message %s from %d (idx %d @ %llx)",
						qPrintable(msg->messageName()),
						msg->contextId(),
						reader.currentIndex(),
						offset
						);
			}

			if(msg->isCommand()) {
				renderTime.start();
				statetracker.receiveCommand(msg);
				totalRenderTime += renderTime.nsecsElapsed();
			}

			// Save images
			if(settings.exportEveryN > 0) {
				switch(settings.exportEveryMode) {
				case ExportEvery::Message:
					++exportCounter;
					break;
				case ExportEvery::Sequence:
					if(msg->type() == protocol::MSG_UNDOPOINT)
						++exportCounter;
					break;
				}

				if(exportCounter >= settings.exportEveryN) {
					exportCounter = 0;
					if(!saveImage(settings, image, imageIndex++))
						return false;
				}
			}

		} else if(record.status == recording::MessageRecord::INVALID) {
			fprintf(stderr, "[E] Invalid message type %d at index %d, offset 0x%llx",
					record.error.type,
					reader.currentIndex(),
					offset
				   );
			return false;
		}
	} while(record.status != recording::MessageRecord::END_OF_RECORDING);

	fprintf(stderr, "[I] Total render time: %s\n", qPrintable(prettyDuration(totalRenderTime)));

	// Save the final result
	if(!saveImage(settings, image, imageIndex++))
		return false;

	return true;
}

