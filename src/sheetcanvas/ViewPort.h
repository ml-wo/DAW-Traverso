/*
    Copyright (C) 2005-2006 Remon Sijrier 
 
    This file is part of Traverso
 
    Traverso is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 
*/

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QGraphicsView>
#include <QGraphicsItem>
#include "ContextItem.h"
#include "AbstractViewPort.h"

class ViewItem;
class SheetView;
class ContextItem;
class Import;
class AudioTrack;
class HoldCursor;
class QGraphicsTextItem;

class ViewPort : public QGraphicsView, public AbstractViewPort
{
        Q_OBJECT
			
public :
        ViewPort(QWidget* parent);
        ViewPort(QGraphicsScene* scene, QWidget* parent);
        virtual ~ViewPort();

        // Get functions


        // Set functions
	void set_holdcursor(const QString& cursorName);
	void set_holdcursor_text(const QString& text);
        void set_holdcursor_pos(QPointF pos);
        void set_edit_point_position(QPointF pos);
        void update_holdcursor_shape();
	void set_current_mode(int mode);
        void set_cursor_shape(const QString& cursor);
        void hide_mouse_cursor();
        virtual void set_sheetview(SheetView* view) {m_sv = view;}

	void reset_cursor();
	
	int get_current_mode() const {return m_mode;}
        QPointF get_hold_cursor_pos() const;

        void grab_mouse();
        void release_mouse();
        inline QPointF map_to_scene(int x, int y) const {
                return mapToScene(x, y);
        }




protected:
	virtual bool event(QEvent *event);
        virtual void enterEvent ( QEvent * );
        virtual void leaveEvent ( QEvent * );
        virtual void paintEvent( QPaintEvent* e);
        virtual void mouseMoveEvent(QMouseEvent* e);
        virtual void mousePressEvent ( QMouseEvent * e );
        virtual void mouseReleaseEvent ( QMouseEvent * e );
        virtual void mouseDoubleClickEvent ( QMouseEvent * e );
	virtual void wheelEvent ( QWheelEvent* e );
	virtual void keyPressEvent ( QKeyEvent* e);
	virtual void keyReleaseEvent ( QKeyEvent* e);
	void tabletEvent ( QTabletEvent * event );

        SheetView* m_sv;
	
private:
        int             m_mode;
        bool            m_holdCursorActive;
        HoldCursor*	m_holdcursor;
        QPoint		m_oldMousePos;

	// Interface wants to call mouseMoveEvent()
	friend class Interface;
};


class HoldCursor : public ContextItem, public QGraphicsItem
{
	Q_OBJECT
#if QT_VERSION >= 0x040600
        Q_INTERFACES(QGraphicsItem)
#endif

public:
	HoldCursor(ViewPort* vp);
	~HoldCursor();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	void set_text(const QString& text);
	void set_type(const QString& type);
        void set_pos(QPointF pos);

        // returns the real cursor position, i.e. the center of the cursor.
        QPointF get_scene_pos();
	void reset();

	QRectF boundingRect() const;

private:
	ViewPort* 	m_vp;
	QGraphicsTextItem* m_textItem;
	QPoint          m_pos;
	QPixmap		m_pixmap;
	QString         m_text;

};


#endif

//eof
