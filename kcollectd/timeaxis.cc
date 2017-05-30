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

#include "timeaxis.h"

void time_iterator::set(time_t start, time_t st, it_type ty)
{
  step = st;
  if (!step) return;

  type = ty;

  switch(type) {
  case seconds: {
    tzset();
    localtime_r(&start, &now_tm);
    now_t = ((start + now_tm.tm_gmtoff + step) / step) * step - now_tm.tm_gmtoff;
    break; }
  case weeks:
    tzset();
    localtime_r(&start, &now_tm);
    step *= 3600*24*7;
    now_tm.tm_sec = now_tm.tm_min = now_tm.tm_hour = 0;
    now_tm.tm_mday -= now_tm.tm_wday - (now_tm.tm_wday == 0 ? 1 : 8);
    now_t = mktime(&now_tm);
    break;
  case month:
    tzset();
    localtime_r(&start, &now_tm);
    now_tm.tm_sec = now_tm.tm_min = now_tm.tm_hour = 0;
    if (now_tm.tm_mon == 11) {
      now_tm.tm_mon = 0;
      ++now_tm.tm_year;
    } else {
      ++now_tm.tm_mon;
    }
    now_tm.tm_mday = 1;
    now_t = mktime(&now_tm);
    break;
  case years:
    tzset();
    localtime_r(&start, &now_tm);
    now_tm.tm_sec = now_tm.tm_min = now_tm.tm_hour = 0;
    now_tm.tm_mon = 0;
    now_tm.tm_mday = 1;
    ++now_tm.tm_year;
    now_tm.tm_isdst = 0;
    now_t = mktime(&now_tm);
    break;
  }
}


time_iterator &time_iterator::operator++()
{
  switch(type) {
  case seconds:
  case weeks:
    now_t += step;
    break;
  case month:
    // max step is 12!
    now_tm.tm_mon += step;
    if (now_tm.tm_mon >= 12) {
      now_tm.tm_mon -= 12;
      ++now_tm.tm_year;
    }
    now_t = mktime(&now_tm);
    break;
  case years:
    now_tm.tm_year += step;
    now_t = mktime(&now_tm);
    break;
  }
  return *this;
}


time_iterator &time_iterator::operator--()
{
  switch(type) {
  case seconds:
  case weeks:
    now_t -= step;
    break;
  case month:
    // max step is 12!
    now_tm.tm_mon -= step;
    if (now_tm.tm_mon < 1) {
      now_tm.tm_mon += 12;
      --now_tm.tm_year;
    }
    now_t = mktime(&now_tm);
    break;
  case years:
    now_tm.tm_year -= step;
    now_t = mktime(&now_tm);
    break;
  }
  return *this;
}

time_t time_iterator::interval()
{
  switch(type) {
  case seconds:
  case weeks:
    return step;
  case month:
    return step * 3600*24*30;
  case years:
    return step * 3600*24*365;
  }
  return 0;
}

const struct tm *time_iterator::tm()
{
  switch(type) {
  case seconds:
  case weeks:
    tzset();
    localtime_r(&now_t, &now_tm);
    break;
  case month:
  case years:
    break;
  }
  return &now_tm;
}

