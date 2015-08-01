/*
*
* orpheus main function implementation
* $Id: main.cc,v 1.7 2004/09/27 22:17:58 konst Exp $
*
* Copyright (C) 2002-2004 by Konstantin Klyagin <konst@konst.org.ua>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
* USA
*
*/
#include "uitext.h"
#include "cdtrack.h"
#include "mp3track.h"
#include "playlist.h"

#ifdef ENABLE_NLS

#include <locale.h>
#include <libintl.h>

#endif

OrpheusUserInterface &ui = tui;

void sighandler(int sig) {
    if(sig == SIGUSR1) {
	string fname = conf.getbasedir() + "/remote", cmd, tok;
	ifstream f(fname.c_str());
	if(f.is_open()) {
	    getline(f, cmd);

	    tok = getword(cmd);

	    if(tok == "next") {
		ui.nexttrack();

	    } else if(tok == "prev") {
		ui.prevtrack();

	    } else if(tok == "add") {
		ui.doadd(cmd);
		ui.update();

	    } else if(tok == "load") {
		ui.loadplaylist(cmd);

	    } else if(tok == "play") {
		ui.play(atoi(cmd.c_str()));

	    }

	    f.close();
	    unlink(fname.c_str());
	}
    }
}

int main(int argc, char **argv) {
#ifdef ENABLE_NLS
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
#endif

    srand(time(0));
    ui.commandline(argc, argv);

    struct sigaction sact;
    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = &sighandler;
    sigaction(SIGUSR1, &sact, 0);

    ofstream f((conf.getbasedir() + "/pid").c_str());
    if(f.is_open()) {
	f << getpid();
	f.close();
    }

    return ui.exec();
}
