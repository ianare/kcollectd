/* 
 Constructs a font object that uses the application's default font. 
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

#include <time.h>

#include <vector>
#include <cmath>

#include <QPainter>
#include <QPolygon>
#include <QRect>

#include <KLocale>
#include <KGlobalSettings>
#include <KIcon>
#include <KMenu>

#include "rrd_interface.h"
#include "misc.h"
#include "timeaxis.h"
#include "graph.moc"

// some magic numbers

// colors
static const double colortable[][3] = {
  { 0, 255, 0},
  { 255, 0, 255 }, 
  { 0, 128, 255 },
  { 255, 128, 0 },
  { 0, 255, 191}, 
  { 64, 0, 255 }, 
  { 255, 0, 64 },
  { 191,255,0 } };

// magnification for the header-font
static const double header_font_mag = 1.2;

// distance between elements
const int marg = 2; 

inline double norm(const QPointF &a)
{
  return sqrt(a.x()*a.x() + a.y()*a.y());
}

/**
 * draw an arrow
 * origin: IRC:peppe
 */
void drawArrow(QPainter &p, const QPoint &first,  const QPoint &second) 
{
  p.save();

  QPen pen = p.pen();
  const int width = pen.width();

  p.translate(second);
  QPointF difference = second - first;
  double angle = atan2(difference.y(), difference.x());
  angle *= 180.0/M_PI; // to degrees
  p.rotate(angle);
  
  static const QPointF arrow[] = {
    QPoint(0.0, 0.0),
    QPoint(-5.0*width, -2.0*width),
    QPoint(-4.0*width, 0.0),
    QPoint(-5.0*width, 2.0*width)
  };
  
  p.setBrush(QBrush(pen.color(), Qt::SolidPattern));
  p.drawLine(-4.0*width, 0.0, -norm(difference), 0.0);
  pen.setWidth(0);
  p.setPen(pen);
  p.drawPolygon(arrow, 4);
  
  p.restore();
}
 
/**
 *
 */
Graph::Graph(QWidget *parent) :
  QFrame(parent), data_is_valid(false), 
  start(time(0)-3600*24), span(3600*24), step(1), dragging(false),
  font(KGlobalSettings::generalFont()), 
  small_font(KGlobalSettings::smallestReadableFont()),
  color_major(140, 115, 60), color_minor(80, 65, 34), 
  color_graph_bg(0, 0, 0),
  //color_major(255, 180, 180), color_minor(220, 220, 220), 
  //color_graph_bg(255, 255, 255),
  //color_minmax(180, 255, 180, 200), color_line(0, 170, 0),
  autoUpdateTimer(-1)
{
  setFrameStyle(QFrame::StyledPanel|QFrame::Plain);
  setMinimumWidth(300);
  setMinimumHeight(150);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setAcceptDrops(true);
  
  // setup color-tables
  for (int i=0; i<8; ++i) {
    color_line[i].setRgb(colortable[i][0], colortable[i][1], colortable[i][2]);
    color_minmax[i].setRgb(0.5*colortable[i][0], 0.5*colortable[i][1], 
	  0.5*colortable[i][2], 160);
  }

  // idiotic case-differentiation on point or pixelsize necessary :(
  if (header_font.pixelSize() == -1) {
    header_font.setPointSizeF(header_font_mag * header_font.pointSizeF());
  } else {
    header_font.setPixelSize(header_font_mag * header_font.pixelSize());
  }
 
  struct timezone tz;
  struct timeval tv;
  gettimeofday(&tv, &tz);
  tz_off = tz.tz_minuteswest * 60;
}

/** 
 * get average, min and max data
 *
 * set start end to the values get_rrd_data returns
 * don't change span, because it can shrink
 */
