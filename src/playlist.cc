/*
*
* orpheus playlist class implementation
* $Id: playlist.cc,v 1.15 2006/05/13 09:14:28 konst Exp $
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

#include "playlist.h"
#include "mp3track.h"
#include "oggtrack.h"
#include "cdtrack.h"
#include "streamtrack.h"

#include "abstract/userinterface.h"

#include "kkstrtext.h"

#include <unistd.h>
#include <map>
#include <algorithm>

OrpheusPlayList plist;
OrpheusRecentLists recent;

bool OrpheusPlayList::save(const string &afname, bool askifany) {
    bool r, idwritten = false;

    if(askifany)
    if(!access(afname.c_str(), F_OK))
    if(ui.askf("YN", "%s already exists. Do you want to overwrite it?", justfname(afname).c_str()) != ASK_YES)
	return false;

    ofstream f((fname = afname).c_str());
    iterator i;

    if(r = f.is_open()) {
	for(i = begin(); i != end(); i++) {
	    if(dynamic_cast<cdtrack *>(*i) && !idwritten) {
		f << "@\tID\t" << cdtrack::getcdid() << endl;
		idwritten = true;
	    }

	    (*i)->toplaylist(f);
	}

	f.close();
	recent.tweak(fname);

    } else {
	ui.status(_("Unable to save the playlist file!"));
    }

    return r;
}

bool OrpheusPlayList::load(const string &afname) {
    bool r;
    ifstream f((fname = afname).c_str());
    string buf, t, param, id, path, ext;
    map<string, string> psetnames;
    int pos;

    clear();
    path = justpathname(afname);

    ui.statusf(_("Loading tracks from %s.."), afname.c_str());

    if(r = f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);

	    buf = trailcut(buf);

	    if(buf.empty())
		continue;

	    if(buf[0] == '#')
		continue;

	    while((pos = buf.find("\\")) != -1)
		buf.replace(pos, 1, "/");

	    ext = ruscase(getrword(t = buf, "."), "tolower");

	    if(ext == "mp3") {
		if(buf[0] != '/')
		    buf = path + "/" + buf;

		if(!access(buf.c_str(), R_OK)) {
		    push_back(new mp3track(buf));
		}

	    } else if(ext == "ogg") {
		if(buf[0] != '/')
		    buf = path + "/" + buf;

		if(!access(buf.c_str(), R_OK)) {
		    plist.push_back(new oggtrack(buf));
		}

	    } else if(getword(t = buf) == "@") {
		param = getword(t);

		if(param == "ID") {
		    id = getword(t);
		    cdtrack::readtracks();
		    cdtrack::cleartracks();

		} else if(!id.empty() && id == cdtrack::getcdid()) {
		    push_back(new cdtrack(strtoul(param.c_str(), 0, 0)));

		}

	    } else if(buf.find("://") != -1) {
		streamtrack *t = new streamtrack(buf);

		if(psetnames.empty())
		    psetnames = streamtrack::loadPresetNames();

		map<string, string>::const_iterator in = psetnames.find(buf);
		if(in != psetnames.end()) t->station = in->second;

		push_back(t);
	    }
	}

	f.close();
	recent.tweak(fname.c_str());

	if(!id.empty() && id == cdtrack::getcdid()) {
	    cdtrack::getcddb();
	} else {
	    ui.statusf(_("Playlist loaded successfully"));
	}

	ui.update();
    } else {
	ui.status(_("Unable to open the playlist file!"));
    }

    if(fname.find("/.orpheus/playlist") != -1)
	fname = "";

    sort();

    return r;
}

short OrpheusPlayList::compare(const track *t1, const track *t2) {
    short r = 0;

    const idtrack *idt1 = dynamic_cast<const idtrack *>(t1);
    const idtrack *idt2 = dynamic_cast<const idtrack *>(t2);

    switch(conf.getsortorder()) {
	case byFileName:
	    if(idt1 && idt2) {
		if(idt1->getfname() < idt2->getfname()) r = 1;
		    else r = -1;
	    }
	    break;

	case byTrackLength:
	    if(t1->getlength() < t2->getlength()) r = 1;
		else r = -1;
	    break;

	case byLastModified:
	    if(idt1 && idt2) {
		if(idt1->getlastmod() < idt2->getlastmod()) r = 1;
		    else r = -1;
	    }
	    break;

	case byTrackName:
	    if(t1->getdescription() < t2->getdescription()) r = 1;
		else r = -1;
	    break;
    }

    return r;
}

void OrpheusPlayList::sort() {
    bool ok;
    short compresult;
    iterator it, ic, imax;

    for(it = begin(); it != end(); ++it) {
	for(ic = imax = it; ic != end(); ++ic) {
	    compresult = compare(*ic, *imax);

	    if(conf.getsortasc()) ok = compresult > 0;
		else ok = compresult < 0;

	    if(ok) imax = ic;
	}

	if(imax != it) {
	    track *p = *imax;
	    *imax = *it;
	    *it = p;
	}
    }
}

void OrpheusPlayList::clear() {
    iterator ip = begin();
    while(ip != end()) {
	delete *ip;
	++ip;
    }

    vector<track *>::clear();
}

// ----------------------------------------------------------------------------

void OrpheusRecentLists::load() {
    ifstream f((conf.getbasedir() + "/recent").c_str());
    string buf;

    if(f.is_open()) {
	while(getstring(f, buf))
	    if(!buf.empty())
		push_back(buf);

	f.close();
    }

    buf = conf.getbasedir() + "/mytunes";
    if(access(buf.c_str(), F_OK))
	mkdir(buf.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
}

void OrpheusRecentLists::save() const {
    ofstream f((conf.getbasedir() + "/recent").c_str());

    if(f.is_open()) {
	for(const_iterator i = begin(); i != end(); ++i)
	    f << *i << endl;

	f.close();
    }
}

void OrpheusRecentLists::tweak(const string &afname) {
    iterator i;

    if(!afname.empty() && afname != conf.getbasedir() + "/playlist") {
	for(i = begin(); i != end() && *i != afname; ++i);
	if(i != end()) erase(i);
	insert(begin(), afname);
    }
}
