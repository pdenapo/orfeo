/*
*
* orpheus configuration class implementation
* $Id: orpheusconf.cc,v 1.14 2006/05/13 09:14:28 konst Exp $
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

#include "orpheusconf.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "kkstrtext.h"

OrpheusConfiguration conf;

OrpheusConfiguration::OrpheusConfiguration()
    : dname((string) getenv("HOME") + "/.orpheus"),
      oggplayer("ogg123 -v"), streamplayer("mplayer"),
      cddev("/dev/cdrom"), mixerdev("/dev/mixer"),
      radioxml("http://www.screamer-radio.com/update.php?fetch=presets"),
      playmode(Normal), sortorder(noSort), cdautofetch(true),
      russian(false), autosavepl(false), autoplay(true), sortasc(true)
{
    if(access(dname.c_str(), X_OK))
	mkdir(dname.c_str(), S_IREAD | S_IWRITE | S_IEXEC);

    load();
}

OrpheusConfiguration::~OrpheusConfiguration() {
}

void OrpheusConfiguration::load() {
    ifstream f((dname + "/config").c_str());
    string buf, param;

    if(f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);
	    param = getword(buf);

	    if(param == "mp3player") mp3player = buf; else
	    if(param == "oggplayer") oggplayer = buf; else
	    if(param == "streamplayer") oggplayer = buf; else
	    if(param == "cddev") cddev = buf; else
	    if(param == "mixerdev") mixerdev = buf; else
	    if(param == "proxy") proxy = buf; else
	    if(param == "radioxml") radioxml = buf; else
	    if(param == "autofetch") cdautofetch = buf == "1"; else
	    if(param == "russian") russian = buf == "1"; else
	    if(param == "autosavepl") autosavepl = buf == "1"; else
	    if(param == "autoplay") autoplay = buf == "1"; else
	    if(param == "playmode") playmode = (PlayMode) strtoul(buf.c_str(), 0, 0); else
	    if(param == "sortorder") sortorder = (SortOrder) strtoul(buf.c_str(), 0, 0); else
	    if(param == "sortasc") sortasc = buf == "1";
	}

	f.close();
    }

    if(mp3player.empty()) {
	if(system("which mpg321") && !system("which mpg123")) mp3player = "mpg123";
	if(mp3player.empty()) mp3player = "mpg321";
	mp3player += " -R -";
    }

#ifndef HAVE_LIBGHTTP
    cdautofetch = false;
#endif
}

void OrpheusConfiguration::save() const {
    ofstream f((dname + "/config").c_str());

    if(f.is_open()) {
	f
	    << "mp3player" << "\t" << mp3player << endl
	    << "oggplayer" << "\t" << oggplayer << endl
	    << "streamplayer" << "\t" << streamplayer << endl
	    << "cddev" << "\t" << cddev << endl
	    << "mixerdev" << "\t" << mixerdev << endl
	    << "proxy" << "\t" << proxy << endl
	    << "radioxml" << "\t" << radioxml << endl
	    << "autofetch" << "\t" << (cdautofetch ? "1" : "0") << endl
	    << "russian" << "\t" << (russian ? "1" : "0") << endl
	    << "autosavepl" << "\t" << (autosavepl ? "1" : "0") << endl
	    << "autoplay" << "\t" << (autoplay ? "1" : "0") << endl
	    << "playmode" << "\t" << (int) playmode << endl
	    << "sortorder" << "\t" << (int) sortorder << endl
	    << "sortasc" << "\t" << (int) sortasc << endl;

	f.close();
    }
}

// ----------------------------------------------------------------------------

string rusconv(const string &tdir, const string &text) {
    string r;

#ifndef HAVE_ICONV_H
    static unsigned char kw[] = {
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,184,164,165,166,167,168,169,170,171,172,173,174,175,
	176,177,178,168,180,181,182,183,184,185,186,187,188,189,190,191,
	254,224,225,246,228,229,244,227,245,232,233,234,235,236,237,238,
	239,255,240,241,242,243,230,226,252,251,231,248,253,249,247,250,
	222,192,193,214,196,197,212,195,213,200,201,202,203,204,205,206,
	207,223,208,209,210,211,198,194,220,219,199,216,221,217,215,218
    };

    static unsigned char wk[] = {
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,163,164,165,166,167,179,169,170,171,172,173,174,175,
	176,177,178,179,180,181,182,183,163,185,186,187,188,189,190,191,
	225,226,247,231,228,229,246,250,233,234,235,236,237,238,239,240,
	242,243,244,245,230,232,227,254,251,253,255,249,248,252,224,241,
	193,194,215,199,196,197,214,218,201,202,203,204,205,206,207,208,
	210,211,212,213,198,200,195,222,219,221,223,217,216,220,192,209
    };

    unsigned char c;
    string::const_iterator i;
    unsigned char *table = 0;

    if(tdir == "kw") table = kw; else
    if(tdir == "wk") table = wk;

    if(conf.getrussian() && table) {
	for(i = text.begin(); i != text.end(); i++) {
	    c = (unsigned char) *i;
	    c &= 0377;
	    if(c & 0200) c = table[c & 0177];
	    r += c;
	}
    } else {
	r = text;
    }
#else

    if(conf.getrussian()) {
	if(tdir == "kw") r = siconv(text, "koi8-r", "cp1251"); else
	if(tdir == "wk") r = siconv(text, "cp1251", "koi8-r"); else
	    r = text;
    } else {
	r = text;
    }

#endif

    return r;
}

string strplaymode(PlayMode mode) {
    switch(mode) {
	case Normal: return _("Normal");
	case TrackRepeat: return _("Repeat track");
	case ListRepeat: return _("Repeat play list");
	case Random: return _("Random");
    }

    return "";
}

string strsortorder(SortOrder order) {
    switch(order) {
	case byFileName: return _("by File Name");
	case byTrackName: return _("by Track Name");
	case byTrackLength: return _("by Track Length");
	case byLastModified: return _("by Last Modified");
	case noSort: return _("no Sorting");
    }

    return "";
}

string up(string s) {
    int k;
    string r;

    for(k = 0; k < s.size(); k++)
	r += toupper(s[k]);

    return r;
}
