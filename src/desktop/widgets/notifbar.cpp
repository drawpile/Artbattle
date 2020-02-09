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

#include "notifbar.h"
#include "utils/icon.h"

#include <QResizeEvent>
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QStyle>

namespace widgets {

NotificationBar::NotificationBar(QWidget *parent)
	: QWidget(parent)
{
	hide();

	auto *layout = new QHBoxLayout;
	layout->setContentsMargins(15, 15, 15, 15);
	setLayout(layout);

	m_label = new QLabel(this);
	m_label->setText("Hello\nThere\nthird line\nfourth line");
	layout->addWidget(m_label, 1);

	m_actionButton = new QPushButton(this);
	layout->addWidget(m_actionButton);
	connect(m_actionButton, &QPushButton::clicked, this, &NotificationBar::actionButtonClicked);
	connect(m_actionButton, &QPushButton::clicked, this, &NotificationBar::hide);

	QPushButton *closeBtn = new QPushButton(this);
	closeBtn->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	layout->addWidget(closeBtn);
	connect(closeBtn, &QPushButton::clicked, this, &NotificationBar::hide);

	auto *dropshadow = new QGraphicsDropShadowEffect(this);
	dropshadow->setOffset(0, 2);
	dropshadow->setBlurRadius(15);
	dropshadow->setColor(Qt::black);

	setGraphicsEffect(dropshadow);
}

void NotificationBar::show(const QString &text, const QString &actionButtonLabel, RoleColor color)
{
	Q_ASSERT(parentWidget());

	switch(color) {
	case RoleColor::Warning: setColor("#ed1515");
	}

	m_label->setText(text);
	m_actionButton->setHidden(actionButtonLabel.isEmpty());
	m_actionButton->setText(actionButtonLabel);
	QWidget::show();
}

void NotificationBar::setColor(const QColor &color)
{
	m_color = color;
	auto pal = palette();
	if(icon::isDark(color))
		pal.setColor(QPalette::Text, Qt::white);
	else
		pal.setColor(QPalette::Text, Qt::black);
	setPalette(pal);
	update();
}

void NotificationBar::showEvent(QShowEvent *)
{
	Q_ASSERT(parentWidget());
	updateSize(parentWidget()->size());
	parent()->installEventFilter(this);
}

void NotificationBar::hideEvent(QHideEvent *)
{
	Q_ASSERT(parent());
	parent()->removeEventFilter(this);
}

bool NotificationBar::eventFilter(QObject *obj, QEvent *event)
{
	Q_ASSERT(obj == parent());
	if(event->type() == QEvent::Resize) {
		const QResizeEvent *re = static_cast<const QResizeEvent*>(event);
		updateSize(re->size());
	}

	return false;
}

void NotificationBar::paintEvent(QPaintEvent *e)
{
	QPainter p(this);
	p.fillRect(e->rect(), m_color);
}

void NotificationBar::updateSize(const QSize &parentSize)
{
	const int minHeight = qMax(
		m_label->sizeHint().height(),
		m_actionButton->sizeHint().height()
	);
	move(0, 0);
	resize(
		parentSize.width(),
		minHeight + 30
	);
}

}
