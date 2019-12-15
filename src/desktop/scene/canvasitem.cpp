/*
   Drawpile - a collaborative drawing program.

   Copyright (C) 2006-2019 Calle Laakkonen

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

#include "canvasitem.h"

#include "core/layerstackpixmapcacheobserver.h"
#include "core/layerstack.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>

#include <QDebug>

namespace drawingboard {

/**
 * @param parent use another QGraphicsItem as a parent
 * @param scene the picture to which this layer belongs to
 */
CanvasItem::CanvasItem(paintcore::LayerStackPixmapCacheObserver *layerstack, QGraphicsItem *parent)
	: QGraphicsObject(parent), m_image(layerstack)
{
	connect(m_image, &paintcore::LayerStackPixmapCacheObserver::areaChanged, this, &CanvasItem::refreshImage);
	connect(m_image, &paintcore::LayerStackPixmapCacheObserver::resized, this, &CanvasItem::canvasResize);
	setFlag(ItemUsesExtendedStyleOption);

	setAcceptHoverEvents(true);
}

void CanvasItem::refreshImage(const QRect &area)
{
	update(area.adjusted(-2, -2, 2, 2));
}

void CanvasItem::canvasResize()
{
	prepareGeometryChange();
	m_blackoutLayer = QBitmap();
}

QRectF CanvasItem::boundingRect() const
{
	if(m_image->layerStack())
		return QRectF(0,0, m_image->layerStack()->width(), m_image->layerStack()->height());
	return QRectF();
}

void CanvasItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
	 QWidget *)
{
	const QRect exposed = option->exposedRect.adjusted(-1, -1, 1, 1).toAlignedRect();

	if(m_blackoutMode == int(handicaps::BlackoutMode::Simple)) {
		painter->fillRect(exposed, Qt::black);
		return;
	}

	if(m_blackoutMode == int(handicaps::BlackoutMode::Spotlight)) {
		painter->fillRect(exposed, Qt::black);

		const QRectF spotlight {
			m_spotlightPos - QPointF(m_spotlightSize, m_spotlightSize),
			QSizeF(m_spotlightSize*2, m_spotlightSize*2)
		};

		if(exposed.intersects(spotlight.toRect())) {
			QPainterPath mask;
			mask.addEllipse(spotlight);
			painter->setClipPath(mask);

			painter->drawPixmap(exposed, m_image->getPixmap(exposed), exposed);
		}

		return;
	}

	painter->drawPixmap(exposed, m_image->getPixmap(exposed), exposed);

	if(m_blackoutMode == int(handicaps::BlackoutMode::Bitmap)) {
		if(m_blackoutLayer.isNull()) {
			m_blackoutLayer = QBitmap(m_image->getPixmap().size());
			m_blackoutLayer.fill(Qt::black);
		}
		painter->drawPixmap(exposed, m_blackoutLayer, exposed);
	}
}

void CanvasItem::setBlackoutHandicap(handicaps::BlackoutMode mode, int radius)
{
	m_blackoutMode = int(mode);
	m_spotlightSize = qBound(1, radius, 120);

	if(m_blackoutMode == int(handicaps::BlackoutMode::Bitmap)) {
		m_blackoutLayer = QBitmap(m_image->getPixmap().size());
		m_blackoutLayer.fill(Qt::black);
	} else {
		m_blackoutLayer = QBitmap();
	}

	update();
}

void CanvasItem::pointerMove(const QPointF &pos)
{
	if(m_blackoutMode == int(handicaps::BlackoutMode::Bitmap)) {
		if(m_blackoutLayer.isNull()) {
			m_blackoutLayer = QBitmap(m_image->getPixmap().size());
			m_blackoutLayer.fill(Qt::black);
		}

		QPainter p(&m_blackoutLayer);

		const QRectF s(-m_spotlightSize + pos.x(), -m_spotlightSize + pos.y(), m_spotlightSize*2, m_spotlightSize*2);
		p.setBrush(Qt::white);
		p.setPen(Qt::NoPen);
		p.drawEllipse(s);
		update(s);

	} else if(m_blackoutMode == int(handicaps::BlackoutMode::Spotlight)) {
		const QRectF s(-m_spotlightSize, -m_spotlightSize, m_spotlightSize*2, m_spotlightSize*2);
		update(s.translated(m_spotlightPos));
		update(s.translated(pos));
	}

	m_spotlightPos = pos;
}

}

