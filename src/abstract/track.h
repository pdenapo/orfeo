#ifndef __TRACK_H__
#define __TRACK_H__

#include "orpheusconf.h"

#include <vector>
#include <fstream>

class track {
    public:
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;

	virtual bool terminated() const = 0;

	virtual int getlength() const = 0;
	virtual string getdescription() const = 0;
	virtual vector<string> getstatus() const = 0;

	virtual void fwd(bool big = false) { }
	virtual void rwd(bool big = false) { }

	virtual void toplaylist(ofstream &f) const { }
};

class idtrack: public track {
    protected:
	time_t lastmod;
	string fname, artist, title, album, year, comment;
	int bitrate, freq;

    public:
	idtrack(): freq(0), bitrate(0), lastmod(0) { }
	virtual ~idtrack() { }

	virtual string getfname() const { return fname; }
	virtual string getartist() const { return artist; }
	virtual string gettitle() const { return title; }
	virtual string getalbum() const { return album; }
	virtual string getyear() const { return year; }
	virtual string getcomment() const { return comment; }

	virtual time_t getlastmod() const { return lastmod; }
	virtual int getbitrate() const { return bitrate; }
	virtual int getfreq() const { return freq; }

	virtual void setidtags(string &aartist, string &atitle, string &aalbum, string &ayear, string &acomment) { }
};

#endif
