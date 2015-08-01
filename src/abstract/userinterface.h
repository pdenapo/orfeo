#ifndef __USERINTERFACE_H__
#define __USERINTERFACE_H__

#include "orpheusconf.h"

#include <stdio.h>
#include <stdarg.h>

enum askresult {
    ASK_YES, ASK_NO, ASK_CANCEL, ASK_ALL
};

class OrpheusUserInterface {
    public:
	virtual int exec() = 0;
	virtual void update() = 0;
	virtual void nexttrack() = 0;
	virtual void prevtrack() = 0;
	virtual void doadd(const string &aitem) = 0;
	virtual void loadplaylist(const string &lname) = 0;
	virtual void play(int n) = 0;

	virtual void commandline(int argc, char **argv) { }
	virtual void status(const string &s) { }

	void statusf(const char *fmt, ...) {
	    char buf[512];
	    va_list ap;
	    va_start(ap, fmt);
	    vsprintf(buf, fmt, ap);
	    va_end(ap);
	    status(buf);
	}

	virtual askresult ask(const string &answersallowed, const string &text) { return ASK_CANCEL; }

	virtual askresult askf(const string &answersallowed, const char *fmt, ...) {
	    char buf[512];
	    va_list ap;
	    va_start(ap, fmt);
	    vsprintf(buf, fmt, ap);
	    va_end(ap);

	    return ask(answersallowed, buf);
	}
};

extern OrpheusUserInterface &ui;

#endif