bool Graph::fetchAllData ()
{
  if (data_is_valid)
    return (true);
  
  if (empty())
    return (false);

  for(graph_list::iterator i = begin(); i != end(); ++i) {
    for(GraphInfo::iterator j = i->begin(); j != i->end(); ++j) {
      const std::string file(j->rrd.toUtf8().data());
      const std::string ds(j->ds.toUtf8().data());    
      
      data_start = start;
      data_end = start + span;
      step = 1;
      get_rrd_data (file, ds, &data_start, &data_end, &step, 
	    "MIN", &j->min_data);
      
      data_start = start;
      data_end = start + span;
      step = 1;
      get_rrd_data (file, ds, &data_start, &data_end, &step, 
	    "MAX", &j->max_data);
      
      data_start = start;
      data_end = start + span;
      step = 1;
      get_rrd_data (file, ds, &data_start, &data_end, &step,
	    "AVERAGE", &j->avg_data);
    }    
  }
  data_is_valid = true;

  return (true);
}

/**
 * 
 */
void Graph::clear()
{
  glist.clear();
  data_is_valid = false;
  layout();
  update();
}

int Graph::calcLegendHeights(int box_size, int width)
{
  const QFontMetrics &fontmetric = fontMetrics();

  int total_legend_height = 0;
  for(graph_list::iterator i = begin(); i != end(); ++i) {

    if (i->empty()) {
      i->legend_lines(0);
      continue;
    }

    std::vector<int> label_width;
    for(GraphInfo::const_iterator gi = i->begin(); gi != i->end(); ++gi)
      label_width.push_back(fontmetric.width(gi->label) + box_size + marg);
    
    const int n = label_width.size();
    int r;
    for(r = 1; r < n; ++r) {
      const int col = (n+r-1)/r;
      int total_width = 0, i=0;
      for (int j=0; j<col; ++j) {
	int max = 0;
	for(int k=0; k<r && i<n; ++i, ++k) {
	  if (max < label_width[i]) 
	    max = label_width[i];
	}
	total_width += max;
      }
      total_width += 4*marg * (col-1);
      if (total_width <= width)
	break;
    }
    i->legend_lines(r);
    total_legend_height += r * fontmetric.lineSpacing();
  }
  return total_legend_height;
}

void Graph::drawLegend(QPainter &paint, int left, int y, 
      int box_size, const GraphInfo &ginfo)
{
  const QFontMetrics &fontmetric = fontMetrics();

  int n = 0, cy = y, cx = left, max_width = 0;
  int lines = ginfo.legend_lines();
  if (lines) {
    for(GraphInfo::const_iterator i = ginfo.begin(); i != ginfo.end(); ++i) {
      paint.drawRect(cx, cy-box_size, box_size-1, box_size-1);
      paint.fillRect(cx+1, cy-box_size+1, box_size-2, box_size-2, 
	    color_line[n % 8]);
      paint.drawText(cx + box_size + marg, cy, i->label);
      
      int w = box_size + marg + fontmetric.width(i->label);
      if (w>max_width) 
	max_width = w;
      cy += fontmetric.lineSpacing();
      ++n;
      if (n % lines == 0) {
	cx += max_width + 4*marg;
	max_width = 0;
	cy = y;
      }
    }
  }
}

void Graph::drawHeader(QPainter &paint)
{
  paint.save();
  paint.setFont(header_font);

  const QFontMetrics &fontmetric = paint.fontMetrics();

  QString format;
  time_t time_span = data_end - data_start;
  if (time_span > 3600*24*356)
    format = i18n("%Y-%m");
  else if (time_span > 3600*24*31)
    format = i18n("%A %Y-%m-%d");
  else if (time_span > 3600*24)
    format = i18n("%A %Y-%m-%d %H:%M");
  else
    format = i18n("%A %Y-%m-%d %H:%M:%S");

  QString buffer_from = Qstrftime(format.toAscii(), localtime(&data_start));
  QString buffer_to = Qstrftime(format.toAscii(), localtime(&data_end));
  QString label = QString(i18n("from %1 to %2"))
    .arg(buffer_from) .arg(buffer_to);
  int x = (contentsRect().left()+contentsRect().right())/2
    - fontmetric.width(label)/2;
  int y = fontmetric.ascent() + marg;
  paint.drawText(x, y, label);

  paint.restore();
}

