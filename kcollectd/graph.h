/* -*- c++ -*- */
/*
 * This file is part of the source of kcollectd, a viewer for
 * rrd-databases created by collectd
 * 
 * Copyright (C) 2008 M G Berberich
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>

#include <QFrame>
#include <QPixmap>
#include <QRect>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>

#include "misc.h"

class time_iterator;

class GraphInfo
{
public:

  struct datasource {
    QString rrd;
    QString ds;
    QString label;
    std::vector<double> avg_data, min_data, max_data;
  };

  void add(const QString &rrd, const QString &ds, const QString &label);
  void add(const datasource &d) { dslist.push_back(d); }
  void clear() { dslist.clear(); }
  size_t size() const { return dslist.size(); }
  Range minmax();
  Range minmax_adj(double *base);

  int top() const { return top_; }
  int bottom() const { return bottom_; }
  int legend_lines() const { return legend_lines_; }
  void top(int t) { top_ = t; }
  void bottom(int b) { bottom_ = b; }
  void legend_lines(int l) { legend_lines_ = l; }

  // iterators pointing to datasources
  typedef std::vector<datasource>::iterator iterator;
  typedef std::vector<datasource>::const_iterator const_iterator;

  iterator begin() { return dslist.begin(); }
  iterator end()   { return dslist.end(); }
  const_iterator begin() const { return dslist.begin(); } 
  const_iterator end() const   { return dslist.end(); }

  void erase(iterator i) { dslist.erase(i); }
  bool empty() const { return dslist.empty(); }

private:
  
  int top_, bottom_, legend_lines_;
  std::vector<datasource> dslist;
};

/**
 * subclassed MimeData for interal drag'n drop
 */
class GraphMimeData : public QMimeData {
  Q_OBJECT
 public:
  void setGraph(const QString &rrd, const QString &ds, const QString &label);
  const QString &rrd() const { return rrd_; }
  const QString &ds() const { return ds_; }
  const QString &label() const { return label_; }
 private:
  QString rrd_;
  QString ds_;
  QString label_;
};

/**
 *
 */
class Graph : public QFrame
{
  Q_OBJECT;
 public:

  typedef std::vector<GraphInfo> graph_list;
  typedef graph_list::iterator iterator;
  typedef graph_list::const_iterator const_iterator;

  explicit Graph(QWidget *parent=0);
  Graph(QWidget *parent, const std::string &rrd, const std::string &ds, 
	const char *name=0);

  void clear();
  GraphInfo &add(const QString &rrd, const QString &ds, const QString &label);
  GraphInfo &add();

  bool changed() { return changed_state; }
  void changed(bool c) { changed_state = c; }

  void autoUpdate(bool active);
  bool autoUpdate() { return (autoUpdateTimer != -1); }

  virtual QSize sizeHint() const;
  virtual void paintEvent(QPaintEvent *ev);
  virtual void resizeEvent(QResizeEvent*);
  
  virtual void last(time_t span);
  virtual void zoom(double factor);
  virtual void mousePressEvent(QMouseEvent *e);
  virtual void mouseMoveEvent(QMouseEvent *e);
  virtual void wheelEvent(QWheelEvent *e);
  virtual void timerEvent(QTimerEvent *event);
  // drag-and-drop
  virtual void dragEnterEvent(QDragEnterEvent *event);
  virtual void dragMoveEvent(QDragMoveEvent *event);
  virtual void dropEvent(QDropEvent *event);
  
  // Iterators
  iterator begin() { return glist.begin(); }
  iterator end()   { return glist.end(); }
  const_iterator begin() const { return glist.begin(); }
  const_iterator end() const   { return glist.end(); }

  bool empty() const { return glist.empty(); }
  time_t range() { return span; }

public slots:
  virtual void removeGraph();
  virtual void splitGraph();

 private:
  bool fetchAllData();
  void drawAll();
  int calcLegendHeights(int box_size, int width);
  void drawLegend(QPainter &paint, int left, int pos, 
	int box_size, const GraphInfo &ginfo);
  void drawFooter(QPainter &paint, int left, int right);
  void drawHeader(QPainter &paint);
  void drawYLines(QPainter &paint, const QRect &rect, 
	const Range &y_range, double base, QColor color);
  void drawYLabel(QPainter &paint, const QRect &rect, 
	const Range &range, double base);
  void drawXLines(QPainter &paint, const QRect &rect, 
	time_iterator it, QColor color);
  void drawXLabel(QPainter &paint, int y, int left, int right, 
	time_iterator it, QString format, bool center);
  void findXGrid(int width, QString &format, bool &center, 
       time_iterator &minor_x, time_iterator &major_x, time_iterator &label_x );
  void drawGraph(QPainter &paint, const QRect &rect, const GraphInfo &gi, 
	double min, double max);
  void layout();
  
  graph_list::iterator graphAt(const QPoint &pos);
  graph_list::const_iterator graphAt(const QPoint &pos) const;

  // rrd-data
  graph_list glist;
  bool data_is_valid;
  time_t start;		// user set start of graph
  time_t span;		// user-set span of graph
  time_t data_start;	// real start of data (from rrd_fetch)
  time_t data_end;	// real end of data (from rrd_fetch)
  time_t tz_off; 	// offset of the local timezone from GMT
  unsigned long step;

  // technical helpers
  int origin_x, origin_y;
  time_t origin_start, origin_end;
  bool dragging;

  // widget-data
  QFont font, header_font, small_font;
  QPixmap offscreen;
  QRect graph_rect;
  int graph_height, label_width, box_size;
  int label_y1, label_y2;
  QColor color_major, color_minor, color_graph_bg;
  QColor color_minmax[8], color_line[8];

  // Auto-Update
  int autoUpdateTimer;
  time_t timer_diff;

  // state
  bool changed_state;
};

/**
 * add a datasource to the GraphInfo
 */
inline void 
GraphInfo::add(const QString &rrd, const QString &ds, const QString &label)
{
  datasource new_ds;
  new_ds.rrd = rrd;
  new_ds.ds = ds;
  new_ds.label = label;
  dslist.push_back(new_ds);
}

/**
 * set graph-infos.
 */
inline void 
GraphMimeData::setGraph(const QString &r, const QString &d, const QString &l)
{
  rrd_ = r;
  ds_ = d;
  label_ = l;
}

/**
 * 
 */
inline GraphInfo &
Graph::add(const QString &rrd, const QString &ds, const QString &label)
{
  GraphInfo &gi = add();
  gi.add(rrd, ds, label);
  data_is_valid = false;
  return gi;
}

inline GraphInfo &Graph::add() 
{ 
  GraphInfo gi; 
  glist.push_back(gi);
  changed(true);
  layout();
  update();
  return glist.back();
}


inline QSize Graph::sizeHint() const
{
  return QSize(640, 480);
}

#endif
