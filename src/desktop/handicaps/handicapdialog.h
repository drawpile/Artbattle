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

#ifndef HANDICAPDIALOG_H
#define HANDICAPDIALOG_H

#include <QDialog>

#include "../../libshared/net/message.h"
#include "handicaps.h"

class QSpinBox;

class Ui_HandicapDialog;

namespace handicaps {

class HandicapDialog : public QDialog
{
	Q_OBJECT
public:
	HandicapDialog(HandicapState *state, QWidget *parent=nullptr);
	~HandicapDialog();

signals:
	void message(const protocol::MessagePtr &msg);

public slots:
	void blackoutActivated(BlackoutMode mode, int radius, int duration);
	void canvasInvertActivated(bool flip, bool mirror, int duration);
	void cursorHidden(int duration);
	void cursorInvertActivated(bool flip, bool mirror, int duration);
	void earthquakeActivated(int h, int v, int duration);
	void wanderingCursorActivated(float strength, int duration);
	void brushSizeJitterActivated(float strength, int duration);

private slots:
	void startBlackout();
	void startCanvasInvert();
	void startHideCursor();
	void startCursorInvert();
	void startEarthquake();
	void startWanderingCursor();
	void startBrushSizeJitter();

	void countdown();

private:
	void startHandicap(const QString &name, int duration, const QJsonObject &params);

	Ui_HandicapDialog *m_ui;
	HandicapState *m_state;
	QTimer *m_countdown;
};

}

#endif
