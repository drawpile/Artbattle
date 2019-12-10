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
#include "handicaps.h"
#include "../../libshared/net/meta2.h"

#include "ui_handicaps.h"

#include <QJsonObject>
#include <QDateTime>
#include <QTimer>

namespace handicaps {

struct HandicapChoice {
	QString mode;
	bool hasStrength;
};

}

Q_DECLARE_METATYPE(handicaps::HandicapChoice)

namespace handicaps {

HandicapDialog::HandicapDialog(HandicapState *state, QWidget *parent)
	: QDialog(parent),
	  m_ui(new Ui_HandicapDialog),
	  m_state(state)
{
	m_ui->setupUi(this);

	m_ui->handicapChoice->addItem("Blackout - simple", QVariant::fromValue<HandicapChoice>({
		QString(),
		false
   }));
	m_ui->handicapChoice->addItem("Blackout - scrub", QVariant::fromValue<HandicapChoice>({
		"bitmap",
		false
   }));
	m_ui->handicapChoice->addItem("Blackout - spotlight", QVariant::fromValue<HandicapChoice>({
		"spotlight",
		false
   }));

	m_countdown = new QTimer(this);
	m_countdown->setSingleShot(false);
	m_countdown->setInterval(1000);

	connect(m_ui->startButton, &QPushButton::clicked, this, &HandicapDialog::startHandicap);
	connect(m_ui->stopButton, &QPushButton::clicked, this, &HandicapDialog::stopHandicap);
	connect(state, &HandicapState::activated, this, &HandicapDialog::handicapActivated);
	connect(m_countdown, &QTimer::timeout, this, &HandicapDialog::countdown);
	connect(m_ui->handicapChoice, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
		const auto c = m_ui->handicapChoice->currentData().value<HandicapChoice>();
		m_ui->handicapStrength->setEnabled(c.hasStrength);
	});
}

HandicapDialog::~HandicapDialog()
{
	delete m_ui;
}

void HandicapDialog::startHandicap()
{
	QJsonObject cmd;
	cmd["type"] = "handicap";
	cmd["name"] = "blackout";
	cmd["expires"] = QDateTime::currentDateTime().addSecs(m_ui->handicapExpiration->value()).toSecsSinceEpoch();

	const auto choice = m_ui->handicapChoice->currentData().value<HandicapChoice>();
	QJsonObject params;
	if(!choice.mode.isEmpty())
		params["mode"] = choice.mode;

	if(!params.isEmpty())
		cmd["params"] = params;

	emit message(protocol::MessagePtr(new protocol::ExtensionCmd(0, QJsonDocument(cmd))));
}

void HandicapDialog::stopHandicap()
{
	QJsonObject cmd;
	cmd["type"] = "handicap";
	emit message(protocol::MessagePtr(new protocol::ExtensionCmd(0, QJsonDocument(cmd))));
}

void HandicapDialog::handicapActivated(const QString &title, int seconds)
{
	m_ui->currentName->setText(title.isEmpty() ? QStringLiteral("None") : title);
	m_ui->currentExpiration->setMaximum(qMax(1, seconds));
	m_ui->currentExpiration->setValue(seconds);
	if(!title.isEmpty())
		m_countdown->start();
}

void HandicapDialog::countdown()
{
	const int v = m_ui->currentExpiration->value() - 1;
	if(v<=0)
		m_countdown->stop();
	m_ui->currentExpiration->setValue(v);
}

}
