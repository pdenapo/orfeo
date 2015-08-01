#ifndef __UITEXT_H__
#define __UITEXT_H__

#include "abstract/userinterface.h"
#include "abstract/track.h"

#include "textwindow.h"
#include "cmenus.h"
#include "colorschemer.h"
#include "fileselector.h"
#include "textinputline.h"
#include "dialogbox.h"

class OrpheusTextUI : public OrpheusUserInterface {
    protected:
	enum ElementColorType {
	    Status = 1,

	    NormalText,
	    Frames,
	    HighLight,

	    DialogText,
	    DialogFrames,
	    DialogHighLight,
	    DialogSelected
	};

	textwindow mainw;
	verticalmenu trackm;
	fileselector selector;
	textinputline input;

	bool xtitles;

	static bool blocked, dotermresize;

	colorschemer<ElementColorType> schemer;

	track *currenttrack;

	static void sighandler(int sign);

	void redraw();
	void loadcolors();
	void init();
	void showhelp();
	void addfiles();

	void addtrackf(const string &fname);

	string inputstr(const string &q, const string &defl = "", char passwdchar = 0);
	string inputfile(const string &q, const string &defl = "", bool multi = true);

	void playtrack(track *t);
	void cleartrack();
	void play(int n);

	void configuration();
	void mixer();
	void lookfor(string sub);
	bool recentlists();
	void loadplaylist(const string &lname);

	void importRadio();

	static int tracklistkeys(verticalmenu &m, int k);
	static int recentlistkeys(verticalmenu &m, int k);

	static void tracksidle(verticalmenu &m);
	static void selectoridle(fileselector &caller);
	static void dialogidle(dialogbox &db);
	static void textinputidle(textinputline &line);
	static void menuidle(verticalmenu &line);

	static void termresize(void);

	void edittags(track *t);

	void generalidle();
	void generalidle(vector<string> &st);

	void usage(const string &argv0);

    public:
	OrpheusTextUI() : currenttrack(0), xtitles(false) { }

	int exec();
	void update();
	void status(const string &s);
	void commandline(int argc, char **argv);
	void nexttrack();
	void prevtrack();
	void doadd(const string &aitem);

	askresult ask(const string &answersallowed, const string &text);
};

extern OrpheusTextUI tui;

#endif
