cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0011 OLD)

project(kcollectd)
set(VERSION 0.9)

find_package(KDE4 REQUIRED)
include (KDE4Defaults)

add_definitions(-Wall -g)

# Basedir for collectd
if(NOT RRD_BASEDIR)
  set(RRD_BASEDIR "/var/lib/collectd/rrd")
endif(NOT RRD_BASEDIR)
set(RRD_BASEDIR ${RRD_BASEDIR} CACHE PATH "path to collectd-data")

# config.h
configure_file(config.h.in config.h)

subdirs(kcollectd po doc)

