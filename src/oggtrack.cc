/*
*
* orpheus ogg vorbis track class implementation
* $Id: oggtrack.cc,v 1.2 2006/05/13 09:14:28 konst Exp $
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

#include "oggtrack.h"
#include "orpheusconf.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>

#ifdef HAVE_LIBVORBISFILE

#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>

#endif

#include <fstream>

#include "kkstrtext.h"

#include "abstract/userinterface.h"
#include "vcedit.h"

static int cpid = -1;
static FILE *fpread, *fpwrite;

oggtrack::oggtrack(const string &afname)
: idtrack(), paused(false), length(60) {
    int npos;

    fname = afname;
    readtag();

    if(title.empty()) {
	if((npos = fname.find(".")) != -1)
	    title = fname.substr(0, npos);

	if((npos = title.find_last_of("/")) != -1)
	    title.erase(0, npos+1);
    }
}

oggtrack::~oggtrack() {
}

void oggtrack::play() {
    runplayer();
    paused = false;
    currentpos = 0;
}

void oggtrack::pause() {
    if(cpid != 1) {
	paused = !paused;
	if(paused) kill(-cpid, SIGSTOP);
	    else kill(-cpid, SIGCONT);
    }
}

void oggtrack::stop() {
    kill(-cpid, SIGCONT);
    kill(-cpid, SIGKILL);
}

bool oggtrack::terminated() const {
    return cpid == -1;
}

int oggtrack::getlength() const {
    return length;
}

string oggtrack::getdescription() const {
    string r;

    r = (string) "(ogg) " + artist;
    if(!artist.empty()) r += ": ";
    r += title;

    return r;
}

vector<string> oggtrack::getstatus() const {
    vector<string> r;
    string desc, sbuf, cmd, tm;
    char cbuf[4096], c;
    int rc;

    // Time: 00:03.23 [01:44.77] of 01:48.00  ( 39.7 kbps)  Output Buffer  87.5%

    if(cpid > 0) {
	fd_set fds;
	struct timeval tv;

	bool term = false;

	FD_ZERO(&fds);
	FD_SET(fileno(fpread), &fds);
	tv.tv_sec = tv.tv_usec = 0;
	select(fileno(fpread)+1, &fds, 0, 0, &tv);

	if(FD_ISSET(fileno(fpread), &fds)) {
	    sbuf = "";

	    rc = read(fileno(fpread), cbuf, sizeof(cbuf)-1);
	    term = !rc;

	    if(rc > 0) {
		cbuf[rc] = 0;
		sbuffer += cbuf;
	    }

	    while(sbuffer.find("\r") != -1) {
		sbuf = getword(sbuffer, "\r");
		cmd = getword(sbuf);

		if(cmd == "Time:") {
		    tm = getword(sbuf);
		    currentpos = atoi(tm.substr(0, 2).c_str())*60 +
			atoi(tm.substr(3, 2).c_str());

		    getword(sbuf);
		    getword(sbuf);

		    tm = getword(sbuf);
		    length = atoi(tm.substr(0, 2).c_str())*60 +
			atoi(tm.substr(3, 2).c_str());

		}
	    }

	} else {
	    term = kill(cpid, 0);
	}

	if(term) {
	    cpid = -1;
	    fclose(fpread);
	    fclose(fpwrite);
	    ui.status(_("The ogg player application has terminated"));
	}
    }

    sprintf(cbuf, _("OGG: %s [%02d:%02d]"),
	justfname(fname).c_str(),
	(int) currentpos/60,
	(int) currentpos-(currentpos/60)*60);

    r.push_back(cbuf);

    if(!album.empty()) {
	desc = album;
	if(!year.empty()) desc += " (" + year + ")";
	if(!comment.empty()) desc += ", " + comment;
	r.push_back(desc);
    }

    r.push_back(title);

    return r;
}

void oggtrack::toplaylist(ofstream &f) const {
    f << fname << endl;
}

void oggtrack::readtag() {
    struct stat st;
    if(!stat(fname.c_str(), &st))
	lastmod = st.st_mtime;

#ifdef HAVE_LIBVORBISFILE
    OggVorbis_File ov;
    FILE *f = fopen(fname.c_str(), "r");
    char *p;

    if(f) {
	memset(&ov, 0, sizeof(ov));

	if(!ov_open(f, &ov, 0, 0)) {
	    vorbis_info *vi = ov_info(&ov, -1);
	    length = (int) ov_time_total(&ov, -1);
	    bitrate = vi->rate;

	    vorbis_comment *vc = ov_comment(&ov, -1);
	    p = vorbis_comment_query(vc, "artist", 0); if(p) artist = p;
	    p = vorbis_comment_query(vc, "title", 0); if(p) title = p;
	    p = vorbis_comment_query(vc, "album", 0); if(p) album = p;
	    p = vorbis_comment_query(vc, "year", 0); if(p) year = p;
	    p = vorbis_comment_query(vc, "comment", 0); if(p) comment = p;

	    if(conf.getrussian()) {
		artist = siconv(artist, "utf-8", "koi8-u");
		title = siconv(title, "utf-8", "koi8-u");
		album = siconv(album, "utf-8", "koi8-u");
		year = siconv(year, "utf-8", "koi8-u");
		comment = siconv(comment, "utf-8", "koi8-u");
	    }

	    ov_clear(&ov);
	}
    }
#endif
}

// ----------------------------------------------------------------------------

void oggtrack::runplayer() {
    int inpipe[2], outpipe[2];

    sbuffer = "";

    if(!pipe(inpipe) && !pipe(outpipe)) {
	if(!(cpid = fork())) {
	    dup2(inpipe[1], STDOUT_FILENO);
	    dup2(inpipe[1], STDERR_FILENO);
	    dup2(outpipe[0], STDIN_FILENO);

	    close(inpipe[0]);
	    close(inpipe[1]);
	    close(outpipe[0]);
	    close(outpipe[1]);

	    setpgid(0, 0);
	    execl("/bin/sh", "/bin/sh", "-c", (conf.getoggplayer() + " '" + fname + "'").c_str(), 0);

	    _exit(0);
	} else {
	    fpread = fdopen(inpipe[0], "r");
	    fpwrite = fdopen(outpipe[1], "w");

	    setvbuf(fpread, 0, _IONBF, 0);

	    sleep(1);
	    waitpid(cpid, 0, WNOHANG);

	    if(::kill(cpid, 0)) {   // The PID doesn't exist anymore
		cpid = -1;
		ui.status(_("Cannot run the OGG player program"));
	    } else {
		ui.status(_("The OGG player program has been launched"));
	    }
	}
    }
}

void oggtrack::setidtags(string &aartist, string &atitle, string &aalbum, string &ayear, string &acomment) {
#ifdef HAVE_LIBVORBIS
    FILE *inpf, *outf;
    string tfname;
    int i;
    bool r = false;

    if(inpf = fopen(fname.c_str(), "rb")) {
	vcedit_state* state = vcedit_new_state();

	if(!vcedit_open(state, inpf)) {
	    i = getpid();

	    do tfname = fname + "_temp_" + i2str(i++);
	    while(!access(tfname.c_str(), F_OK));

	    if(outf = fopen(tfname.c_str(), "w+")) {
		vorbis_comment* vc = vcedit_comments(state);
		vorbis_comment_clear(vc);
		vorbis_comment_init(vc);

		if(!atitle.empty())
		    vorbis_comment_add(vc, (char *) (string("title=") + atitle).c_str());

		if(!aartist.empty())
		    vorbis_comment_add(vc, (char *) (string("artist=") + aartist).c_str());

		if(!aalbum.empty())
		    vorbis_comment_add(vc, (char *) (string("album=") + aalbum).c_str());

		if(!ayear.empty())
		    vorbis_comment_add(vc, (char *) (string("year=") + ayear).c_str());

		if(!acomment.empty())
		    vorbis_comment_add(vc, (char *) (string("comment=") + acomment).c_str());

		r = !vcedit_write(state, outf);
		fclose(outf);
	    }
	}

	fclose(inpf);
    }

    if(r) {
	rename(tfname.c_str(), fname.c_str());
	artist = aartist;
	title = atitle;
	album = aalbum;
	year = ayear;
	comment = acomment;
    } else {
	unlink(tfname.c_str());
	ui.statusf("Coudln't update the OGG ID tags");
    }
#endif
}
