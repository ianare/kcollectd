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

#include <string>
#include <iostream>
#include <exception>

#include <boost/filesystem.hpp>

#include <QTreeWidgetItem>

#include <KAboutData>
#include <KCmdLineArgs>
#include <KApplication>
#include <KMessageBox>
#include <KLocale>

#include "../config.h"

#include "gui.h"

int main(int argc, char **argv)
{
  using namespace boost::filesystem;

  std::vector<std::string> rrds;
  KAboutData about("kcollectd", "kcollectd", 
	ki18n("KCollectd"), VERSION, 
	ki18n("Viewer for Collectd-databases"),
 	KAboutData::License_GPL,
	ki18n("© 2008, 2009 M G Berberich"),
	ki18n("Maintainer and developer"),
	"http://www.forwiss.uni-passau.de/~berberic/Linux/kcollectd.html",
	"berberic@fmi.uni-passau.de");
  about.addAuthor(ki18n("M G Berberich"), KLocalizedString(),
	"M G Berberich <berberic@fmi.uni-passau.de>", 
	"http://www.forwiss.uni-passau.de/~berberic");
  about.setTranslator(ki18nc("NAME OF TRANSLATORS", "Your names"),
	ki18nc("EMAIL OF TRANSLATORS", "Your emails"));
  KCmdLineArgs::init(argc, argv, &about);

  KCmdLineOptions options;
  options.add("+[file]", ki18n("A kcollectd-file to open"));
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication application;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();  
  try {
    if (application.isSessionRestored()) {
      kRestoreMainWindows<KCollectdGui>();
    } else {      
      KCollectdGui *gui = new KCollectdGui;
      // handling arguments
      if(args->count() == 1) gui->load(args->arg(0));
      gui->setObjectName("kcollectd#");
      gui->show();
    }
  } 
  catch(basic_filesystem_error<path> &e) {
    KMessageBox::error(0, i18n("Failed to read collectd-structure at \'%1\'\n"
		"Terminating.", QString(RRD_BASEDIR)));
    exit(1);
  }

  return application.exec();
}
