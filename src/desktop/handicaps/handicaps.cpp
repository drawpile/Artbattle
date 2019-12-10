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

#include "handicaps.h"

#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QDebug>

namespace handicaps {

HandicapState::HandicapState(QObject *parent)
	: QObject(parent)
{
	m_expirationTimer = new QTimer(this);
	m_expirationTimer->setSingleShot(true);
	connect(m_expirationTimer, &QTimer::timeout, this, &HandicapState::deactivate);
}

void HandicapState::activate(const QString &name, const QDateTime &expiration, const QJsonObject &params)
{
	// First, deactivate existing handicap
	deactivate();

	if(name.isEmpty()) {
		qInfo() << "Handicap cleared.";
		return;
	}

	const qint64 expiresInMsecs = QDateTime::currentDateTime().msecsTo(expiration);

	qInfo() << "Handicap:" << name << "expires in" << expiresInMsecs/1000 << "seconds";

	if(expiresInMsecs < 0) {
		qWarning("Handicap %s expiration time was in the past", qPrintable(name));
		return;
	}

	if(name == "blackout") {
		BlackoutMode mode = BlackoutMode::Simple;

		const QString modeStr = params["mode"].toString();
		if(modeStr == "bitmap")
			mode = BlackoutMode::Bitmap;
		else if (modeStr == "spotlight")
			mode = BlackoutMode::Spotlight;

		emit blackout(mode);
		emit activated("Blackout: " + (modeStr.isEmpty() ? QString("simple") : modeStr), expiresInMsecs/1000);

	} else {
		qWarning("Unhandled handicap type: %s", qPrintable(name));
		return;
	}

	m_current = name;
	m_expirationTimer->start(int(expiresInMsecs));
}

void HandicapState::deactivate()
{
	m_current = QString();
	m_expirationTimer->stop();
	emit blackout(BlackoutMode::Off);
	emit activated(QString(), 0);
}

}
