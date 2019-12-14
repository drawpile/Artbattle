/*
   Drawpile - a collaborative drawing program.

   Copyright (C) 2019 Calle Laakkonen

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

#include "handicapdialog.h"
#include "../../libshared/net/meta2.h"

#include "ui_handicaps.h"

#include <QJsonObject>
#include <QTimer>

namespace handicaps {

HandicapDialog::HandicapDialog(HandicapState *state, QWidget *parent)
	: QDialog(parent),
	  m_ui(new Ui_HandicapDialog),
	  m_state(state)
{
	m_ui->setupUi(this);

	m_countdown = new QTimer(this);
	m_countdown->setSingleShot(false);
	m_countdown->setInterval(1000);
	m_countdown->start();
	connect(m_countdown, &QTimer::timeout, this, &HandicapDialog::countdown);

	connect(m_ui->blackoutStart, &QPushButton::clicked, this, &HandicapDialog::startBlackout);
	connect(m_ui->blackoutStop, &QPushButton::clicked, this, [this]() { startHandicap("blackout", -1, QJsonObject()); });

	connect(m_ui->canvasInvertStart, &QPushButton::clicked, this, &HandicapDialog::startCanvasInvert);
	connect(m_ui->canvasInvertStop, &QPushButton::clicked, this, [this]() { startHandicap("canvasInvert", -1, QJsonObject()); });

	connect(m_ui->hideCursorStart, &QPushButton::clicked, this, &HandicapDialog::startHideCursor);
	connect(m_ui->hideCursorStop, &QPushButton::clicked, this, [this]() { startHandicap("canvasInvert", -1, QJsonObject()); });

	connect(m_ui->cursorInvertStart, &QPushButton::clicked, this, &HandicapDialog::startCursorInvert);
	connect(m_ui->cursorInvertStop, &QPushButton::clicked, this, [this]() { startHandicap("cursorInvert", -1, QJsonObject()); });

	connect(m_ui->quakeStart, &QPushButton::clicked, this, &HandicapDialog::startEarthquake);
	connect(m_ui->quakeStop, &QPushButton::clicked, this, [this]() { startHandicap("earthquake", -1, QJsonObject()); });

	connect(state, &HandicapState::blackout, this, &HandicapDialog::blackoutActivated);
	connect(state, &HandicapState::canvasInvert, this, &HandicapDialog::canvasInvertActivated);
	connect(state, &HandicapState::hideCursor, this, &HandicapDialog::cursorHidden);
	connect(state, &HandicapState::cursorInvert, this, &HandicapDialog::cursorInvertActivated);
	connect(state, &HandicapState::earthquake, this, &HandicapDialog::earthquakeActivated);

	connect(m_ui->stopAllButton, &QPushButton::clicked, this, [this]() { startHandicap(QString(), 0, QJsonObject()); });
}

HandicapDialog::~HandicapDialog()
{
	delete m_ui;
}

void HandicapDialog::startBlackout()
{
	QJsonObject params;
	if(m_ui->blackoutScrub->isChecked())
		params["mode"] = "bitmap";
	else if(m_ui->blackoutSpotlight->isChecked())
		params["mode"] = "spotlight";
	else
		params["mode"] = "full";

	params["r"] = m_ui->spotlightSize->value();
	startHandicap("blackout", m_ui->blackoutTime->value(), params);
}

void HandicapDialog::blackoutActivated(BlackoutMode mode, int radius, int duration)
{
	Q_UNUSED(radius)

	const bool off = mode == BlackoutMode::Off;
	m_ui->blackoutBox->setProperty("countdown", duration);
	m_ui->blackoutTime->setEnabled(off);
	m_ui->blackoutStart->setEnabled(off);
	m_ui->blackoutFull->setEnabled(off);
	m_ui->blackoutScrub->setEnabled(off);
	m_ui->blackoutSpotlight->setEnabled(off);
	m_ui->blackoutStart->setEnabled(off);
	m_ui->spotlightSize->setEnabled(off);
	m_ui->spotlightSizeBox->setEnabled(off);
}

void HandicapDialog::startCanvasInvert()
{
	QJsonObject params;
	if(m_ui->canvasFlip->isChecked() || m_ui->canvasInvertBoth->isChecked())
		params["flip"] = true;
	if(m_ui->canvasMirror->isChecked() || m_ui->canvasInvertBoth->isChecked())
		params["mirror"] = true;

	startHandicap("canvasInvert", m_ui->canvasInvertTime->value(), params);
}

void HandicapDialog::canvasInvertActivated(bool flip, bool mirror, int duration)
{
	const bool off = !flip & !mirror;

	m_ui->invertCanvasBox->setProperty("countdown", duration);
	m_ui->canvasFlip->setEnabled(off);
	m_ui->canvasMirror->setEnabled(off);
	m_ui->canvasInvertBoth->setEnabled(off);
	m_ui->canvasInvertStart->setEnabled(off);
	m_ui->canvasInvertTime->setEnabled(off);
}

void HandicapDialog::startHideCursor()
{
	startHandicap("hideCursor", m_ui->hideCursorTime->value(), QJsonObject());
}

void HandicapDialog::cursorHidden(int duration)
{
	const bool off = duration <= 0;
	m_ui->hideCursorBox->setProperty("countdown", duration);
	m_ui->hideCursorTime->setEnabled(off);
	m_ui->hideCursorStart->setEnabled(off);
}

void HandicapDialog::startCursorInvert()
{
	QJsonObject params;
	if(m_ui->cursorFlip->isChecked() || m_ui->cursorFlipMirror->isChecked())
		params["flip"] = true;
	if(m_ui->cursorMirror->isChecked() || m_ui->cursorFlipMirror->isChecked())
		params["mirror"] = true;

	startHandicap("cursorInvert", m_ui->cursorInvertTime->value(), params);
}

void HandicapDialog::cursorInvertActivated(bool flip, bool mirror, int duration)
{
	const bool off = !flip & !mirror;

	m_ui->invertCursorBox->setProperty("countdown", duration);
	m_ui->cursorFlip->setEnabled(off);
	m_ui->cursorMirror->setEnabled(off);
	m_ui->cursorFlipMirror->setEnabled(off);
	m_ui->cursorInvertStart->setEnabled(off);
	m_ui->cursorInvertTime->setEnabled(off);
}

void HandicapDialog::startEarthquake()
{
	QJsonObject params;
	params["h"] = m_ui->quakeHorizontal->value();
	params["v"] = m_ui->quakeVertical->value();
	startHandicap("earthquake", m_ui->quakeTime->value(), params);
}

void HandicapDialog::earthquakeActivated(int h, int v, int duration)
{
	const bool off = h==0 && v==0;

	m_ui->earthquakeBox->setProperty("countdown", duration);
	m_ui->quakeTime->setEnabled(off);
	m_ui->quakeStart->setEnabled(off);
	m_ui->quakeVertical->setEnabled(off);
	m_ui->quakeHorizontal->setEnabled(off);
	m_ui->quakeVBox->setEnabled(off);
	m_ui->quakeHBox->setEnabled(off);
}


void HandicapDialog::startHandicap(const QString &name, int duration, const QJsonObject &params)
{
	QJsonObject cmd;
	cmd["type"] = "handicap";
	cmd["name"] = name;
	if(duration>=0)
		cmd["expires"] = duration;

	if(!params.isEmpty())
		cmd["params"] = params;
	emit message(protocol::MessagePtr(new protocol::ExtensionCmd(0, QJsonDocument(cmd))));
}

void HandicapDialog::countdown()
{
	QGroupBox * const countdowns[] = {
		m_ui->blackoutBox,
		m_ui->invertCanvasBox,
		m_ui->hideCursorBox,
		m_ui->invertCursorBox,
		m_ui->earthquakeBox
	};
	for(unsigned int i=0;i<sizeof(countdowns)/sizeof(*countdowns);++i) {
		QGroupBox *w = countdowns[i];
		QVariant v = w->property("countdown");
		if(!v.isNull()) {

			QString originalTitle = w->property("originalTitle").toString();
			if(originalTitle.isEmpty()) {
				originalTitle = w->title();
				w->setProperty("originalTitle", originalTitle);
			}

			const int value = v.toInt() - 1;

			if(value <= 0) {
				w->setProperty("countdown", QVariant());
				w->setTitle(originalTitle);
			} else {
				w->setProperty("countdown", value);
				w->setTitle(QStringLiteral("%1 - %2 s").arg(originalTitle).arg(value));
			}
		}
	}
}

}
