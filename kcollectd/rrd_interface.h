/* -*- c++ -*-
 *
 * (C) 2008 M G Berberich
 *
 */
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

#ifndef RRD_INTERAFCE_H
#define RRD_INTERAFCE_H

#include <string>
#include <vector>
#include <set>

void get_dsinfo(const std::string &rrdfile, std::set<std::string> &list);

void get_rrd_data (const std::string &file, const std::string &ds, 
      time_t *start, time_t *end, unsigned long *step, const char *type, 
      std::vector<double> *result);

#endif
