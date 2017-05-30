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

#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <limits>

#include <qstring.h>

#include "misc.h"


/**
 * determine the apropriate SI-prefix for a number
 */
bool si_char(double d, std::string &s, double &m)
{
  const struct {
    double factor;
    const char * const si_char;
  } si_table[] = {
    { 1e-24, "y" },
    { 1e-21, "z" },
    { 1e-18, "a" },
    { 1e-15, "f" },
    { 1e-12, "p" },
    { 1e-9,  "n" },
    { 1e-6,  "µ" },
    { 1e-3,  "m" },
    { 1,     ""  },
    { 1e3,   "k" },
    { 1e6,   "M" },
    { 1e9,   "G" },
    { 1e12,  "T" },
    { 1e15,  "P" },
    { 1e18,  "E" },
    { 1e21,  "Z" },
    { 1e24,  "Y" },
  };
  const int tablesize = sizeof(si_table)/sizeof(*si_table);

  int i;
  for(i=0; i < tablesize; ++i) {
    if (d < si_table[i].factor) break;
  }
  if (i == 0 || i == tablesize) {
    m = 1.0;
    s = "";
    return false;
  } else {
    --i;
    m = si_table[i].factor;
    s = si_table[i].si_char;
    return true;
  }
}

/**
 * formats a number with prefix s and magnitude m, precission p
 */
std::string si_number(double d, int p, const std::string &s, double m)
{
  std::ostringstream os;
  os << std::setprecision(p) << d/m;
  if (!s.empty())
    os << " " << s;
  return os.str();
}

/**
 * some kind of strftime that returns a QString and handles locale
 * !! strftime already is localized
 */
QString Qstrftime(const char *format, const tm *t)
{
  char buffer[50];
  if(strftime(buffer, sizeof(buffer), format, t))
    return QString::fromLocal8Bit(buffer);
  else
    return QString();
}

/**
 * determine min and max values for a graph and save it into y_range
 */

Range ds_minmax(const std::vector<double> &avg_data, 
      const std::vector<double> &min_data,
      const std::vector<double> &max_data)
{
  const std::size_t size = avg_data.size();
  // all three datasources must be of equal length
  if (size != min_data.size() || size != max_data.size())
    return Range();

  bool valid = false;
  double min(std::numeric_limits<double>::max());
  double max(std::numeric_limits<double>::min());

  // process avg_data
  if (!avg_data.empty()) {
    for(std::size_t i=0; i<size; ++i) {
      if (isnan(avg_data[i])) continue;
      valid = true;
      if (min > avg_data[i]) min = avg_data[i];
      if (max < avg_data[i]) max = avg_data[i];
    }
  }

  // process min/max-data
  if (!min_data.empty() && !max_data.empty()) {  
    for(std::size_t i=0; i<size; ++i) {
      if (isnan(min_data[i]) || isnan(max_data[i])) continue;
      valid = true;
      if (min > min_data[i]) min = min_data[i];
      if (max < min_data[i]) max = min_data[i];
      if (min > max_data[i]) min = max_data[i];
      if (max < max_data[i]) max = max_data[i];
    }
  }
  
  // no data found at all
  if(!valid) return Range();

  return Range(min, max);
}

/**
 * determine min and max values for a graph and base
 * and do some adjustment
 */

Range range_adj(const Range &y_range, double *base)
{
  if(!y_range.isValid()) 
    return Range();
  
  double min(y_range.min()), max(y_range.max());
  double tmp_base = 1.0;

  if (y_range.min() == y_range.max()) {
    max += 1;
    min -= 1;
  } else {
    // allign to sensible values
    tmp_base = pow(10, floor(log(max-min)/log(10)));
    min = floor(min/tmp_base)*tmp_base;
    max = ceil(max/tmp_base)*tmp_base;
    
    // setting some margin at top and bottom
    const double margin = 0.05 * (max-min);
    max += margin;
    min -= margin;
  }

  if (base) *base = tmp_base;
  return Range(min, max);
}

/**
 * merges two ranges @a a and @a b into a range, so that the new range
 * encloses @a a and @a b.
 */
Range range_max(const Range &a, const Range &b)
{
  if (!a.isValid() || !b.isValid())
    return Range();

  Range r;
  r.min(fmin(a.min(), b.min()));
  r.max(fmax(a.max(), b.max()));
  return r;
}

// definition of NaN in Range
const double Range::NaN = std::numeric_limits<double>::quiet_NaN();