void Graph::drawFooter(QPainter &paint, int left, int right)
{
  paint.save();
  paint.setFont(header_font);

  const QFontMetrics &fontmetric = paint.fontMetrics();

  QString format;
  time_t time_span = data_end - data_start;
  if (time_span > 3600*24*356)
    format = i18n("%Y-%m");
  else if (time_span > 3600*24*31)
    format = i18n("%A %Y-%m-%d");
  else if (time_span > 3600*24)
    format = i18n("%A %Y-%m-%d %H:%M");
  else
    format = i18n("%A %Y-%m-%d %H:%M:%S");

  QString buffer_from = Qstrftime(format.toAscii(), localtime(&data_start));
  QString buffer_to = Qstrftime(format.toAscii(), localtime(&data_end));
  QString label = QString(i18n("from %1 to %2"))
    .arg(buffer_from)
    .arg(buffer_to);
  
  fontmetric.width(label);
  paint.drawText((left+right)/2-fontmetric.width(label)/2, 
	label_y2, label);

  paint.restore();
}

void Graph::drawXLines(QPainter &paint, const QRect &rect, 
      time_iterator i, QColor color)
{
  if (!i.valid()) return;
  
  // setting up linear mappings
  const linMap xmap(data_start, rect.left(), data_end, rect.right());
 
  // if lines are to close draw nothing
  if (i.interval() * xmap.m() < 3) 
    return;

  // draw lines
  paint.save();
  paint.setPen(color);
  for(; *i <= data_end; ++i) {
    int x = xmap(*i);
    paint.drawLine(x, rect.top(), x, rect.bottom());
  }
  paint.restore();
}

void Graph::drawXLabel(QPainter &paint, int y, int left, int right, 
      time_iterator i, QString format, bool center)
{
  if (!i.valid()) return;
  paint.save();
  paint.setFont(small_font);

  // setting up linear mappings
  const linMap xmap(data_start, left, data_end, right);

  // draw labels
  paint.setPen(palette().windowText().color());
  if (center) --i;
  for(; *i <= data_end; ++i) {
    // special handling for localtime/mktime on DST
    time_t t = center ? *i + i.interval() / 2 : *i;
    tm tm;
    localtime_r(&t, &tm);
    QString label = Qstrftime(i18n(format.toAscii()).toAscii(), &tm);
    if(! label.isNull()) {
      const int width = paint.fontMetrics().width(label);
      int x = center 
	? xmap(*i + i.interval() / 2) - width / 2
	: xmap(*i) - width / 2;
	  
      if (x > left && x + width < right)
	paint.drawText(x, y, label);
    }
  }
  paint.restore();
}

