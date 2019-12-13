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

	m_canvasInvertTimer = new QTimer(this);
	m_canvasInvertTimer->setSingleShot(true);
	connect(m_canvasInvertTimer, &QTimer::timeout, this, [this]() { emit canvasInvert(false, false, 0); });
}

void HandicapState::activate(const QString &name, int expiration, const QJsonObject &params)
{
	if(name.isEmpty()) {
		qInfo() << "Clearing all handicaps";
		emit blackout(BlackoutMode::Off, 0, 0);
		emit canvasInvert(false, false, 0);
		return;
	}

	qInfo() << "Handicap:" << name << "expires in" << expiration << "seconds";

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

		if(expiration > 0)
			m_blackoutTimer->start(expiration * 1000);
		else
			mode = BlackoutMode::Off;

		emit blackout(mode, params["r"].toInt(), expiration);

	} else if(name == "canvasInvert") {
		emit canvasInvert(
			params["flip"].toBool(),
			params["mirror"].toBool(),
			expiration
		);

		if(expiration > 0)
			m_canvasInvertTimer->start(expiration * 1000);

	} else {
		qWarning() << "Unhandled handicap type:" << name;
		return;
	}
}

}
