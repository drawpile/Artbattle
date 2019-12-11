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
#include <QDateTime>
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

	connect(state, &HandicapState::blackout, this, &HandicapDialog::blackoutActivated);
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
	else if(m_ui->blackoutSpotlight->isChecked()) {
		params["mode"] = "spotlight";
		params["r"] = m_ui->spotlightSize->value();
	} else
		params["mode"] = "full";

	startHandicap("blackout", m_ui->blackoutTime->value(), params);
}

void HandicapDialog::blackoutActivated(BlackoutMode mode, int radius, int duration)
{
	Q_UNUSED(radius)

	const bool off = mode == BlackoutMode::Off;
	if(off) {
		m_ui->blackoutTime->setValue(m_ui->blackoutTime->property("originalValue").toInt());
	} else {
		m_ui->blackoutTime->setProperty("originalValue", m_ui->blackoutTime->value());
		m_ui->blackoutTime->setValue(duration);
	}

	m_ui->blackoutTime->setEnabled(off);
	m_ui->blackoutStart->setEnabled(off);
	m_ui->blackoutFull->setEnabled(off);
	m_ui->blackoutScrub->setEnabled(off);
	m_ui->blackoutSpotlight->setEnabled(off);
	m_ui->blackoutStart->setEnabled(off);
	m_ui->spotlightSize->setEnabled(off);
	m_ui->spotlightSizeBox->setEnabled(off);
}

void HandicapDialog::startHandicap(const QString &name, int duration, const QJsonObject &params)
{
	QJsonObject cmd;
	cmd["type"] = "handicap";
	cmd["name"] = name;
	if(duration>=0)
		cmd["expires"] = QDateTime::currentDateTime().toSecsSinceEpoch() + qMax(1, duration);

	if(!params.isEmpty())
		cmd["params"] = params;
	emit message(protocol::MessagePtr(new protocol::ExtensionCmd(0, QJsonDocument(cmd))));
}

void HandicapDialog::countdown()
{
	QSpinBox * const countdowns[] = {
		m_ui->blackoutTime
	};

	for(unsigned int i=0;i<sizeof(countdowns)/sizeof(QWidget*);++i) {
		QSpinBox *box = countdowns[i];
		if(box->isEnabled())
			continue;
		const int v = box->value() - 1;
		if(v >= 0)
			box->setValue(v);
	}
}

}