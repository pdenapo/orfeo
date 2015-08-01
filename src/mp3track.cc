/*
*
* orpheus mp3 track class implementation
* $Id: mp3track.cc,v 1.21 2004/07/08 23:53:38 konst Exp $
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

#include "mp3track.h"
#include "orpheusconf.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>

#include <fstream>

#include "kkstrtext.h"

#include "abstract/userinterface.h"

#define MPGVER_25       0
#define MPGVER_RES      1
#define MPGVER_2        2
#define MPGVER_1        3

#define MPGLAYER_3      1
#define MPGLAYER_2      2
#define MPGLAYER_1      3

static int cpid = -1, trackcount = 0;
static FILE *fpread, *fpwrite;
static char cbuf[65535];

static u_int32_t extract_bitfield(unsigned char *h, int start, int end) {
    u_int32_t hdr;
    memcpy(&hdr, h, 4);
    hdr = ntohl(hdr);
    hdr = hdr << start;
    hdr = hdr >> (start + (31 - end));
    return hdr;
}

// ----------------------------------------------------------------------------

mp3track::mp3track(const string &afname)
: idtrack(), currentpos(0) {
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

mp3track::~mp3track() {
}

void mp3track::play() {
    sendcommand("LOAD %s", fname.c_str());
    aterminated = false;
    trackcount++;
}

void mp3track::pause() {
    sendcommand("PAUSE");
}

void mp3track::stop() {
    sendcommand("STOP");
}

void mp3track::fwd(bool big) {
    sendcommand(big ? "JUMP +1280" : "JUMP +128");
}

void mp3track::rwd(bool big) {
    sendcommand(big ? "JUMP -1280" : "JUMP -128");
}

bool mp3track::terminated() const {
    return aterminated;
}

int mp3track::getlength() const {
    return length;
}

string mp3track::getdescription() const {
    string r;

    r = (string) "(mp3) " + artist;
    if(!artist.empty()) r += ": ";
    r += title;

    return r;
}

vector<string> mp3track::getstatus() const {
    vector<string> r;
    string desc, sbuf, cmd;

    if(cpid > 0) {
	fd_set fds;
	struct timeval tv;

	while(1) {
	    FD_ZERO(&fds);
	    FD_SET(fileno(fpread), &fds);
	    tv.tv_sec = tv.tv_usec = 0;
	    select(fileno(fpread)+1, &fds, 0, 0, &tv);

	    if(FD_ISSET(fileno(fpread), &fds)) {
		fgets(cbuf, 63, fpread);

		sbuf = cbuf;
		cmd = getword(sbuf);

		if(cmd == "@F") {
		    getword(sbuf);
		    getword(sbuf);
		    currentpos = strtoul(sbuf.c_str(), 0, 0);

		} else if(cmd == "@P") {
		    if(!strtoul(sbuf.c_str(), 0, 0)) {
			aterminated = !(--trackcount);
		    }

		}

	    } else {
		if(::kill(cpid, 0)) {
		    cpid = -1;
		    fclose(fpread);
		    fclose(fpwrite);
		    aterminated = true;
		    ui.status(_("The mp3 player application has terminated"));
		}
		break;
	    }
	}
    }

    sprintf(cbuf, _("MP3: %s [%02d:%02d]"),
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

void mp3track::toplaylist(ofstream &f) const {
    f << fname << endl;
}

void mp3track::readtag() {
    struct stat st;
    if(!stat(fname.c_str(), &st))
	lastmod = st.st_mtime;

    ifstream f(fname.c_str());
    unsigned char *h = 0;

    /*
    *
    * Some code here was taken from the source code of mp3ai, a program
    * by Salvatore Sanfilippo <antirez@invece.org>
    *
    * http://www.hping.org/mp3ai
    *
    */

    static int bitrate_tbl[80] = {
	/* V1 L1 */
	0, 32, 64, 96, 128, 160, 192, 224,
	256, 288, 320, 352, 384, 416, 448, 0,
	/* V1 L2 */
	0, 32, 48, 56, 64, 80, 96, 112,
	128, 160, 192, 224, 256, 320, 384, 0,
	/* V1 L3 */
	0, 32, 40, 48, 56, 64, 80, 96,
	112, 128, 160, 192, 224, 256, 320, 0,
	/* V2[.5] L1 */
	0, 32, 48, 56, 64, 80, 96, 112,
	128, 144, 160, 176, 192, 224, 256, 0,
	/* V2[.5] L2 & L3 */
	0, 8, 16, 24, 32, 40, 48, 56,
	64, 80, 96, 112, 128, 144, 160, 0
    };

    static int freq_tbl[12] = {
	/* V1 */
	44100, 48000, 32000, 0,
	/* V2 */
	22050, 24000, 16000, 0,
	/* V2.5 */
	11025, 12000, 8000, 0
    };

    u_int32_t aux;
    int i, filelen;

    if(f.is_open()) {
	f.seekg(-128, ios::end);
	filelen = f.tellg();

	f.read(cbuf, 3);
	cbuf[3] = 0;

	if((string) cbuf == "TAG") {
	    f.read(cbuf, 30); cbuf[30] = 0; title = rusconv("wk", strim(cbuf));
	    f.read(cbuf, 30); cbuf[30] = 0; artist = rusconv("wk", strim(cbuf));
	    f.read(cbuf, 30); cbuf[30] = 0; album = rusconv("wk", strim(cbuf));
	    f.read(cbuf, 4); cbuf[4] = 0; year = rusconv("wk", strim(cbuf));
	    f.read(cbuf, 30); cbuf[30] = 0; comment = rusconv("wk", strim(cbuf));
	    f.read(cbuf, 1);
	}

	f.seekg(0, ios::beg);
	f.read(cbuf, sizeof(cbuf));

	for(i = 0; i < sizeof(cbuf); i++) {
	    h = (unsigned char *) (cbuf+i);
	    if(ismpegheader(h))
		break;
	}

	if(i != sizeof(cbuf)) {
	    int ver = extract_bitfield(h, 11, 12);   /* mpeg version */
	    int layer = extract_bitfield(h, 13, 14); /* mpeg layer */
	    aux = extract_bitfield(h, 16, 19); /* bitrate */
	    int framelen;

	    switch(ver) {
		case MPGVER_1:
		    switch(layer) {
			case MPGLAYER_1: bitrate = bitrate_tbl[aux]; break;
			case MPGLAYER_2: bitrate = bitrate_tbl[aux+16]; break;
			case MPGLAYER_3: bitrate = bitrate_tbl[aux+32]; break;
		    }
		    break;

		case MPGVER_2:
		case MPGVER_25:
		    switch(layer) {
			case MPGLAYER_1: bitrate = bitrate_tbl[aux+48]; break;
			case MPGLAYER_2:
			case MPGLAYER_3: bitrate = bitrate_tbl[aux+64]; break;
		    }
		    break;
	    }

	    bitrate *= 1000; /* the bitrate table is /1000 */
	    aux = extract_bitfield(h, 20, 21); /* frequency */

	    switch(ver) {
		case MPGVER_1: freq = freq_tbl[aux]; break;
		case MPGVER_2: freq = freq_tbl[aux+4]; break;
		case MPGVER_25: freq = freq_tbl[aux+8]; break;
		default: freq = 0; break;
	    }

	    int framepad = extract_bitfield(h, 22, 22);      /* padding */

	    /* Compute the frame length in bytes */
	    if(freq)
	    switch(layer) {
		case MPGLAYER_1: framelen = (((12*bitrate)/freq)+framepad) * 4; break;
		case MPGLAYER_2: framelen = ((144*bitrate)/freq)+framepad; break;
		case MPGLAYER_3:
		    if(ver == MPGVER_2 || ver == MPGVER_25) framelen = ((144*bitrate)/(freq*2))+framepad;
			else framelen = ((144*bitrate)/freq)+framepad;
		    break;
		default: framelen = 0; break;
	    }

	    /* Extimate the playtime and number of frames */
	    if(bitrate) {
		unsigned int datalen = filelen;
		length = datalen/(bitrate/8);
	    }
	}

	bitrate /= 1000;
	f.close();
    }
}

