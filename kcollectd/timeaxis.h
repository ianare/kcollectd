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

#ifndef TIMEAXIS_H
#define TIMEAXIS_H

#include <time.h>
#include <sys/time.h>

#include <iostream>

class time_iterator {
 public:  
  enum it_type { seconds, weeks, month, years };

  time_iterator();
  time_iterator(time_t start, time_t step, it_type type = seconds); 
  void set(time_t start, time_t step, it_type type = seconds); 

  time_iterator &operator++();
  time_iterator &operator--();
  time_t &operator*();
  time_t *operator->();

  bool valid();
  time_t interval();
  const struct tm *tm();

 protected:
  it_type type;
  time_t now_t;
  struct tm now_tm;
  time_t step;
};

inline time_iterator::time_iterator(time_t start, time_t step, it_type type)
{
  set(start, step, type);
} 

inline time_iterator::time_iterator()
{
  set(0, 0);
} 

inline time_t &time_iterator::operator*()
{
  return now_t;
}

inline time_t *time_iterator::operator->()
{
  return &now_t;
}

inline bool time_iterator::valid()
{
  return step != 0;
}

#endif
