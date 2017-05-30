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

#ifndef LABELING_H
#define LABELING_H

#include <string>
#include <vector>

bool si_char(double d, std::string &s, double &m);

std::string si_number(double d, int p, const std::string &s, double m);

QString Qstrftime(const char *format, const tm *t);


/**
 * linear mapping from range [x1, x2] to range [y1, y2]
 */
class linMap {
  double m_, t_;
public:
  linMap(double x1, double y1, double x2, double y2) {
    m_ = (y2-y1)/(x2-x1);
    t_ = y1-m_*x1;
  }
  double operator()(double x) const { return m_ * x + t_; }
  double m() const { return m_; }
};


/**
 * small helper holding tow doubles e.g. a point
 */
class Range {
  double x_;
  double y_;
  static const double NaN;
public:
  Range() : x_(NaN), y_(NaN) { }
  Range(double x, double y) : x_(x), y_(y) { }
  double min() const { return x_; }
  double max() const { return y_; }
  void min(double a) { x_ = a; }
  void max(double a) { y_ = a; }
  void set(double x, double y) { x_ = x; y_ = y; }
  bool isValid() const { return x_ != NaN; }
};

Range ds_minmax(const std::vector<double> &avg_data, 
      const std::vector<double> &min_data,
      const std::vector<double> &max_data);

Range range_adj(const Range &range, double *base);

Range range_max(const Range &a, const Range &b);

#endif