void Graph::findXGrid(int width, QString &format, bool &center,
      time_iterator &minor_x, time_iterator &major_x, time_iterator &label_x)
{
  const time_t min = 60;
  const time_t hour = 3600;
  const time_t day = 24*hour;
  const time_t week = 7*day;
  const time_t month = 31*day;
  const time_t year = 365*day;
  
  enum bla { align_tzalign, align_week, align_month };
  struct {
    time_t maxspan;
    time_t major;
    time_t minor;
    const char *format;
    bool center;
    bla align;
  } axis_params[] = {
    {   day,   1*min,     10, "%H:%M",              false, align_tzalign },
    {   day,   2*min,     30, "%H:%M",              false, align_tzalign },
    {   day,   5*min,    min, "%H:%M",              false, align_tzalign },
    {   day,  10*min,    min, "%H:%M",              false, align_tzalign },
    {   day,  30*min, 10*min, "%H:%M",              false, align_tzalign },
    {   day,    hour, 10*min, "%H:%M",              false, align_tzalign },
    {   day,  2*hour, 30*min, "%H:%M",              false, align_tzalign },
    {   day,  3*hour,   hour, "%H:%M",              false, align_tzalign },
    {   day,  6*hour,   hour, "%H:%M",              false, align_tzalign },
    {   day, 12*hour, 3*hour, "%H:%M",              false, align_tzalign },
    {  week, 12*hour, 3*hour, "%a %H:%M",           false, align_tzalign },
    {  week,     day, 3*hour, "%a",                 true,  align_tzalign },
    {  week,   2*day, 6*hour, "%a",                 true,  align_tzalign },
    { month,     day, 6*hour, "%a %d",              true,  align_tzalign },
    { month,     day, 6*hour, "%d",                 true,  align_tzalign },
    {  year,    week,    day, I18N_NOOP("week %V"), true,  align_week    },
    {  year,   month,    day, "%b",                 true,  align_month   },
    {     0,       0,      0, 0,                    true,  align_tzalign },
  };

  const QFontMetrics fontmetric(font);
  const time_t time_span = data_end - data_start;
  const time_t now = time(0);

  for(int i=0; axis_params[i].maxspan; ++i) {
    if (time_span < axis_params[i].maxspan) {
      QString label = Qstrftime(axis_params[i].format, localtime(&now));
      if(!label.isNull()) {
	const int textwidth = fontmetric.width(label) 
	  * time_span / axis_params[i].major * 3 / 2;
	if (textwidth < width) {
	  switch(axis_params[i].align) {
	  case align_tzalign:
	    minor_x.set(data_start, axis_params[i].minor);
	    major_x.set(data_start, axis_params[i].major);
	    label_x.set(data_start, axis_params[i].major);
	    format = axis_params[i].format;
	    center = axis_params[i].center;
	    break;
	  case align_week:
	    minor_x.set(data_start, day);
	    major_x.set(data_start, 1, time_iterator::weeks);
	    label_x.set(data_start, 1, time_iterator::weeks);
	    format = axis_params[i].format;
	    center = axis_params[i].center;
	    break;
	  case align_month:
	    minor_x.set(data_start, axis_params[i].minor);
	    major_x.set(data_start, 1, time_iterator::month);
	    label_x.set(data_start, 1, time_iterator::month);
	    format = axis_params[i].format;
	    center = axis_params[i].center;
	    break;
	  }
	  return;
	}
      }
    }
  }
  QString label = Qstrftime("%Y", localtime(&now));
  if(!label.isNull()) {
    const int textwidth = fontmetric.width(label) * 3 / 2;
    // fixed-point calculation with 16 bit fraction.
    int num = (time_span * textwidth * 16) / ( year * width);
    if (num < 16 ) {
      minor_x.set(data_start, 1, time_iterator::month);
      major_x.set(data_start, 1, time_iterator::years);
      label_x.set(data_start, 1, time_iterator::years);
      format = "%Y";
      center = true;
    } else {
      minor_x.set(data_start, 1, time_iterator::years);
      major_x.set(data_start, (num+15)/16, time_iterator::years);
      label_x.set(data_start, (num+15)/16, time_iterator::years);
      format = "%Y";
      center = false;
    }
  }
}

void Graph::drawYLabel(QPainter &paint, const QRect &rect, 
      const Range &y_range, double base)
{
  // setting up linear mappings
  const linMap ymap(y_range.min(), rect.bottom(), y_range.max(), rect.top());

  const QFontMetrics fontmetric(font);

  // SI Unit for nice display
  std::string SI;
  double mag;
  si_char(y_range.max(), SI, mag);

  // make sure labels don not overlap
  const int fontheight = fontmetric.height();
  while(rect.height() < fontheight * (y_range.max()-y_range.min())/base) 
    if (rect.height() > fontheight * (y_range.max()-y_range.min())/base/2)
      base *= 2;
    else if (rect.height() > fontheight * (y_range.max()-y_range.min())/base/5)
      base *= 5;
    else
      base *= 10;

  // draw labels
  double min = ceil(y_range.min()/base)*base;
  double max = floor(y_range.max()/base)*base;
  for(double i = min; i <= max; i += base) {
    const std::string label = si_number(i, 6, SI, mag);
    const int x = rect.left() - fontmetric.width(label.c_str()) - 4;
    paint.drawText(x,  ymap(i) + fontmetric.ascent()/2, label.c_str());
  }
}

