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
#include <QJsonArray>
#include <QDebug>

namespace handicaps {

HandicapState::HandicapState(QObject *parent)
	: QObject(parent)
{
	m_blackoutTimer = new QTimer(this);
	m_blackoutTimer->setSingleShot(true);
	connect(m_blackoutTimer, &QTimer::timeout, this, [this]() { emit blackout(BlackoutMode::Off, 0, 0); });
}

void HandicapState::activate(const QString &name, const QDateTime &expiration, const QJsonObject &params)
{
	if(name == "stop") {
		qInfo() << "Clearing all handicaps";
		emit blackout(BlackoutMode::Off, 0, 0);
		return;
	}

	const int expiresInMsecs = int(qBound(-1ll, QDateTime::currentDateTime().msecsTo(expiration), 3600 * 1000ll));
	const int durationSecs = expiresInMsecs / 1000;

	qInfo() << "Handicap:" << name << "expires in" << durationSecs << "seconds";

	if(name == "blackout") {
		BlackoutMode mode = BlackoutMode::Simple;

		const QString modeStr = params["mode"].toString();
		if(modeStr == "bitmap")
			mode = BlackoutMode::Bitmap;
		else if(modeStr == "spotlight")
			mode = BlackoutMode::Spotlight;
		else if(modeStr == "full")
			mode = BlackoutMode::Simple;
		else
			mode = BlackoutMode::Off;

		if(expiresInMsecs > 0)
			m_blackoutTimer->start(expiresInMsecs);
		else
			mode = BlackoutMode::Off;

		emit blackout(mode, params["r"].toInt(), durationSecs);

	} else {
		qWarning() << "Unhandled handicap type:" << name;
		return;
	}
}

}
