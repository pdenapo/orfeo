/*
*
* orpheus text UI class implementation
* $Id: uitext.cc,v 1.35 2006/05/13 09:14:28 konst Exp $
*
* Copyright (C) 2002-6 by Konstantin Klyagin <konst@konst.org.ua>
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
#include "playlist.h"
#include "cdtrack.h"
#include "mp3track.h"
#include "oggtrack.h"
#include "streamtrack.h"
#include "orpheusconf.h"
#include "mixerctl.h"

#include <algorithm>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <iostream>

OrpheusTextUI tui;

bool OrpheusTextUI::blocked = false;
bool OrpheusTextUI::dotermresize = false;

int OrpheusTextUI::exec() {
    int n;
    bool finished;
    struct sigaction sact;

    finished = false;

    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = &sighandler;
    sigaction(SIGINT, &sact, 0);
    sigaction(SIGCHLD, &sact, 0);

    kinterface();

    loadcolors();
    init();
    redraw();

    recent.load();

    if(conf.getautosaveplaylist())
	plist.load(conf.getbasedir() + "/playlist");

    if(conf.getautoplay())
    if(plist.size())
	playtrack(plist.front());

    while(!finished) {
	n = trackm.open();
	finished = !n;

	if(!finished) {
	    if(!plist.empty())
		playtrack(plist[n-1]);
	}
    }

    if(dynamic_cast<oggtrack *>(currenttrack)) {
	currenttrack->stop();
    }

    if(conf.getautosaveplaylist())
	plist.save(conf.getbasedir() + "/playlist");

    recent.save();

    attrset(0);
    kendinterface();
    for(int i = 0; i < LINES; i++) cout << endl;
    cout << flush;

    return 0;
}

void OrpheusTextUI::playtrack(track *t) {
    if(currenttrack) {
	currenttrack->stop();

	if(dynamic_cast<mp3track *>(currenttrack) && !dynamic_cast<mp3track *>(t)) {
	    mp3track::kill();
	}
    }

    (currenttrack = t)->play();
    update();
}

void OrpheusTextUI::cleartrack() {
    if(currenttrack) {
	currenttrack->stop();

	if(static_cast<mp3track *>(currenttrack))
	    mp3track::kill();

	currenttrack = 0;
    }
}

void OrpheusTextUI::play(int n) {
    if(n > 0 && n < plist.size())
	playtrack(*(plist.begin() + n + 1));
}

void OrpheusTextUI::nexttrack() {
    if(conf.getplaymode() == Random) {
	int k;
	while(*(plist.begin()+(k = randlimit(0, plist.size()-1))) == currenttrack);
	playtrack(*(plist.begin()+k));

    } else {
	OrpheusPlayList::iterator nt = find(plist.begin(), plist.end(), currenttrack);

	if(nt != plist.end()) {
	    if(nt != plist.end() && ++nt != plist.end()) {
		playtrack(*nt);

	    } else {
		cleartrack();
		update();
	    }
	}
    }
}

void OrpheusTextUI::prevtrack() {
    OrpheusPlayList::iterator nt;

    nt = find(plist.begin(), plist.end(), currenttrack);

    if(nt != plist.end()) {
	if(nt != plist.begin()) {
	    nt--;
	    playtrack(*nt);
	} else {
	    cleartrack();
	    update();
	}
    }
}

void OrpheusTextUI::loadcolors() {
    string fname = (string) getenv("HOME") + "/.orpheus/colors";

    schemer.push(Status, "status black/white");
    schemer.push(NormalText, "normal_text green/transparent");
    schemer.push(Frames, "frames blue/transparent bold");
    schemer.push(HighLight, "highlight yellow/transparent bold");
    schemer.push(DialogText, "dialog_text black/white");
    schemer.push(DialogFrames, "dialog_frames blue/white");
    schemer.push(DialogHighLight, "dialog_highlight red/white");
    schemer.push(DialogSelected, "dialog_selected white/black   bold");

    if(access(fname.c_str(), F_OK)) {
	schemer.save(fname);
    } else {
	schemer.load(fname);
    }
}

void OrpheusTextUI::init() {
    mainw = textwindow(0, 1, COLS-1, LINES-2, schemer[Frames]);

    trackm = verticalmenu(1, 6, COLS-27, LINES-2, schemer[NormalText], schemer[HighLight]);
    trackm.otherkeys = &tracklistkeys;
    trackm.idle = &tracksidle;

    selector.idle = &selectoridle;

    selector.setcolor(schemer[DialogText],
	schemer[DialogHighLight],
	schemer[DialogSelected],
	schemer[DialogText]);

    input.setcolor(schemer[Status]);
    input.idle = &textinputidle;
    kt_resize_event = &termresize;
}

void OrpheusTextUI::redraw() {
    string url = "http://thekonst.net/orpheus/";

    attrset(schemer[Status]);
    mvhline(0, 0, ' ', COLS);

    kwriteatf(1, 0, schemer[Status], "%s %s", ruscase(PACKAGE, "toupper").c_str(), VERSION);
    kwriteatf(COLS-url.size()-1, 0, schemer[Status], "%s", url.c_str());

    status(_("Orpheus started"));

    mainw.open();

    mvhline(5, 1, HLINE, COLS-2);
    mvvline(6, COLS-27, VLINE, LINES-8);
    mvhline(5, 0, LTEE, 1);
    mvhline(5, COLS-1, RTEE, 1);
    mvhline(LINES-2, COLS-27, BTEE, 1);
    mvhline(5, COLS-27, TTEE, 1);

    update();
    showhelp();
}

void OrpheusTextUI::status(const string &s) {
    attrset(schemer[Status]);
    mvhline(LINES-1, 0, ' ', COLS);
    kwriteat(1, LINES-1, s.c_str(), schemer[Status]);
}

void OrpheusTextUI::update() {
    OrpheusPlayList::iterator i;
    string desc;
    int pos, length, desclen;
    char buf[64];

    trackm.clear();

    for(i = plist.begin(); i != plist.end(); i++) {
	desc = currenttrack == *i ? ">" : " ";
	desc += (*i)->getdescription();
	desclen = COLS-28;

	length = (*i)->getlength();
	if(length == -1)
	    desclen += 6;

	pos = desclen-7-desc.size();

	if(pos >= 0) {
	    desc += string(pos, ' ');
	} else {
	    desc.erase(desclen-10);
	    desc += ".. ";
	}

	if(length != -1) {
	    sprintf(buf, " %02d:%02d", (int) length/60, (int) length-(length/60)*60);
	    desc += buf;
	}

	trackm.additem(desc);
    }

    // Update xterm title

    if(xtitles) {
	cout << "\033]0;Orpheus";
	if(currenttrack) cout << ": " << currenttrack->getdescription() << "\007";
	cout << "\007" << flush;
    }

    ofstream fcur((conf.getbasedir() + "/currently_playing").c_str());
    if(fcur.is_open()) {
	if(currenttrack) {
	    desc = currenttrack->getdescription();
	    if(desc.substr(0, 1) == "(") {
		pos = desc.find(")");
		if(pos != -1 && desc.size() > pos+2) desc.erase(0, pos+2);
	    }
	    fcur << desc;
	}

	fcur.close();
    }

    if(!blocked)
	trackm.redraw();
}

void OrpheusTextUI::generalidle() {
    vector<string> st;
    generalidle(st);
}

void OrpheusTextUI::generalidle(vector<string> &st) {
    fd_set fds;
    struct timeval tv;
    OrpheusPlayList::iterator nt;
    int k;

    FD_ZERO(&fds);
    FD_SET(0, &fds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    select(1, &fds, 0, 0, &tv);

    if(currenttrack) {
	if(currenttrack->terminated()) {
	    switch(conf.getplaymode()) {
		case Random:
		case Normal:
		    nexttrack();
		    break;

		case ListRepeat:
		    nt = find(plist.begin(), plist.end(), currenttrack);
		    if(nt != plist.end()) {
			if(nt == plist.end()-1) playtrack(*plist.begin());
			else nexttrack();
		    }
		    break;

		case TrackRepeat:
		    currenttrack->play();
		    break;
	    }
	} else {
	    st = currenttrack->getstatus();
	}
    } else {
	st.push_back(_("Silent"));
    }

    if(!blocked && dotermresize) {
	init();
	redraw();
	dotermresize = false;
    }
}

void OrpheusTextUI::tracksidle(verticalmenu &m) {
    int i;
    vector<string> st;

    tui.generalidle(st);

    attrset(tui.schemer[HighLight]);

    for(i = 0; i < 3; i++) {
	if(i < st.size()) {
	    if(st[i].size() > COLS-4) {
		st[i].erase(COLS-6);
		st[i] += "..";
	    }

	    mvprintw(i+2, 2, "");
	    printstring(st[i]);
	    mvhline(i+2, 2+st[i].size(), ' ', COLS-4-st[i].size());
	} else {
	    mvhline(i+2, 2, ' ', COLS-4);
	}
    }

    refresh();
}

void OrpheusTextUI::selectoridle(fileselector &caller) {
    blocked = true;
    tui.generalidle();
    blocked = false;
}

void OrpheusTextUI::dialogidle(dialogbox &db) {
    blocked = true;
    tui.generalidle();
    blocked = false;
}

void OrpheusTextUI::menuidle(verticalmenu &m) {
    blocked = true;
    tui.generalidle();
    blocked = false;
}

void OrpheusTextUI::textinputidle(textinputline &line) {
    tui.generalidle();
}

void OrpheusTextUI::termresize(void) {
    dotermresize = true;
}

void OrpheusTextUI::showhelp() {
    typedef pair<string, string> spair;
    vector<spair> help;
    vector<spair>::iterator i;

    help.push_back(spair(_("Start/Stop"), "enter/s"));
    help.push_back(spair(_("Pause"), "space/p"));
    help.push_back(spair(_("Skip ahead/back"), "}/{ ]/["));
    help.push_back(spair(_("Add/Remove item"),  "a/d"));
    help.push_back(spair(_("Add broadcast"), "b"));
    help.push_back(spair(_("Next/Prev track"), "+/-"));
    help.push_back(spair(_("Move Up/Down"), "<-/->"));
    help.push_back(spair(_("Edit tags"), "e"));
    help.push_back(spair(_("Search track"), "/"));

    help.push_back(spair("", ""));

    help.push_back(spair(_("Load/Save list"), "f3/f2"));
    help.push_back(spair(_("Recent playlists"), "tab"));
    help.push_back(spair(_("Clear list"), "!"));
    help.push_back(spair(_("Read/Eject CD"), "f5/j"));

    help.push_back(spair("", ""));

    help.push_back(spair(_("Config/Mixer"), "c/m"));
    help.push_back(spair(_("Quit"), "q"));

    for(i = help.begin(); i != help.end(); i++) {
	mainw.write(COLS-25, i-help.begin()+5, i->first);
	mainw.write(COLS-2-i->second.size(), i-help.begin()+5, schemer[HighLight], i->second);
    }
}

void OrpheusTextUI::addtrackf(const string &fname) {
    string ext = up(fname.substr(fname.size()-4));
    if(ext == ".MP3") plist.push_back(new mp3track(fname)); else
    if(ext == ".OGG") plist.push_back(new oggtrack(fname)); else
    if(fname.find("://") != -1) plist.push_back(new streamtrack(fname));
}

void OrpheusTextUI::doadd(const string &aitem) {
    struct stat st;
    string fname, rbuf, lp;

    lp = getrword(rbuf = aitem, "/");
    if(lp == "." || lp == "..")
	return;

    if(!stat(aitem.c_str(), &st)) {
	if(S_ISDIR(st.st_mode)) {
	    DIR *d = opendir(aitem.c_str());
	    struct dirent *de;

	    if(d) {
		while(de = readdir(d)) {
		    fname = aitem + "/" + de->d_name;
		    if(!stat(fname.c_str(), &st))
			doadd(fname);

		}

		closedir(d);
	    }

	} else if(S_ISREG(st.st_mode)) {
	    addtrackf(aitem);
	}

    }
}

void OrpheusTextUI::addfiles() {
    textwindow selwindow(0, 0, (int) (COLS*0.65), (int) (LINES*0.7), schemer[DialogFrames], TW_CENTERED);
    selwindow.set_title(schemer[DialogHighLight], _(" [ <Ins> select; <Space> confirm; <Esc> cancel ] "));

    selector.setoptions(FSEL_MULTI);
    selector.setwindow(selwindow);

    selector.exec();
    selector.close();

    if(selector.getlastkey() != KEY_ESC) {
	vector<string> r = selector.getselected();
	vector<string>::iterator ir;

	for(ir = r.begin(); ir != r.end(); ir++) {
	    doadd(*ir);
	}

	plist.sort();
	update();
    }
}

static const char *stryesno(bool r) {
    return r ? _("yes") : _("no");
}

void OrpheusTextUI::configuration() {
    dialogbox db;
    int n, b, i;
    bool finished = false;

    string mp3player = conf.getmp3player();
    string oggplayer = conf.getoggplayer();
    string streamplayer = conf.getstreamplayer();
    string cddev = conf.getcddevice();
    string mixerdev = conf.getmixerdevice();
    string proxy = conf.getproxy();
    string radioxml = conf.getradioxml();

    bool autofetch = conf.getautofetch();
    bool russian = conf.getrussian();
    bool autosavepl = conf.getautosaveplaylist();
    bool autoplay = conf.getautoplay();

    PlayMode playmode = conf.getplaymode();
    SortOrder sortorder = conf.getsortorder();
    bool sortasc = conf.getsortasc();

    textwindow w(0, 0, (int) (COLS*0.85), (int) (LINES*0.8),
	schemer[DialogFrames], TW_CENTERED);

    w.set_title(schemer[DialogHighLight], _(" Orpheus configuration "));

    db.setwindow(&w, false);

    db.settree(new treeview(schemer[DialogText],
	schemer[DialogSelected], schemer[DialogHighLight],
	schemer[DialogText]));

    db.setbar(new horizontalbar(schemer[DialogText],
	schemer[DialogSelected], _("Change"), _("Done"), 0));

    db.idle = &dialogidle;
    db.addautokeys();

    treeview &t = *db.gettree();

    while(!finished) {
	t.clear();

	i = t.addnode(_(" MP3 player "));
	t.addleaff(i, 0, 10, _(" Command line : %s "), mp3player.c_str());
	t.addleaff(i, 0, 11, _(" Russian recoding for MP3 tags : %s "), stryesno(russian));

	i = t.addnode(_(" OGG player "));
	t.addleaff(i, 0, 50, _(" Command line : %s "), oggplayer.c_str());

	i = t.addnode(_(" Radio "));
	t.addleaff(i, 0, 70, _(" Command line : %s "), streamplayer.c_str());
#ifdef HAVE_LIBGHTTP
	t.addleaff(i, 0, 71, _(" Presets URL : %s "), radioxml.c_str());
#endif

	i = t.addnode(_(" CD player "));
	t.addleaff(i, 0, 20, _(" Device : %s "), cddev.c_str());

	i = t.addnode(_(" Mixer "));
	t.addleaff(i, 0, 60, _(" Device : %s "), mixerdev.c_str());

#ifdef HAVE_LIBGHTTP
	i = t.addnode(_(" CDDB support "));
	t.addleaff(i, 0, 30, _(" Auto-fetch CD info : %s "), stryesno(autofetch));
	t.addleaff(i, 0, 31, _(" HTTP proxy : %s "), proxy.c_str());
#endif

	i = t.addnode(_(" Miscellaneous "));
	t.addleaff(i, 0, 40, _(" Auto-save current playlist : %s "), stryesno(autosavepl));
	t.addleaff(i, 0, 41, _(" Play mode : %s "), strplaymode(playmode).c_str());
	t.addleaff(i, 0, 42, _(" Auto-play: start playing whenever possible : %s "), stryesno(autoplay));
	t.addleaff(i, 0, 43, _(" Sort track list : %s "), strsortorder(sortorder).c_str());

	if(sortorder != noSort)
	    t.addleaff(i, 0, 44, _(" Sort direction : %s "), sortasc ? "Ascending" : "Descending");

	finished = !db.open(n, b, (void **) &i);

	if(!finished) {
	    if(b == 0) {
		switch(i) {
		    case 10:
			mp3player = inputstr(_("MP3 player command line: "), mp3player);
			break;
		    case 11:
			russian = !russian;
			break;
		    case 20:
			cddev = inputstr(_("CD device filename: "), cddev);
			break;
		    case 30:
			autofetch = !autofetch;
			break;
		    case 31:
			proxy = inputstr(_("HTTP proxy: "), proxy);
			break;
		    case 40:
			autosavepl = !autosavepl;
			break;
		    case 41:
			playmode++;
			if(playmode == PlayMode_size)
			    playmode = Normal;
			break;
		    case 42:
			autoplay = !autoplay;
			break;
		    case 43:
			sortorder++;
			if(sortorder == SortOrder_size)
			    sortorder = byFileName;
			break;
		    case 44:
			sortasc = !sortasc;
			break;
		    case 50:
			oggplayer = inputstr(_("OGG player command line: "), oggplayer);
			break;
		    case 60:
			oggplayer = inputstr(_("Mixer device filename: "), mixerdev);
			break;
		    case 70:
			streamplayer = inputstr(_("Stream player command line: "), streamplayer);
			break;
		    case 71:
			radioxml = inputstr(_("Radio stations presets file: "), radioxml);
			break;
		}
	    } else {
		conf.setmp3player(mp3player);
		conf.setoggplayer(oggplayer);
		conf.setstreamplayer(oggplayer);
		conf.setcddevice(cddev);
		conf.setmixerdevice(mixerdev);
		conf.setproxy(proxy);
		conf.setradioxml(radioxml);
		conf.setautofetch(autofetch);
		conf.setrussian(russian);
		conf.setautosaveplaylist(autosavepl);
		conf.setplaymode(playmode);
		conf.setautoplay(autoplay);
		conf.setsortorder(sortorder);
		conf.setsortasc(sortasc);
		conf.save();
		plist.sort();
		finished = true;
	    }
	}
    }

    db.close();
    update();
}

void OrpheusTextUI::lookfor(string sub) {
    if(plist.empty()) return;

    OrpheusPlayList::iterator nt = plist.begin()+trackm.getpos()+1;
    sub = up(sub);

    while(nt != plist.end()) {
	if(up((*nt)->getdescription()).find(sub) != -1) {
	    string a = (*nt)->getdescription();
	    trackm.setpos(nt-plist.begin());
	    update();
	    break;
	}
	++nt;
    }
}

void OrpheusTextUI::mixer() {
    dialogbox db;
    int n, b, i, width, val, len;
    bool finished = false;
    channeltype ct;
    string bar;

    static map<channeltype, string> cnames;
    static int maxtlen = 0;

    set<channeltype> chavail;
    mixerctl mix(conf.getmixerdevice());

    if(!mix.is_open()) {
	status(_("Cannot open the mixer device: ") + conf.getmixerdevice());
	return;
    }

    if(cnames.empty()) {
	cnames[ctVolume] = _("Volume");
	cnames[ctBass] = _("Bass");
	cnames[ctTreble] = _("Treble");
	cnames[ctSynth] = _("Synth");
	cnames[ctPCM] = _("PCM");
	cnames[ctSpeaker] = _("Speaker");
	cnames[ctLine] = _("Line");
	cnames[ctMic] = _("Microphone");
	cnames[ctCD] = _("CD");
	cnames[ctMix] = _("Mix");
	cnames[ctPCM2] = _("PCM 2");
	cnames[ctRecord] = _("Record");
	cnames[ctInput] = _("Input");
	cnames[ctOutput] = _("Output");
	cnames[ctLine1] = _("Line 1");
	cnames[ctLine2] = _("Line 2");
	cnames[ctLine3] = _("Line 3");
	cnames[ctDigital1] = _("Digital 1");
	cnames[ctDigital2] = _("Digital 2");
	cnames[ctDigital3] = _("Digital 3");
	cnames[ctPhoneIn] = _("Phone In");
	cnames[ctPhoneOut] = _("Phone Out");
	cnames[ctVideo] = _("Video");
	cnames[ctRadio] = _("Radio");
	cnames[ctMonitor] = _("Monitor");

	maxtlen = 0;
	map<channeltype, string>::const_iterator ict = cnames.begin();
	while(ict != cnames.end()) {
	    if(ict->second.size() > maxtlen) maxtlen = ict->second.size();
	    ++ict;
	}
    }

    chavail = mix.getchannels();

    textwindow w(0, 0, (int) (COLS*0.8), (int) (LINES*0.6),
	schemer[DialogFrames], TW_CENTERED);

    w.set_title(schemer[DialogHighLight], _(" Mixer settings "));

    db.setwindow(&w, false);

    db.setmenu(new verticalmenu(schemer[DialogText],
	schemer[DialogSelected]));

    db.setbar(new horizontalbar(schemer[DialogText],
	schemer[DialogSelected], _("Lower"), _("Raise"), 0));

    db.getbar()->item = 1;
    db.addkey('+', 1);
    db.addkey('-', 0);

    db.idle = &dialogidle;
    db.addautokeys();

    verticalmenu &m = *db.getmenu();
    width = w.x2-w.x1-maxtlen-8;

    while(!finished) {
	m.clear();

	for(ct = ctVolume; ct != channeltype_end; ct++) {
	    if(chavail.count(ct)) {
		val = mix.readlevel(ct);
		if(val >= 0) {
		    len = (int) ((double) width/100*val);
		    bar = string(len, '#');
		    bar += string(width-len, ' ');
		    m.additemf(0, (int) ct, _("\001 %s : <\001%s\001> "), (string(maxtlen-cnames[ct].size(), ' ')+cnames[ct]).c_str(), bar.c_str());
		}
	    }
	}

	finished = !db.open(n, b, (void **) &i);

	if(!finished) {
	    ct = (channeltype) i;
	    val = mix.readlevel(ct);

	    switch(b) {
		case 0: if(val) val--; break;
		case 1: if(val < 100) val++; break;
	    }

	    mix.writelevel(ct, val);
	}
    }

    db.close();
}

// ----------------------------------------------------------------------------

int OrpheusTextUI::tracklistkeys(verticalmenu &m, int k) {
    static string sub;
    string fname, plname = plist.getfname();
    int nitems = plist.size();

    switch(k) {
	case KEY_F(2):
	    if(plname.empty()) {
		plname = conf.getbasedir() + "/mytunes/";

		idtrack *idt = 0;

		if(plist.size()) {
		    idt = dynamic_cast<idtrack*>(*(plist.begin()+m.getpos()));
		}

		if(idt) {
		    string artist = idt->getartist();
		    string album = idt->getalbum();
		    string fname = idt->getfname();

		    if(!artist.empty() && !album.empty()) {
			plname += artist + " " + album;
		    } else {
			fname = justpathname(fname);
			int npos = fname.rfind("/");
			if(npos != -1) fname.erase(0, npos+1);
			plname += fname;
		    }

		} else {
		    plname += "noname";
		}

		plname += ".m3u";
	    }

	    fname = tui.inputfile(_("Playlist to save: "), plname, false);
	    if(!fname.empty())
		plist.save(fname, true);
	    break;

	case KEY_F(3):
	    fname = tui.inputfile(_("Playlist to load: "), plname, false);
	    if(!fname.empty()) {
		tui.cleartrack();

		plist.load(fname);
		nitems = 0;

		tui.update();
	    }
	    break;

	case KEY_F(5):
	    cdtrack::readtracks();
	    break;

	case 'b':
	    tui.importRadio();
	    break;

	case 'j':
	case 'J':
	    if(tui.currenttrack)
	    if(dynamic_cast<cdtrack *>(tui.currenttrack))
		tui.currenttrack->stop();

	    cdtrack::eject();
	    break;

	case ']':
	case '}':
	    if(tui.currenttrack)
		tui.currenttrack->fwd(k == '}');
	    break;

	case '[':
	case '{':
	    if(tui.currenttrack)
		tui.currenttrack->rwd(k == '{');
	    break;

	case '+':
	    tui.nexttrack();

	    if(!tui.currenttrack)
	    if(conf.getplaymode() == ListRepeat)
		tui.playtrack(*plist.begin());
	    break;

	case '-':
	    tui.prevtrack();
	    break;

	case KEY_LEFT:
	    if(m.getpos() && plist.size() >= 2) {
		swap<track *>(plist[m.getpos()-1], plist[m.getpos()]);
		m.setpos(m.getpos()-1);
		tui.update();
	    }
	    break;

	case KEY_RIGHT:
	    if(m.getpos() < m.getcount()-1) {
		swap<track *>(plist[m.getpos()+1], plist[m.getpos()]);
		m.setpos(m.getpos()+1);
		tui.update();
	    }
	    break;

	case 'd':
	case 'D':
	case KEY_DC:
	    if(!plist.empty()) {
		OrpheusPlayList::iterator i = plist.begin()+m.getpos();
		if(*i == tui.currenttrack)
		    tui.cleartrack();

		delete *i;
		plist.erase(i);
		tui.update();
	    }
	    break;

	case 'a':
	case 'A':
	case KEY_IC:
	    tui.addfiles();
	    break;

	case 'c':
	case 'C':
	    tui.configuration();
	    break;

	case 'm':
	case 'M':
	    tui.mixer();
	    break;

	case 'S':
	case 's':
	    if(tui.currenttrack) {
		tui.cleartrack();
		tui.update();
	    }
	    break;

	case 'P':
	case 'p':
	case ' ':
	    if(tui.currenttrack) {
		tui.currenttrack->pause();
		tui.update();
	    }
	    break;

	case 'E':
	case 'e':
	    if(!plist.empty())
	    if(plist[m.getpos()]) {
		tui.edittags(plist[m.getpos()]);
		tui.update();
	    }
	    break;

	case '\t':
	    if(tui.recentlists()) nitems = 0;
	    break;

	case '!':
	    tui.cleartrack();
	    plist.clear();
	    tui.update();
	    break;

	case '/':
	    sub = tui.inputstr(_("Look for: "), sub);
	    if(!sub.empty())
		tui.lookfor(sub);
	    break;

	case 'Q':
	case 'q':
	    return 0;
    }

    if(conf.getautoplay())
    if(plist.size() > nitems)
    if(!tui.currenttrack)
	tui.playtrack(plist[nitems]);

    return -1;
}

int OrpheusTextUI::recentlistkeys(verticalmenu &m, int k) {
    switch(k) {
	case 'd':
	case 'D':
	case KEY_DC:
	    if(!recent.empty()) {
		OrpheusRecentLists::iterator i = recent.begin()+m.getpos();
		recent.erase(i);
		return -2;
	    }
	    break;
	case '\t':
	    return 0;
    }

    return -1;
}

#define INPUT_POS       LINES-2

string OrpheusTextUI::inputstr(const string &q, const string &defl, char passwdchar) {
    screenarea sa(0, INPUT_POS, COLS, INPUT_POS);

    attrset(schemer[Status]);
    mvhline(INPUT_POS, 0, ' ', COLS);
    kwriteatf(0, INPUT_POS, schemer[Status], "%s", q.c_str());

    input.setpasswordchar(passwdchar);
    input.removeselector();
    input.setvalue(defl);
    input.setcoords(q.size(), INPUT_POS, COLS-q.size());
    input.exec();

    sa.restore();
    return input.getlastkey() == KEY_ENTER ? defl : input.getvalue();
}

string OrpheusTextUI::inputfile(const string &q, const string &defl, bool multi) {
    screenarea sa(0, INPUT_POS, COLS, INPUT_POS);
    string r;

    textwindow selwindow(0, 0, (int) (COLS*0.65), (int) (LINES*0.7), schemer[DialogFrames], TW_CENTERED);

    if(multi) {
	selector.setoptions(FSEL_MULTI);
	selwindow.set_title(schemer[DialogHighLight], _(" [ <Ins> select; <Space> confirm; <Esc> cancel ] "));
    } else {
	selector.setoptions(0);
	selwindow.set_title(schemer[DialogHighLight], _(" [ <Enter> select; <Esc> cancel ] "));
    }

    selector.setwindow(selwindow);

    mvhline(INPUT_POS, 0, ' ', COLS);
    kwriteatf(0, INPUT_POS, schemer[Status], "%s", q.c_str());
    kwriteatf(COLS-8, INPUT_POS, schemer[Status], "[Ctrl-T]");

    input.connectselector(selector);

    input.setcoords(q.size(), INPUT_POS, COLS-q.size()-8);
    input.setvalue(defl);
    input.exec();

    r = (input.getlastkey() != 13 ? "" : input.getvalue());

    if(r.rfind("/") != -1) {
	selector.setstartpoint(r.substr(0, r.find("/")));
    }

    sa.restore();
    return r;
}

askresult OrpheusTextUI::ask(const string &answersallowed, const string &text) {
    int line = LINES-2;
    screenarea sarea(0, line, COLS, line);
    string msg;
    string::const_iterator c;
    int key;

    attrset(schemer[Status]);
    mvhline(line, 0, ' ', COLS);

    msg = text + " (";
    for(c = answersallowed.begin(); c != answersallowed.end(); c++) {
	msg += c != answersallowed.begin() ? tolower(*c) : toupper(*c);
	if(c != answersallowed.end()-1) msg += '/';
    }

    msg += ") ";
    kwriteat(0, line, msg.c_str(), schemer[Status]);

    while(1) {
	key = toupper(getch());
	if(key == '\r') key = toupper(*answersallowed.begin());
	if(answersallowed.find(key) != -1) break;
    }

    sarea.restore();

    switch(key) {
	case 'Y': return ASK_YES;
	case 'C': return ASK_CANCEL;
	case 'A': return ASK_ALL;
	case 'N': return ASK_NO;
    }

    return ASK_NO;
}

void OrpheusTextUI::sighandler(int sign) {
    switch(sign) {
	case SIGCHLD:
	    while(wait3(0, WNOHANG, 0) > 0);
	    break;
    }
}

void OrpheusTextUI::edittags(track *tr) {
    idtrack *m = dynamic_cast<idtrack *>(tr);

    if(m) {
	dialogbox db;
	int n, b, i;
	bool finished = false;

	textwindow w(0, 0, (int) (COLS*0.85), (int) (LINES*0.6),
	    schemer[DialogFrames], TW_CENTERED);

	w.set_title(schemer[DialogHighLight], _(" Edit ID tags "));

	db.setwindow(&w, false);

	db.settree(new treeview(schemer[DialogText],
	    schemer[DialogSelected], schemer[DialogHighLight],
	    schemer[DialogText]));

	db.setbar(new horizontalbar(schemer[DialogText],
	    schemer[DialogSelected], _("Change"), _("Save"), 0));

	db.idle = &dialogidle;
	db.addautokeys();

	treeview &t = *db.gettree();

	string artist = m->getartist();
	string title = m->gettitle();
	string album = m->getalbum();
	string year = m->getyear();
	string comment = m->getcomment();

	while(!finished) {
	    t.clear();

	    i = t.addnode(_(" File name "));
	    t.addleaff(i, 0, 0, " %s ", m->getfname().c_str());

	    i = t.addnode(_(" Parameters "));
	    t.addleaff(i, 0, 0, " Bit rate : %d ", m->getbitrate());

	    if(m->getfreq())
		t.addleaff(i, 0, 0, " Frequency : %d ", m->getfreq());

	    i = t.addnode(_(" ID3 "));
	    t.addleaff(i, 0, 10, _(" Artist : %s "), artist.c_str());
	    t.addleaff(i, 0, 11, _(" Album : %s "), album.c_str());
	    t.addleaff(i, 0, 12, _(" Title : %s "), title.c_str());
	    t.addleaff(i, 0, 13, _(" Year : %s "), year.c_str());
	    t.addleaff(i, 0, 14, _(" Comment : %s "), comment.c_str());

	    finished = !db.open(n, b, (void **) &i);

	    if(!finished) {
		if(b == 0) {
		    switch(i) {
			case 10:
			    artist = inputstr(_("Artist name: "), artist);
			    break;
			case 11:
			    album = inputstr(_("Album name: "), album);
			    break;
			case 12:
			    title = inputstr(_("Track title: "), title);
			    break;
			case 13:
			    year = inputstr(_("Year: "), year);
			    break;
			case 14:
			    comment = inputstr(_("Track comment: "), comment);
			    break;
		    }
		} else {
		    m->setidtags(artist, title, album, year, comment);
		    finished = true;
		}
	    }
	}

	db.close();
    }
}

void OrpheusTextUI::loadplaylist(const string &lname) {
    cleartrack();
    plist.load(lname);
}

bool OrpheusTextUI::recentlists() {
    int n;

    textwindow rlwindow(0, 0, (int) (COLS*0.7), (int) (LINES*0.7), schemer[DialogFrames], TW_CENTERED);
    rlwindow.set_title(schemer[DialogHighLight], _(" Recent Lists [ <Del> remove; <Enter> load ] "));
    verticalmenu m(schemer[DialogText], schemer[DialogSelected]);
    m.setwindow(rlwindow);

    m.idle = &menuidle;
    m.otherkeys = &recentlistkeys;

    n = -2;

    while(n < 0) {
	m.clear();

	for(OrpheusRecentLists::const_iterator il = recent.begin(); il != recent.end(); ++il)
	    m.additemf(" %s ", il->c_str());

	n = m.open();
    }

    m.close();

    if(!recent.empty())
    if(n) {
	loadplaylist((recent.begin()+n-1)->c_str());
	return true;
    }

    return false;
}

void OrpheusTextUI::importRadio() {
    vector<streamtrack::TopGroup> psets = streamtrack::loadPresets();
    set<int> opennodes, marked;

    dialogbox db;
    int n, b, i, id;
    bool finished = false, added = false;
    string url;

    textwindow w(0, 0, (int) (COLS*0.85), (int) (LINES*0.8),
	schemer[DialogFrames], TW_CENTERED);

    w.set_title(schemer[DialogHighLight], _(" Select radio broadcasts "));

    db.setwindow(&w, false);

    db.settree(new treeview(schemer[DialogText],
	schemer[DialogSelected], schemer[DialogHighLight],
	schemer[DialogText]));

    db.setbar(new horizontalbar(schemer[DialogText],
	schemer[DialogSelected], _("Add URL"), _("Mark/Unmark"), _("Done"), 0));

    db.idle = &dialogidle;
    db.addautokeys();

    db.addkey(KEY_IC, 1);
    db.addkey(' ', 1);

    db.getbar()->item = 1;
    treeview &t = *db.gettree();
    t.collapsable = true;

    while(!finished) {
	if(!t.empty()) {
	    opennodes.clear();

	    for(i = 0; i < t.menu.getcount(); i++) {
		int id = t.getid(i);

		if(t.isnode(id))
		if(t.isnodeopen(id))
		    opennodes.insert(id);
	    }
	}

	t.clear();

	for(vector<streamtrack::TopGroup>::const_iterator it = psets.begin(); it != psets.end(); ++it) {
	    int ntopgroup = t.addnodef(" %s ", it->name.c_str());
	    if(opennodes.count(ntopgroup)) t.opennode(ntopgroup);

	    for(vector<streamtrack::SubGroup>::const_iterator is = it->subgroups.begin(); is != it->subgroups.end(); ++is) {
		int nsubgroup = t.addnodef(ntopgroup, 0, 0, " %s ", is->name.c_str());
		if(opennodes.count(nsubgroup)) t.opennode(nsubgroup);

		for(vector<streamtrack::Station>::const_iterator ist = is->stations.begin(); ist != is->stations.end(); ++ist) {
		    id = nsubgroup+ist-is->stations.begin()+1;

		    t.addleaff(nsubgroup,
			marked.count(id) ? schemer[DialogFrames] : 0,
			(void *) &(*ist), " %s ", ist->name.c_str());
		}
	    }
	}

	finished = !db.open(n, b, (void **) &i);

	if(!finished) {
	    switch(b) {
		case 0:
		    url = inputstr(_("Broadcast URL: "));
		    if(url.find("://") != -1) {
			plist.push_back(new streamtrack(url));
			finished = added = true;
		    }
		    break;

		case 1:
		    id = t.getid(n-1);

		    if(!t.isnode(id)) {
			if(marked.count(id)) marked.erase(id);
			    else marked.insert(id);
			t.setcur(t.getid(n));
		    } else {
			if(t.isnodeopen(id)) t.closenode(id);
			    else t.opennode(id);
		    }
		    break;

		case 2:
		    if(!marked.empty()) {
			for(set<int>::const_iterator im = marked.begin(); im != marked.end(); ++im) {
			    streamtrack::Station *st = (streamtrack::Station *) t.getref(*im);
			    if(!st->urls.empty()) {
				streamtrack *t = new streamtrack(st->urls.front());
				t->station = st->name;
				plist.push_back(t);
			    }
			}

			finished = added = true;
		    }
		    break;
	    }
	}
    }

    db.close();
    if(added) update();
}

void OrpheusTextUI::usage(const string &argv0) {
    cout << _("Usage: ") << argv0 << " [OPTION].." << endl << endl;

    cout << _("  --ascii, -a              use ASCII characters for windows and UI controls") << endl;
    cout << _("  --x-titles, -xt          show track information in xterm title bar") << endl;
    cout << _("  --remote, -r <cmd>       execute a remote command (see README for details)") << endl;
    cout << _("  --help                   display this stuff") << endl;
    cout << _("  --version, -v            show the program version info") << endl;

    cout << endl << _("Report bugs to <orpheus-bugs@konst.org.ua>.") << endl;
}

void OrpheusTextUI::commandline(int argc, char **argv) {
    int i;
    string arg;

    for(i = 1; i < argc; i++) {
	arg = argv[i];

	if(arg == "--ascii" || arg == "-a") {
	    kintf_graph = 0;

	} else if(arg == "--x-titles" || arg == "-xt") {
	    xtitles = true;

	} else if(arg == "--remote" || arg == "-r") {
	    ofstream f((conf.getbasedir() + "/remote").c_str());
	    if(f.is_open()) {
		f << argv[++i];
		f.close();

		ifstream fpid((conf.getbasedir() + "/pid").c_str());
		if(fpid.is_open()) {
		    fpid >> arg;
		    kill(atoi(arg.c_str()), SIGUSR1);
		    fpid.close();
		}
	    }
	    exit(0);

	} else if(arg == "--version" || arg == "-v") {
	    cout << PACKAGE << " " << VERSION << endl
		<< "Written by Konstantin Klyagin." << endl << endl
		<< "This is free software; see the source for copying conditions.  There is NO" << endl
		<< "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << endl;
	    exit(0);

	} else {
	    usage(argv[0]);
	    exit(0);
	}
    }
}
