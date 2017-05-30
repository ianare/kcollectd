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

#ifndef GUI_H
#define GUI_H

#include <KMainWindow>
#include <kactioncollection.h>

#include "graph.h"

class QLabel;
class Graph;
class QTreeWidget;
class QVBoxLayout;
class KAction;
class KPushButton;

class KCollectdGui : public KMainWindow // QWidget
{
  Q_OBJECT;
public:
  KCollectdGui(QWidget *parent=0);
  virtual ~KCollectdGui();

  QTreeWidget *listview() { return listview_; }
  KActionCollection* actionCollection() { return &action_collection; }

  void set(Graph *graph);
  void load(const QString &filename);
  void save(const QString &filename);

public slots:  
  void startDrag(QTreeWidgetItem * widget, int col);
  virtual void last_month();
  virtual void last_week();
  virtual void last_day();
  virtual void last_hour();
  virtual void zoomIn();
  virtual void zoomOut();
  virtual void autoUpdate(bool active);
  virtual void hideTree(bool active);
  virtual void splitGraph();
  virtual void load();
  virtual void save();

protected:
  virtual void saveProperties(KConfigGroup &);
  virtual void readProperties(const KConfigGroup &);

private:
  QTreeWidget *listview_;
  QVBoxLayout *vbox;
  Graph * graph;
  KPushButton *auto_button;
  KAction *auto_action, *panel_action;
  QString filename;

  KActionCollection action_collection;
};

inline void KCollectdGui::last_month()
{
  graph->last(3600*24*31);
}

inline void KCollectdGui::last_week()
{
  graph->last(3600*24*7);
}

inline void KCollectdGui::last_day()
{
  graph->last(3600*24);
}

inline void KCollectdGui::last_hour()
{
  graph->last(3600);
}

inline void KCollectdGui::zoomIn()
{
  graph->zoom(1.0/1.259921050);
}

inline void KCollectdGui::zoomOut()
{
  graph->zoom(1.259921050);
}

inline void KCollectdGui::splitGraph()
{
  graph->splitGraph();
}

#endif