void Graph::drawYLines(QPainter &paint, const QRect &rect, 
      const Range &y_range, double base, QColor color)
{
  // setting up linear mappings
  const linMap ymap(y_range.min(), rect.bottom(), y_range.max(), rect.top());

  const QFontMetrics fontmetric(font);

  // adjust linespacing
  if (base * -ymap.m() < 5.0) {
    if (2.0 * base * -ymap.m() > 5.0) 
      base = base * 2.0;
    else 
      base = base * 5.0;
  }

  // and return if unsufficient
  if (base * -ymap.m() < 5) return;

  // draw lines
  paint.save();
  paint.setPen(color);
  double min = ceil(y_range.min()/base)*base;
  double max = y_range.max();
  for(double i = min; i < max; i += base) {
    paint.drawLine(rect.left(), ymap(i), rect.right(), ymap(i));
  }
  paint.restore();
}

/**
 * draw the graph itself
 */
void Graph::drawGraph(QPainter &paint, const QRect &rect, 
      const GraphInfo &ginfo, double min, double max)
{
  const linMap ymap(min, rect.bottom(), max, rect.top());
  // define once use many
  QPolygon points;
  
  paint.save();
  //paint.setRenderHint(QPainter::Antialiasing);

  // draw all min/max backshadows
  int color_nr = 0;
  for(GraphInfo::const_iterator gi = ginfo.begin(); gi != ginfo.end(); ++gi) {
    const std::vector<double> &min_data = gi->min_data;
    const std::vector<double> &max_data = gi->max_data;

    if (min_data.empty() || max_data.empty())
      continue;
    const int size = gi->min_data.size();

    // setting up linear mappings
    const linMap xmap(0, rect.left(), size-1, rect.right());
   
    if (!min_data.empty() && !max_data.empty()) {  
      paint.setPen(Qt::NoPen);
      paint.setBrush(QBrush(color_minmax[color_nr++ % 8]));
      for(int i=0; i<size; ++i) {
	while (i<size && (isnan(min_data[i]) || isnan(max_data[i]))) ++i;
	int l = i;
	while (i<size && !isnan(min_data[i]) && !isnan(max_data[i])) ++i;
	const int asize = i-l;
	points.resize(asize*2);
	int k;
	for(k=0; k<asize; ++k, ++l) {
	  points.setPoint(k, xmap(l), ymap(min_data[l]));
	}
	--l;
	for(; k<2*asize; ++k, --l) {
	  points.setPoint(k, xmap(l), ymap(max_data[l]));
	}
	paint.drawPolygon(points);
      }
    }
  }

  // draw all averages
  color_nr = 0;
  for(GraphInfo::const_iterator gi = ginfo.begin(); gi != ginfo.end(); ++gi) {
    const std::vector<double> &avg_data = gi->avg_data;

    if (avg_data.empty()) continue;
    const int size = avg_data.size();
    
    // setting up linear mappings
    const linMap xmap(0, rect.left(), size-1, rect.right());
   
    // draw ing
    if (!avg_data.empty()) {
      paint.setPen(color_line[color_nr++ % 8]);
      for(int i=0; i<size; ++i) {
	while (i<size && isnan(avg_data[i])) ++i;
	int l = i;
	while (i<size && !isnan(avg_data[i])) ++i;
	const int asize = i-l;
	points.resize(asize);
	for(int k=0; k<asize; ++k, ++l) {
	  points.setPoint(k, xmap(l), ymap(avg_data[l]));
	}
	paint.drawPolyline(points);
      }
    }
  }
  paint.restore();
}

