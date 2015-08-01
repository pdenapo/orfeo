/*
*
* orpheus CD track class implementation
* $Id: cdtrack.cc,v 1.22 2006/05/13 09:14:28 konst Exp $
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

#include "cdtrack.h"
#include "playlist.h"

#include "abstract/userinterface.h"

#include "kkstrtext.h"

#include <algorithm>
#include <functional>
#include <memory>

#ifdef HAVE_LIBGHTTP
#include <ghttp.h>
#endif

#include <unistd.h>

int cdfd = -1, numtracks;
string cddbquery, cdid, cdtitle;
bool ejected = false;

struct istracknumber: public binary_function<int, const track*, bool> {
    public: bool operator()(int n, const track *c) const {
	const cdtrack *cdt = static_cast<const cdtrack *>(c);

	if(cdt) {
	    return cdt->getnumber() == n;
	} else {
	    return false;
	}
    }
};

// ----------------------------------------------------------------------------

void cdtrack::play() {
    struct cdrom_ti n;
    n.cdti_trk0 = number;
    n.cdti_trk1 = number;

    if(!ioctl(cdfd, CDROMPLAYTRKIND, &n)) {
	paused = false;
    }
}

void cdtrack::pause() {
    if(paused) {
	ioctl(cdfd, CDROMRESUME);
	paused = false;

    } else if(ioctl(cdfd, CDROMPAUSE) != -1) {
	paused = true;

    }
}

void cdtrack::stop() {
    if(ioctl(cdfd, CDROMSTOP) != -1) {
    }
}

string cdtrack::getdescription() const {
    return (string) "(cd ) " + i2str(number) + ". " + title;
}

vector<string> cdtrack::getstatus() const {
    vector<string> r;
    struct cdrom_subchnl subc;
    char buf[64];
    OrpheusPlayList::iterator it;
    string sb;

    subc.cdsc_format = CDROM_MSF;

    if(ioctl(cdfd, CDROMSUBCHNL, &subc) != -1) {
	sprintf(buf, _("track %d [%02d:%02d]"), number,
	    subc.cdsc_reladdr.msf.minute, subc.cdsc_reladdr.msf.second);

	r.push_back("CD: " + cdtitle + "; " + buf);

	it = find_if(plist.begin(), plist.end(), bind1st(istracknumber(), subc.cdsc_trk));

	if(it != plist.end()) {
	    cdtrack *ct = static_cast<cdtrack *>(*it);
	    if(ct) {
		r.push_back(ct->gettitle());
	    }
	}
    }

    return r;
}

bool cdtrack::terminated() const {
    double nframes, lframe;
    struct cdrom_subchnl subc;
    int ptime;

    subc.cdsc_format = CDROM_MSF;

    if(ioctl(cdfd, CDROMSUBCHNL, &subc) != -1) {
	return
	    subc.cdsc_audiostatus != CDROM_AUDIO_PLAY &&
	    subc.cdsc_audiostatus != CDROM_AUDIO_PAUSED;
    }

    return true;
}

void cdtrack::fwd(bool big) {
    playseconds(big ? 40 : 4);
}

void cdtrack::rwd(bool big) {
    playseconds(big ? -40 : -4);
}

void cdtrack::playseconds(int offset) {
    struct cdrom_msf msf;
    struct cdrom_subchnl subc;

    subc.cdsc_format = CDROM_MSF;

    if(ioctl(cdfd, CDROMSUBCHNL, &subc) != -1) {
	msf.cdmsf_sec0 = subc.cdsc_absaddr.msf.second+offset;
	msf.cdmsf_min0 = subc.cdsc_absaddr.msf.minute;
	msf.cdmsf_frame0 = subc.cdsc_absaddr.msf.frame;
	msf.cdmsf_min1 = toc.cdte_addr.msf.minute;
	msf.cdmsf_sec1 = toc.cdte_addr.msf.second;
	msf.cdmsf_frame1 = toc.cdte_addr.msf.frame;

	if(msf.cdmsf_sec0 > 60 && (offset < 0)) {
	    msf.cdmsf_sec0 = 60-abs(offset);
	    msf.cdmsf_min0--;
	}

	if(ioctl(cdfd, CDROMPLAYMSF, &msf) != -1) {
	}
    }
}

void cdtrack::readlength() {
    signed int start_sec, start_min, end_min, end_sec;
    struct cdrom_msf msf;
    struct cdrom_tochdr toch;

    toc.cdte_track = number;
    toc.cdte_format = CDROM_MSF;

    if(ioctl(cdfd, CDROMREADTOCENTRY, &toc) != -1) {
	start_sec = toc.cdte_addr.msf.second;
	start_min = toc.cdte_addr.msf.minute;

	msf.cdmsf_min0 = toc.cdte_addr.msf.minute;
	msf.cdmsf_sec0 = toc.cdte_addr.msf.second;
	msf.cdmsf_frame0 = toc.cdte_addr.msf.frame;

	if(ioctl(cdfd, CDROMREADTOCHDR, &toch) != -1) {
	    if(toch.cdth_trk1 == number) {
		toc.cdte_track = CDROM_LEADOUT;
	    } else {
		toc.cdte_track = number+1;
	    }

	    if(ioctl(cdfd, CDROMREADTOCENTRY, &toc) != -1) {
		msf.cdmsf_min1 = toc.cdte_addr.msf.minute;
		msf.cdmsf_sec1 = toc.cdte_addr.msf.second;
		msf.cdmsf_frame1 = toc.cdte_addr.msf.frame;

		end_sec = toc.cdte_addr.msf.second;
		end_min = toc.cdte_addr.msf.minute;

		length = (end_min*60+end_sec)-(start_min*60+start_sec);
	    }
	}
    }
}

void cdtrack::toplaylist(ofstream &f) const {
    f << "@\t" << number << endl;
}

// ----------------------------------------------------------------------------

static bool isourtrack(const track *t) {
    const cdtrack *cd = dynamic_cast<const cdtrack *>(t);
    return (cd != 0);
}

static int getdbsum(int val) {
    int ret = 0, i;
    string buf = i2str(val);

    for(i = 0; i < buf.size(); i++)
	ret += buf[i]-'0';

   return ret;
}

void cdtrack::cleartracks() {
    OrpheusPlayList::iterator it;

    while((it = find_if(plist.begin(), plist.end(), isourtrack)) != plist.end()) {
	delete *it;
	plist.erase(it);
    }
}

void cdtrack::readtracks() {
    int st_track, lst_track, track, start_sec, start_min, end_sec, end_min;
    int tracklen, tracksum, disklen, sframe;
    unsigned int discid;
    struct cdrom_tocentry toc;
    struct cdrom_tochdr toch;
    cdtrack *ct;
    char buf[64];

    ui.status(_("Reading CD tracks.."));
    cleartracks();

    if(cdfd == -1) {
	cdfd = open(conf.getcddevice().c_str(), O_RDONLY | O_NONBLOCK);
    }

    cddbquery = "";

    if(!ioctl(cdfd, CDROMSTART) && !ioctl(cdfd, CDROMREADTOCHDR, &toch)) {
	st_track = toch.cdth_trk0;
	lst_track = toch.cdth_trk1;

	toc.cdte_format = CDROM_MSF;
	tracksum = 0;

	for(track = 1; track <= lst_track; track++) {
	    toc.cdte_track = track;

	    if(ioctl(cdfd, CDROMREADTOCENTRY, &toc) != -1) {
		start_sec = toc.cdte_addr.msf.second;
		start_min = toc.cdte_addr.msf.minute;

		if(toch.cdth_trk1 == track) {
		    toc.cdte_track = CDROM_LEADOUT;
		} else {
		    toc.cdte_track = track + 1;
		}

		sframe = (start_min * 60 + start_sec) * 75
		    + toc.cdte_addr.msf.frame;

		if(ioctl(cdfd, CDROMREADTOCENTRY, &toc) != -1) {
		    end_sec = toc.cdte_addr.msf.second;
		    end_min = toc.cdte_addr.msf.minute;

		    tracklen = (end_min * 60 + end_sec) - (start_min * 60 + start_sec);

		    if(track == 1) {
			disklen = tracklen;
		    } else {
			disklen += tracklen;
		    }

		    plist.push_back(ct = new cdtrack(track, tracklen));
		    ct->toc = toc;

		    cddbquery += (string) "+" + i2str(sframe);
		    tracksum += getdbsum(start_min * 60 + start_sec);
		}
	    }
	}

	discid = (tracksum % 0xFF) << 24 | disklen << 8 | (lst_track-st_track);
	cddbquery += (string) "+" + i2str(disklen);

	sprintf(buf, "%08x", discid);
	cdid = buf;

	sprintf(buf, "cddb+query+%08x+%d", discid, (lst_track-st_track+1));
	cddbquery.insert(0, buf);

	getcddb();
	ui.update();
	ui.statusf(_("Done: read %d CD tracks"), lst_track-st_track+1);

    } else {
	ui.status(_("No CD in the drive!"));

    }
}

bool cdtrack::loadtracks(const string &fname) {
    ifstream f(fname.c_str());
    string buf, param;
    int ntrack;
    OrpheusPlayList::iterator it;
    bool r = false;

    if(f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);

	    if(buf.substr(0, 1) != "#") {
		param = getword(buf, "=");

		if(param == "DTITLE") {
		    cdtitle = rusconv("wk", buf);

		} else if(param.substr(0, 6) == "TTITLE") {
		    ntrack = strtoul(param.substr(6).c_str(), 0, 0);
		    it = find_if(plist.begin(), plist.end(), bind1st(istracknumber(), ntrack+1));

		    if(it != plist.end()) {
			cdtrack *ct = static_cast<cdtrack *>(*it);
			if(ct) {
			    ct->settitle(rusconv("wk", buf));
			    r = true;
			}
		    }

		}
	    }
	}

	f.close();
    }

    return r;
}

string cdtrack::makerequest(const string &cmd) {
    string uri, srvname, srvcgi, reply;

#ifdef HAVE_LIBGHTTP
    ghttp_request *r = ghttp_request_new();

    if(!r) return "";

    srvname = "freedb.freedb.org";
    srvcgi = "~cddb/cddb.cgi";

    uri = (string) "http://" + srvname + "/" + srvcgi + "?cmd=" + cmd +
	"&hello=private+free.the.cddb+" + PACKAGE + "+" + VERSION + "&proto=5";

    auto_ptr<char> curi(strdup(uri.c_str()));
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

    return reply;
}

void cdtrack::getcddb() {
    string genre, id, reply, fname;
    int npos, i;
    char readc[64];
    vector<string> lst;
    vector<string>::iterator il;

    if(!conf.getautofetch())
	return;

    ui.status(_("Getting the CDDB data.."));

    fname = (string) getenv("HOME") + "/.cddb/";
    if(access(fname.c_str(), X_OK)) {
	mkdir(fname.c_str(), S_IREAD | S_IWRITE | S_IEXEC);
    }

    cdtitle = "";
    fname = fname + cdid;

    if(access(fname.c_str(), R_OK)) {
	reply = makerequest(cddbquery);

	switch(strtoul(getword(reply).c_str(), 0, 0)) {
	    case 200:
		break;
	    case 210:
		break;
	    case 211:
		if((npos = reply.find("\r\n")) != -1) {
		    reply.erase(0, npos+1);

		    for(i = 0; i < 3; i++)
		    switch(i) {
			case 0: genre = getword(reply); break;
			case 1: id = getword(reply); break;
			case 2:
			    cdtitle = reply;
			    if((npos = cdtitle.find_first_of("\r\n")) != -1)
				cdtitle.erase(npos);
			    break;
		    }
		}

		ui.status(_("Found! Let's read the tracks.."));
		break;
	    default:
		break;
	}

	if(!cdtitle.empty()) {
	    reply = makerequest((string) "cddb+read+" + genre + "+" + id);

	    if(strtoul(getword(reply).c_str(), 0, 0) == 210) {
		breakintolines(reply, lst);

		if(lst.size() > 2) {
		    lst.erase(lst.begin());
		    lst.pop_back();

		    ofstream f(fname.c_str());

		    if(f.is_open()) {
			for(il = lst.begin(); il != lst.end(); il++)
			    f << *il << endl;
			f.close();
		    }
		}
	    }
	}
    }

    if(loadtracks(fname)) {
	ui.status(_("CDDB info has been fetched"));
	ui.update();
    } else {
	ui.status(_("Failed to fetch track titles"));
    }
}

void cdtrack::eject() {
    if(cdfd == -1)
	cdfd = open(conf.getcddevice().c_str(), O_RDONLY | O_NONBLOCK);

    if(ioctl(cdfd, ejected ? CDROMCLOSETRAY : CDROMEJECT) != -1)
	ejected = !ejected;
}

string cdtrack::getcdid() {
    return cdid;
}
