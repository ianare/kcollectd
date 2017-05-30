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

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cstdlib>
#include <cstring>

#include <rrd.h>
#include <errno.h>

#include "rrd_interface.h"

/**
 * wrapper for rrd_info taking a string instead of char*
 *
 * copying the filename is probably unnecessary, but the signature of
 * rrd_info does not guarantee leaving it alone.
 */
static inline 
rrd_info_t *rrd_info(int, const std::string &filename)
{
  char c_file[filename.length()+1];
  filename.copy(c_file, std::string::npos);
  c_file[filename.length()] = 0;
  char * arg[] = { 0, c_file, 0 };
  return rrd_info(2, arg);
}

/**
 * read the datasources-names of a rrd
 *
 * using rrd_info
 */
void get_dsinfo(const std::string &rrdfile, std::set<std::string> &list)
{
  using namespace std;

  list.clear();

  rrd_info_t *infos = rrd_info(2, rrdfile);
  rrd_info_t *i = infos;
  while (i) {
    string line(i->key);
    if (line.substr(0,3) == "ds[") {
      size_t e =  line.find("].type");
      if (e != string::npos) {
	string name = line.substr(3, line.find("].type")-3);
	list.insert(name);
      }
    }
    i = i->next;
  }
  if (infos) 
    rrd_info_free(infos);  
}

/**
 * gets data from a rrd
 *
 * @a start and @a end may get changed from this function and represent
 * the start and end of the data returned.
 */
void get_rrd_data (const std::string &file, const std::string &ds, 
      time_t *start, time_t *end, unsigned long *step, const char *type, 
      std::vector<double> *result)
{
  unsigned long ds_cnt = 0;
  char **ds_name;
  rrd_value_t *data;
  int status;

  result->clear();

  status = rrd_fetch_r(file.c_str(), type, 
	start, end, step, &ds_cnt, &ds_name, &data); 
  if (status != 0) {
    return;
  }

  const unsigned long length = (*end - *start) / *step;

  for(unsigned int i=0; i<ds_cnt; ++i) {
    if (ds != ds_name[i]) {
      free(ds_name[i]);
      continue;
    }
    
    for (unsigned int n = 0; n < length; ++n) 
      result->push_back (data[n * ds_cnt + i]);
    break;

    free(ds_name[i]);
  }
  free(ds_name);
  free(data);
}