/**
 * draw the widgets contents.
 *
 * this covers a grid, the graph istself, x- and y-label and a header
 */
void Graph::drawAll()
{
  const int numgraphs =  glist.size();

  if (numgraphs) {
    if (!data_is_valid)
      fetchAllData ();

    // clear
    QPainter paint(&offscreen);
    paint.setFont(font);
    paint.eraseRect(0, 0, contentsRect().width(), contentsRect().height());
    //paint.fillRect(0, 0, contentsRect().width(), contentsRect().height(), QColor(245, 245, 245));
    
    // margin calculations
    // place for labels at the left and two line labels below
    const QFontMetrics &fontmetric = paint.fontMetrics();
    const QFontMetrics smallmetric = QFontMetrics(small_font);
    const QFontMetrics headermetric = QFontMetrics(header_font);

    time_iterator minor_x, major_x, label_x;
    QString format_x;
    bool center_x;
    findXGrid(graph_rect.width(), format_x, center_x, minor_x, major_x, label_x);
    drawHeader(paint);
    
    int n = 0;
    for(graph_list::iterator i = begin(); i != end(); ++n, ++i) {
      const int top = i->top() - contentsRect().top();
      const int bottom = i->bottom() - contentsRect().top();

      // y-scaling
      double base;
      Range y_range = i->minmax_adj(&base);
      if (!y_range.isValid())
	continue;
      
      // geometry
      const int xlabel_base = bottom + marg + smallmetric.ascent();
      const int legend_base = top + graph_height + fontmetric.ascent();
      
      // panel area
      QRect panelrect(graph_rect.left(), top, graph_rect.width(), bottom-top);
      
      // graph-background
      paint.fillRect(panelrect, color_graph_bg);
      
      // draw minor, major, graph
      drawXLabel(paint, xlabel_base, graph_rect.left(), graph_rect.right(), 
	    label_x, format_x, center_x);
      drawXLines(paint, panelrect, minor_x, color_minor);
      drawYLines(paint, panelrect, y_range, base/10, color_minor);
      drawXLines(paint, panelrect, major_x, color_major);
      drawYLines(paint, panelrect, y_range, base, color_major);
      drawYLabel(paint, panelrect, y_range, base);
      drawGraph(paint, panelrect, *i, y_range.min(), y_range.max());
      drawLegend(paint, marg, legend_base, box_size, *i);
    }
    paint.end();
    // copy to screen
    QPainter(this).drawPixmap(contentsRect(), offscreen);
  } else {
    QPainter paint(this);
    paint.eraseRect(contentsRect());
    const QString label(i18n("Drop sensors from list here"));
    const int labelwidth = paint.fontMetrics().width(label);
    paint.drawText((width()-labelwidth)/2, height()/2, label);
    paint.end();
  }
}

void Graph::layout() 
{
  const int numgraphs =  glist.size();
  if (!numgraphs) return;

  // resize offscreen-map to widget-size
  offscreen = QPixmap(contentsRect().width(), contentsRect().height());

  QPainter paint(&offscreen);
  paint.setFont(font);

  // margin calculations
  // place for labels at the left and two line labels below
  const QFontMetrics fontmetric = QFontMetrics(font);
  const QFontMetrics smallmetric = QFontMetrics(small_font);
  const QFontMetrics headermetric = QFontMetrics(header_font);

  const int labelwidth = fontmetric.boundingRect("888.888 M").width();
  box_size = fontmetric.ascent()-2;

  // area for graphs (including legends)
  graph_rect.setRect(labelwidth + marg, headermetric.height() + 2*marg,
	offscreen.width() - labelwidth - marg,
	offscreen.height() - headermetric.height() - 2*marg);
    
  const int total_legend_height = 
    calcLegendHeights(box_size, offscreen.width() - 2*marg);
  graph_height = (graph_rect.height() - total_legend_height 
	- marg*(2*numgraphs-1)) / numgraphs;

  int top = graph_rect.top();
  for(graph_list::iterator i = begin(); i != end(); ++i) {
    int bottom = top + graph_height - marg - smallmetric.lineSpacing();
    i->top(top + contentsRect().top());
    i->bottom(bottom + contentsRect().top());
    top += graph_height + i->legend_lines()*fontmetric.lineSpacing() + 2*marg; 
  }
}

