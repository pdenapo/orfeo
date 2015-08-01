/*
*
* orpheus stream track class implementation
* $Id$
*
* Copyright (C) 2006 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "streamtrack.h"
#include "playlist.h"

#include "abstract/userinterface.h"

#include "kkstrtext.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>

#include <algorithm>

#ifdef HAVE_LIBGHTTP
#include <ghttp.h>

#ifdef HAVE_LIBXML2
#include <libxml/xmlreader.h>
#endif

#endif

static int cpid = -1;
static FILE *fpread, *fpwrite;

streamtrack::streamtrack(const string &aurl): url(aurl), paused(false) {
}

streamtrack::~streamtrack() {
}

void streamtrack::play() {
    paused = false;
    runplayer();
}

void streamtrack::pause() {
    if(cpid != 1) {
	paused = !paused;
	if(paused) kill(-cpid, SIGSTOP);
	    else kill(-cpid, SIGCONT);
    }
}

void streamtrack::stop() {
    kill(-cpid, SIGCONT);
    kill(-cpid, SIGKILL);
}

bool streamtrack::terminated() const {
    return cpid == -1;
}

string streamtrack::getdescription() const {
    string r = "(bdc) ";

    if(!station.empty()) {
	r += station;
    } else {
	r += url;
    }

    return r;
}

vector<string> streamtrack::getstatus() const {
    vector<string> r;
    string desc, sbuf, cmd, tm;
    char cbuf[4096], c;
    int rc;

    // ICY Info: StreamTitle='White Stripes - Fell in Love With a Girl';StreamUrl='http://www.freeformrock.com';
    // ICY Info: StreamTitle='';StreamUrl='';

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

	    while(sbuffer.find("\n") != -1) {
		sbuf = getword(sbuffer, "\n");
		cmd = getword(sbuf, ":");

		if(cmd == "ICY Info") {
		    int pos;

		    if((pos = sbuf.find("StreamTitle='")) != -1) {
			if(sbuf.substr(pos+12, 2) != "''") {
			    tm = sbuf.substr(pos);
			    getword(tm, "'");
			    if((pos = tm.find("';")) != -1)
				title = tm.substr(0, pos);
			}
		    }
		} else if(cmd == "Name   ") {
		    if(station.empty()) {
			station = sbuf.substr(1);
			ui.update();
		    }
		} else if(cmd == "Genre  ") {
		    genre = sbuf.substr(1);
		} else if(cmd == "Bitrate") {
		    bitrate = sbuf.substr(1);
		}
	    }

	} else {
	    term = kill(cpid, 0);
	}

	if(term) {
	    cpid = -1;
	    fclose(fpread);
	    fclose(fpwrite);
	    ui.status(_("The stream player application has terminated"));
	}
    }

    string top;

    if(!station.empty()) top = station;

    if(!genre.empty()) {
	if(!top.empty()) top += ", ";
	top += genre;
    }

    if(!bitrate.empty()) {
	if(!top.empty()) top += ", ";
	top += bitrate;
    }

    r.push_back(top);
    r.push_back(url);
    r.push_back(title);

    return r;
}

void streamtrack::toplaylist(ofstream &f) const {
    f << url << endl;
}

// ----------------------------------------------------------------------------

void streamtrack::runplayer() {
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
	    execl("/bin/sh", "/bin/sh", "-c", (conf.getstreamplayer() + " '" + url + "'").c_str(), 0);

	    _exit(0);
	} else {
	    fpread = fdopen(inpipe[0], "r");
	    fpwrite = fdopen(outpipe[1], "w");

	    setvbuf(fpread, 0, _IONBF, 0);

	    sleep(1);
	    waitpid(cpid, 0, WNOHANG);

	    if(::kill(cpid, 0)) {   // The PID doesn't exist anymore
		cpid = -1;
		ui.status(_("Cannot run the stream player program"));
	    } else {
		ui.status(_("The stream player program has been launched"));
	    }
	}
    }
}

string streamtrack::makerequest(const string &url) {
    string uri, reply;

#ifdef HAVE_LIBGHTTP
    ghttp_request *r = ghttp_request_new();

    if(!r) return "";

    auto_ptr<char> curi(strdup(url.c_str()));
    auto_ptr<char> cproxy(strdup(conf.getproxy().c_str()));

    if(strlen(cproxy.get())) {
	ghttp_set_proxy(r, cproxy.get());
    }

    ghttp_set_uri(r, curi.get());

    ghttp_set_header(r, "User-Agent", ((string) PACKAGE + " " + VERSION).c_str());
    ghttp_set_header(r, http_hdr_Connection, "close");
    ghttp_prepare(r);
    ghttp_process(r);

    auto_ptr<char> creply(ghttp_get_body(r));

    if(creply.get()) {
	reply = creply.get();
    } else {
	ghttp_request_destroy(r);
    }
#endif

    string fname = conf.getbasedir() + "/presets.xml";

    if(!reply.empty()) {
	ofstream f(fname.c_str());
	if(f.is_open()) {
	    f << reply;
	    f.close();
	}

    } else {
	string buf;
	ifstream f(fname.c_str());
	if(f.is_open()) {
	    while(!f.eof()) {
		getline(f, buf);
		reply += buf;
	    }

	    f.close();
	}

    }

    return reply;
}

struct equals_url: public binary_function<const char *, const track*, bool> {
    public: bool operator()(const char *aurl, const track *c) const {
	const streamtrack *st = static_cast<const streamtrack *>(c);

	if(st) {
	    return st->getURL() == (string) aurl;
	} else {
	    return false;
	}
    }
};

vector<streamtrack::TopGroup> streamtrack::loadPresets() {
    static vector<TopGroup> r;

#if defined(HAVE_LIBXML2) && defined(LIBXML_READER_ENABLED)
    if(r.empty()) {
	ui.status(_("Fetching the presets XML file.."));
	string reply = makerequest(conf.getradioxml()), stname;

	if(!reply.empty()) {
	    ui.status(_("Internet Radio presets file has been fetched"));
	    xmlTextReaderPtr rd = xmlReaderForMemory(reply.c_str(), reply.size(), "presets.xml", 0, 0);

	    if(rd) {
		while(xmlTextReaderRead(rd) == 1) {
		    if(xmlTextReaderNodeType(rd) == XML_READER_TYPE_ELEMENT) {
			const char *p = (const char *) xmlTextReaderConstName(rd);
			string name = p ? up(p) : "";

			if(name == "GROUP") {
			    if(p = (const char *) xmlTextReaderGetAttribute(rd, (const xmlChar *) "title")) {
				switch(xmlTextReaderDepth(rd)) {
				    case 1:
					r.push_back(TopGroup(p));
					break;
				    case 2:
					r.back().subgroups.push_back(SubGroup(p));
					break;
				}
			    }

			} else if(name == "STATION") {
			    p = (const char *) xmlTextReaderGetAttribute(rd, (const xmlChar *) "title");
			    if(p) r.back().subgroups.back().stations.push_back(Station(p));

			} else if(name == "SOURCE") {
			    if(xmlTextReaderRead(rd) == 1) {
				p = (const char *) xmlTextReaderConstValue(rd);
				if(p) r.back().subgroups.back().stations.back().urls.push_back(p);
			    }
			}
		    }
		}

		xmlFreeTextReader(rd);
	    } else {
		ui.status(_("Unable to parse the presets file"));
	    }

	} else {
	    ui.status(_("Internet Radio presets fetch failed"));
	}
    }
#endif
    return r;
}

map<string, string> streamtrack::loadPresetNames() {
    vector<TopGroup> psets = loadPresets();
    map<string, string> r;

    for(vector<TopGroup>::const_iterator it = psets.begin(); it != psets.end(); ++it) {
	for(vector<SubGroup>::const_iterator is = it->subgroups.begin(); is != it->subgroups.end(); ++is) {
	    for(vector<Station>::const_iterator ist = is->stations.begin(); ist != is->stations.end(); ++ist) {
		for(vector<string>::const_iterator iu = ist->urls.begin(); iu != ist->urls.end(); ++iu) {
		    r[*iu] = ist->name;
		}
	    }
	}
    }

    return r;
}
