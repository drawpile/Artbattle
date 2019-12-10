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

class Ui_HandicapDialog;

namespace handicaps {

class HandicapState;

class HandicapDialog : public QDialog
{
	Q_OBJECT
public:
	HandicapDialog(HandicapState *state, QWidget *parent=nullptr);
	~HandicapDialog();

signals:
	void message(const protocol::MessagePtr &msg);

public slots:
	void handicapActivated(const QString &name, int seconds);

private slots:
	void startHandicap();
	void stopHandicap();
	void countdown();

private:
	Ui_HandicapDialog *m_ui;
	HandicapState *m_state;
	QTimer *m_countdown;
};

}

#endif