/**
 * Qt (re)paint event
 */
void Graph::paintEvent(QPaintEvent *e)
{
  QFrame::paintEvent(e);
  drawAll();
}

void Graph::resizeEvent(QResizeEvent *e) 
{
  layout();
}

Graph::graph_list::iterator Graph::graphAt(const QPoint &pos)
{
  for(graph_list::iterator i = begin(); i != end(); ++i) {
    if (i->top() < pos.y() && i->bottom() > pos.y())
      return i;
  }
  return end();
}

Graph::graph_list::const_iterator Graph::graphAt(const QPoint &pos) const
{
  for(graph_list::const_iterator i = begin(); i != end(); ++i) {
    if (i->top() < pos.y() && i->bottom() > pos.y())
      return i;
  }
  return end();
}

/**
 * switch auto-update on or off
 * 
 * Auto-update does a update every 10 sec.
 */
void Graph::autoUpdate(bool active)
{
  if (active == true) {
    if (autoUpdateTimer == -1) {
      autoUpdateTimer = startTimer(10000);
      timer_diff = 0.99 * span;
      start = time(0) - timer_diff;
      data_is_valid = false;
      update();
    }
  } else {
    if (autoUpdateTimer != -1) {
      killTimer(autoUpdateTimer);
      autoUpdateTimer = -1;
    }
  }
}

/**
 *
 */
void Graph::removeGraph()
{
  const int numgraphs =  glist.size();
  if (!numgraphs) return;

  graph_list::iterator target = graphAt(QPoint(0, origin_y));
  if (target != end()) {
    glist.erase(target);
    changed(true);
  }
  layout();
  update();
}

/**
 *
 */
void Graph::splitGraph()
{
  add();
  layout();
  update();
}


/**
 * Qt mouse-press-event
 */
void Graph::mousePressEvent(QMouseEvent *e)
{
  origin_x = e->x ();
  origin_y = e->y ();

  origin_start = data_start;
  origin_end = data_end;

  // context-menu
  if (e->button() == Qt::RightButton) {
    // map for delete-datasource-options
    typedef std::map<QAction *, GraphInfo::iterator> actionmap;
    actionmap acts; 
    
    // context-menu
    KMenu menu(this);
    
    graph_list::iterator s_graph = graphAt(e->pos());
    
    menu.addAction(KIcon("list-add"), 
	  i18n("add new subgraph"), this, SLOT(splitGraph()));

    if (s_graph != end()) {
      menu.addAction(KIcon("edit-delete"),
	    i18n("delete this subgraph"), this, SLOT(removeGraph()));
      menu.addSeparator();
      
      // generate entries to remove datasources
      for(GraphInfo::iterator i = s_graph->begin(); i != s_graph->end(); ++i) {
	QAction *T = menu.addAction(KIcon("list-remove"),
	      i18n("remove ") + i->label);
	acts[T] = i;
      }
    }

    QAction *action = menu.exec(e->globalPos());

    actionmap::iterator result = acts.find(action);
    if (result != acts.end()) {
      s_graph->erase(result->second);
      layout();
      update();
    }
  }
}

/**
 * Qt mouse-event
 *
 * handle dragging of mouse in graph
 */
void Graph::mouseMoveEvent(QMouseEvent *e)
{
  if (e->buttons() == Qt::LeftButton) {
    if (autoUpdateTimer != -1) 
      return;
    
    int x = e->x();
    int y = e->y();
    
    if ((x < graph_rect.left()) || (x >= graph_rect.right()))
      return;
    if ((y < 0) || (y >= height()))
      return;
    
    int offset = (x - origin_x) * (origin_end - origin_start) 
      / graph_rect.width();
    
    start = origin_start - offset;
    const time_t now = time(0);
    if (start + span > now + span * 2 / 3 )
      start = now - span / 3;
    
    if (autoUpdateTimer != -1) 
      timer_diff = time(0) - start;
    
    data_is_valid = false;
    update();
  } else  if (e->buttons() == Qt::MidButton){
    dragging = true;
    update();
  } else {
    e->ignore();
  }
}

