
cmake 

Install Prefix:
  -DCMAKE_INSTALL_PREFIX=[prefix]

Debug/Release:
  -DCMAKE_BUILD_TYPE=[DEBUG|Release]

RRD-Basedir:
  -DRRD_BASEDIR=[basedir]
  default is /var/lib/collectd/rrd

rpath:
  set
  -DCMAKE_SKIP_RPATH=true
  if you do not want to have an rpath in kcollectd.
