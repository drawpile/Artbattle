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

	m_hideCursorTimer = new QTimer(this);
	m_hideCursorTimer->setSingleShot(true);
	connect(m_hideCursorTimer, &QTimer::timeout, this, [this]() { emit hideCursor(0); });

	m_cursorInverTimer = new QTimer(this);
	m_cursorInverTimer->setSingleShot(true);
	connect(m_cursorInverTimer, &QTimer::timeout, this, [this]() { emit cursorInvert(false, false, 0); });

	m_earthquakeTimer = new QTimer(this);
	m_earthquakeTimer->setSingleShot(true);
	connect(m_earthquakeTimer, &QTimer::timeout, this, [this]() { emit earthquake(0, 0, 0); });

	m_wanderingCursorTimer = new QTimer(this);
	m_wanderingCursorTimer->setSingleShot(true);
	connect(m_wanderingCursorTimer, &QTimer::timeout, this, [this]() { emit wanderingCursor(0, 0); });

	m_brushSizeJitterTimer = new QTimer(this);
	m_brushSizeJitterTimer->setSingleShot(true);
	connect(m_brushSizeJitterTimer, &QTimer::timeout, this, [this]() { emit brushSizeJitter(0, 0); });
}

void HandicapState::activate(const QString &name, int expiration, const QJsonObject &params)
{
	if(name.isEmpty()) {
		qInfo() << "Clearing all handicaps";
		emit blackout(BlackoutMode::Off, 0, 0);
		emit canvasInvert(false, false, 0);
		emit hideCursor(0);
		emit cursorInvert(false, false, 0);
		emit earthquake(0, 0, 0);
		emit wanderingCursor(0, 0);
		emit brushSizeJitter(0, 0);
		return;
	}

	qInfo() << "Handicap:" << name << "expires in" << expiration << "seconds";

	QTimer *timer = nullptr;

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
			timer = m_blackoutTimer;
		else
			mode = BlackoutMode::Off;

		emit blackout(mode, params["r"].toInt(), expiration);

	} else if(name == "canvasInvert") {
		emit canvasInvert(
			params["flip"].toBool(),
			params["mirror"].toBool(),
			expiration
		);

		timer = m_canvasInvertTimer;

	} else if(name == "hideCursor") {
		emit hideCursor(expiration);
		timer = m_hideCursorTimer;

	} else if(name == "cursorInvert") {
		emit cursorInvert(
			params["flip"].toBool(),
			params["mirror"].toBool(),
			expiration
		);
		timer = m_cursorInverTimer;

	} else if(name == "earthquake") {
		emit earthquake(
			params["h"].toInt(),
			params["v"].toInt(),
			expiration
		);
		timer = m_earthquakeTimer;

	} else if(name == "wanderingCursor") {
		emit wanderingCursor(
			params["speed"].toDouble(),
			expiration
		);
		timer = m_wanderingCursorTimer;

	} else if(name == "brushSizeJitter") {
		emit brushSizeJitter(
			params["strength"].toDouble(),
			expiration
		);
		timer = m_brushSizeJitterTimer;

	} else {
		qWarning() << "Unhandled handicap type:" << name;
	}

	if(timer && expiration > 0)
		timer->start(expiration * 1000);
}

}
