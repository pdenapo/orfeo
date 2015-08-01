#ifndef __CDTRACK_H__
#define __CDTRACK_H__

#include "abstract/track.h"

#include <string>

#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

class cdtrack : public track {
    private:
	int length, number;
	struct cdrom_tocentry toc;
	string title;
	bool paused;

	void playseconds(int offset);
	void readlength();

	static bool loadtracks(const string &fname);
	static string makerequest(const string &cmd);

    public:
	cdtrack(int anumber, int alength = 0)
	    : length(alength), number(anumber), paused(false)
	{
	    char buf[64];
	    sprintf(buf, "CD Track # %02d", anumber);
	    title = buf;

	    if(!length)
		readlength();
	}

	static void readtracks();
	static void cleartracks();
	static void getcddb();
	static void eject();

	static string getcdid();

	void play();
	void pause();
	void stop();

	int getnumber() const { return number; }
	int getlength() const { return length; }

	string getdescription() const;
	vector<string> getstatus() const;

	string gettitle() const { return title; }
	void settitle(const string &atitle) { title = atitle; }

	bool terminated() const;

	void fwd(bool big = false);
	void rwd(bool big = false);

	void toplaylist(ofstream &f) const;
};

#endif
