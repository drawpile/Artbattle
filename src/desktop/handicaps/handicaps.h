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

#ifndef DP_HANDICAPS_H
#define DP_HANDICAPS_H

#include <QObject>

class QDateTime;
class QJsonObject;
class QTimer;

namespace handicaps {

enum class BlackoutMode {
	Off,
	Simple,
	Bitmap,
	Spotlight
};

class HandicapState : public QObject
{
	Q_OBJECT
public:
	HandicapState(QObject *parent);

public slots:
	void activate(const QString &name, const QDateTime &expiration, const QJsonObject &params);
	void deactivate();

signals:
	void blackout(BlackoutMode mode);
	void activated(const QString &title, int seconds);

private:
	void activateBlackout(const QJsonObject &params);

	QTimer *m_expirationTimer;
	QString m_current;
};

}

#endif