/**
 *
 */
void Graph::wheelEvent(QWheelEvent *e)
{
  if (e->delta() < 0)
    zoom(1.259921050);
  else
    zoom(1.0/1.259921050);
}


/**
 *
 */
void Graph::timerEvent(QTimerEvent *event)
{
  data_is_valid = false;
  start = time(0) - timer_diff;
  update();
}

/**
 *
 */

void Graph::dragEnterEvent(QDragEnterEvent *event)
{
  const GraphMimeData *mimeData = 
    qobject_cast<const GraphMimeData *>(event->mimeData());

  if (mimeData)
    event->acceptProposedAction();
  else
    event->ignore();
}

/**
 *hermione•load•load•longterm
 */
void Graph::dragMoveEvent(QDragMoveEvent *event)
{
  const GraphMimeData *mimeData = 
    qobject_cast<const GraphMimeData *>(event->mimeData());
  if (!mimeData) return;

  if(glist.size() == 0) {
    event->accept(contentsRect());
  } else {
    graph_list::iterator i  = graphAt(event->pos());
    if(i != end()) {
      QRect g;
      g.setCoords(contentsRect().left(), i->top(), 
	    contentsRect().right(), i->bottom());
      event->accept(g);
    } else {
      event->ignore();
    }
  }
}

/**
 *
 */
void Graph::dropEvent(QDropEvent *event)
{
  const GraphMimeData *mimeData = 
    qobject_cast<const GraphMimeData *>(event->mimeData());
  if (!mimeData) return;

  event->acceptProposedAction();
  const int numgraphs =  glist.size();
  if (numgraphs) {
    graph_list::iterator target = graphAt(event->pos());
    if (target != end()) {
      target->add(mimeData->rrd(), mimeData->ds(), mimeData->label());
    } else {
      add(mimeData->rrd(), mimeData->ds(), mimeData->label());
    }
  } else {
    add(mimeData->rrd(), mimeData->ds(), mimeData->label());
  }
  data_is_valid = false;
  layout();
  update();
}

/**
 * set the graph to display the last new_span seconds
 */
void Graph::last(time_t new_span)
{
  span = new_span;
  
  if (autoUpdateTimer != -1) {
    timer_diff = 0.99 * span;
    start = time(0) - timer_diff;
  } else {
    start = time(0) - 0.99 * span;
  }
  data_is_valid = false;
  update();
}

/**
 * zoom graph with factor
 */
void Graph::zoom(double factor)
{
  // don't zoom to wide
  if (factor < 1 && span*factor < width()) return;

  time_t time_center = data_end - span / 2;  
  if (time_center < 0) return;
  span *= factor;
  start = time_center - (span / 2);

  const time_t now = time(0);
  if (start + span > now + span * 2 / 3 )
    start = now - span / 3;

  if (autoUpdateTimer != -1) {
    timer_diff = 0.99 * span;
    start = time(0) - timer_diff;
  }
  
  data_is_valid = false;
  update();
}

/**
 * returns range for y-values
 */
Range GraphInfo::minmax()
{
  Range r;
  for(const_iterator i = begin(); i != end(); ++i) {
    Range a = ds_minmax(i->avg_data, i->min_data, i->max_data);
    if (a.isValid()) {
      if (r.isValid())
	r = range_max(r, a);
      else
	r = a;
    }
  }
  return r;
}

/**
 * returns adjusted range for y-values
 */
Range GraphInfo::minmax_adj(double *base)
{
  Range y = minmax();
  return range_adj(y, base);
}