bool mp3track::ismpegheader(unsigned char *h) const {
    short context;
    char *p = (char *) &context;
    p[0] = h[1];
    p[1] = h[0];
    return (context & 0xfffe) == 0xfffa;
}

// ----------------------------------------------------------------------------

void mp3track::runplayer() {
    int inpipe[2], outpipe[2];

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
	    execl("/bin/sh", "/bin/sh", "-c", conf.getmp3player().c_str(), 0);

	    _exit(0);
	} else {
	    fpread = fdopen(inpipe[0], "r");
	    fpwrite = fdopen(outpipe[1], "w");

	    setvbuf(fpread, 0, _IONBF, 0);

	    sleep(1);
	    waitpid(cpid, 0, WNOHANG);

	    if(::kill(cpid, 0)) {   // The PID doesn't exist anymore
		cpid = -1;
		ui.status(_("Cannot run the MP3 player program"));
	    } else {
		ui.status(_("The MP3 player program has been launched"));
	    }
	}
    }
}

void mp3track::kill() {
    if(cpid > 0) {
	::kill(-cpid, SIGKILL);
	cpid = -1;
	trackcount = 0;
	fclose(fpread);
	fclose(fpwrite);
    }
}

void mp3track::sendcommand(const char *fmt, ...) {
    va_list ap;

    if(cpid < 0) runplayer();

    if(cpid > 0) {
	va_start(ap, fmt);
	vsprintf(cbuf, fmt, ap);
	va_end(ap);

	fprintf(fpwrite, "%s\n", cbuf);
	fflush(fpwrite);
    }
}

#define writetag(t, n) \
    memset(cbuf, ' ', n); strncpy(cbuf, rusconv("kw", t).c_str(), n); \
    cbuf[n] = 0; f.write(cbuf, n);

void mp3track::setidtags(string &aartist, string &atitle, string &aalbum, string &ayear, string &acomment) {
    fstream f(fname.c_str(), ios::in | ios::out);

    if(f.is_open()) {
	artist = aartist, title = atitle, album = aalbum, year = ayear, comment = acomment;
	f.seekg(-128, ios::end);

	f.read(cbuf, 3);
	cbuf[3] = 0;

	bool atag;

	if(atag = (string) cbuf != "TAG") {
	    f.seekg(0, ios::end);
	    f.write("TAG", 3);
	}

	writetag(title, 30);
	writetag(artist, 30);
	writetag(album, 30);
	writetag(year, 4);
	writetag(comment, 30);

	if(atag) {
	    writetag((string) "\000", 1);
	}

	f.close();
    } else {
	ui.statusf("Cannot open %s in RW mode", justfname(fname).c_str());
    }
}
