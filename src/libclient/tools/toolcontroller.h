/*
   Drawpile - a collaborative drawing program.

   Copyright (C) 2015-2016 Calle Laakkonen

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

#ifndef TOOLCONTROLLER_H
#define TOOLCONTROLLER_H

#include "strokesmoother.h"
#include "tool.h"
#include "brushes/brush.h"
#include "canvas/features.h"

#include <QObject>

class QCursor;

namespace canvas { class CanvasModel; }
namespace net { class Client; }

namespace tools {

class Tool;

/**
 * @brief The ToolController dispatches user input to the currently active tool
 */
class ToolController : public QObject
{
	Q_PROPERTY(QCursor activeToolCursor READ activeToolCursor() NOTIFY toolCursorChanged)
	Q_PROPERTY(int smoothing READ smoothing WRITE setSmoothing NOTIFY smoothingChanged)
	Q_PROPERTY(uint16_t activeLayer READ activeLayer WRITE setActiveLayer NOTIFY activeLayerChanged)
	Q_PROPERTY(uint16_t activeAnnotation READ activeAnnotation WRITE setActiveAnnotation NOTIFY activeAnnotationChanged)
	Q_PROPERTY(brushes::ClassicBrush activeBrush READ activeBrush WRITE setActiveBrush NOTIFY activeBrushChanged)
	Q_PROPERTY(canvas::CanvasModel* model READ model WRITE setModel NOTIFY modelChanged)

	Q_OBJECT
public:
	explicit ToolController(net::Client *client, QObject *parent=nullptr);
	~ToolController();

	void setActiveTool(Tool::Type tool);
	Tool::Type activeTool() const;

	QCursor activeToolCursor() const;

	void setActiveLayer(uint16_t id);
	uint16_t activeLayer() const { return m_activeLayer; }

	void setActiveAnnotation(uint16_t id);
	uint16_t activeAnnotation() const { return m_activeAnnotation; }

	void setActiveBrush(const brushes::ClassicBrush &b);
	const brushes::ClassicBrush &activeBrush() const { return m_activebrush; }

	void setModel(canvas::CanvasModel *model);
	canvas::CanvasModel *model() const { return m_model; }

	void setSmoothing(int smoothing);
	int smoothing() const { return m_smoothing; }

	// TODO this is used just for sending the commands. Replace with a signal?
	inline net::Client *client() const { return m_client; }

	Tool *getTool(Tool::Type type);

	//! Is there a multipart drawing operation in progress?
	bool isMultipartDrawing() const;

	/**
	 * Apply an offset to the position of the active tool
	 *
	 * This is used to correct the tool position when the canvas is
	 * resized while the local user is still drawing.
	 */
	void offsetActiveTool(int xOffset, int yOffset);

	float handicapBrushSizeOffset() const { return m_handicapBrushSizeOffset; }

public slots:
	//! Start a new stroke
	void startDrawing(const QPointF &point, qreal pressure, bool right, float zoom);

	//! Continue a stroke
	void continueDrawing(const QPointF &point, qreal pressure, bool shift, bool alt);

	//! Stylus hover (not yet drawing)
	void hoverDrawing(const QPointF &point);

	//! End a stroke
	void endDrawing();

	/**
	 * @brief Undo the latest part of a multipart drawing
	 *
	 * Multipart drawings are not committed until finishMultipartDrawing is
	 * called, so undoing is a local per-tool operation.
	 *
	 * @return false if there was nothing to undo
	 */
	bool undoMultipartDrawing();

	//! Commit the current multipart drawing (if any)
	void finishMultipartDrawing();

	//! Cancel the current multipart drawing (if any)
	void cancelMultipartDrawing();

	//! ArtBattle extension: set brush size jitter strength
	void setHandicapBrushSizeJitter(float strength);

signals:
	void activeToolChanged(Tool::Type type);
	void toolCursorChanged(const QCursor &cursor);
	void activeLayerChanged(int layerId);
	void activeAnnotationChanged(uint16_t annotationId);
	void activeBrushChanged(const brushes::ClassicBrush&);
	void modelChanged(canvas::CanvasModel *model);
	void smoothingChanged(int smoothing);

	void colorUsed(const QColor &color);
	void zoomRequested(const QRect &rect, int steps);

private slots:
	void onAnnotationRowDelete(const QModelIndex&, int first, int last);
	void onFeatureAccessChange(canvas::Feature feature, bool canUse);

private:
	void registerTool(Tool *tool);

	Tool *m_toolbox[Tool::_LASTTOOL];
	net::Client *m_client;

	canvas::CanvasModel *m_model;

	brushes::ClassicBrush m_activebrush;
	Tool *m_activeTool;
	uint16_t m_activeLayer;
	uint16_t m_activeAnnotation;
	bool m_prevShift, m_prevAlt;

	int m_smoothing;
	StrokeSmoother m_smoother;

	QTimer *m_handicapBrushSizeJitterTimer;
	float m_handicapBrushSizeJitter;
	float m_handicapBrushSizeOffset;
	float m_handicapBrushSizeOffsetTarget;
};

}

#endif // TOOLCONTROLLER_H
