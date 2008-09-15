/*
   DrawPile - a collaborative drawing program.

   Copyright (C) 2008 Calle Laakkonen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include <QDebug>
#include <QImage>

#include "layer.h"
#include "tile.h"
#include "brush.h"
#include "point.h"

namespace dpcore {

Layer::Layer(const QImage& image) {
	width_ = image.width();
	height_ = image.height();
	xtiles_ = width_ / Tile::SIZE + ((width_ % Tile::SIZE)>0);
	ytiles_ = height_ / Tile::SIZE + ((height_ % Tile::SIZE)>0);
	tiles_ = new Tile*[xtiles_ * ytiles_];
	for(int y=0;y<ytiles_;++y)
		for(int x=0;x<xtiles_;++x)
			tiles_[y*xtiles_+x] = new Tile(image, x, y);
}

Layer::Layer(const QColor& color, int width, int height) {
	width_ = width;
	height_ = height;
}

QImage Layer::toImage() const {
	QImage image(width_, height_, QImage::Format_RGB32);
	for(int i=0;i<xtiles_*ytiles_;++i)
		tiles_[i]->copyToImage(image);

	return image;
}

void Layer::paint(const QRectF& rect, QPainter *painter) {
	const int top = qMax(int(rect.top()), 0);
	const int left = qMax(int(rect.left()), 0);
	const int right = Tile::roundTo(qMin(int(rect.right()), width_));
	const int bottom = Tile::roundTo(qMin(int(rect.bottom()), height_));

	for(int y=top;y<bottom;y+=Tile::SIZE) {
		const int yindex = (y/Tile::SIZE);
		for(int x=left;x<right;x+=Tile::SIZE) {
			const int xindex = x/Tile::SIZE;
			tiles_[xtiles_*yindex + xindex]->paint(painter,
					QPoint(xindex*Tile::SIZE, yindex*Tile::SIZE));
		}
	}
}

/**
 * @param x
 * @param y
 * @return invalid color if x or y is outside image boundaries
 */
QColor Layer::colorAt(int x, int y)
{
	if(x<0 || y<0 || x>=width_ || y>=height_)
		return QColor();
	const int yindex = y/Tile::SIZE;
	const int xindex = x/Tile::SIZE;
	return QColor::fromRgb(
			tiles_[yindex*xtiles_ + xindex]->pixel(
				x-xindex*Tile::SIZE,
				y-yindex*Tile::SIZE
				));
}

/**
 * Apply a single dab of the brush to the layer
 * @param brush brush to use
 * @parma point where to dab. May be outside the image.
 */
void Layer::dab(const Brush& brush, const Point& point)
{
	const int dia = brush.diameter(point.pressure());
	const int top = point.y() - brush.radius(point.pressure());
	const int left = point.x() - brush.radius(point.pressure());
	const int bottom = top + dia;
	const int right = left + dia;
	if(left+dia<=0 || top+dia<=0 || left>=width_ || top>=height_)
		return;

	// Render the brush
	uchar *values = brush.render(point.pressure());
	QColor color = brush.color(point.pressure());

	// A single dab can (and often does) span multiple tiles.
	int y = top;
	int yb = 0; // y in relation to brush origin
	while(y<bottom) {
		const int yindex = y / Tile::SIZE;
		const int yt = y - yindex * Tile::SIZE;
		const int hb = yt+dia-yb < Tile::SIZE ? dia-yb : Tile::SIZE-yt;
		int x = left;
		int xb = 0; // x in relation to brush origin
		while(x<right) {
			const int xindex = x / Tile::SIZE;
			const int xt = x - xindex * Tile::SIZE;
			const int wb = xt+dia-xb < Tile::SIZE ? dia-xb : Tile::SIZE-xt;

			tiles_[xtiles_ * yindex + xindex]->composite(
					values + yb * dia + xb,
					color,
					xt, yt,
					wb, hb,
					dia-wb
					);

			x = (xindex+1) * Tile::SIZE;
			xb = xb + wb;
		}
		y = (yindex+1) * Tile::SIZE;
		yb = yb + hb;
	}
}

/**
 * The last point is not drawn, so successive lines can be drawn blotches.
 * @param brush brush to draw the line with
 * @param from starting point
 * @param to ending point
 * @param distance distance from previous dab.
 */
void Layer::drawLine(const Brush& brush, const Point& from, const Point& to, int *distance) {
	#if 0
	qreal pressure = point1.pressure();
	qreal deltapressure;
	if(qAbs(pressure2-pressure1) < 1.0/255.0)
		deltapressure = 0;
	else
		deltapressure = (pressure2-pressure1) / hypot(point1.x()-to.x(), point1.y()-to.y());
	#endif

	const int spacing = brush.spacing()*brush.radius(from.pressure())/100;

	Point point = from;
	int &x0 = point.rx();
	int &y0 = point.ry();
	int x1 = to.x();
	int y1 = to.y();
	int dy = y1 - y0;
	int dx = x1 - x0;
	int stepx, stepy;

	if (dy < 0) {
		dy = -dy;
		stepy = -1;
	} else {
		stepy = 1;
	}
	if (dx < 0) {
		dx = -dx;
		stepx = -1;
	} else {
		stepx = 1;
	}

	dy *= 2;
	dx *= 2;

	if (dx > dy) {
		int fraction = dy - (dx >> 1);
		while (x0 != x1) {
			if (fraction >= 0) {
				y0 += stepy;
				fraction -= dx;
			}
			x0 += stepx;
			fraction += dy;
			if(distance) {
				if(++*distance > spacing) {
					dab(brush, point);
					*distance = 0;
				}
			} else {
				dab(brush, point);
			}
		}
	} else {
		int fraction = dx - (dy >> 1);
		while (y0 != y1) {
			if (fraction >= 0) {
				x0 += stepx;
				fraction -= dy;
			}
			y0 += stepy;
			fraction += dx;
			if(distance) {
				if(++*distance > spacing) {
					dab(brush, point);
					*distance = 0;
				}
			} else {
				dab(brush, point);
			}
		}
	}
}

}
