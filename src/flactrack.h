#ifndef __FLACTRACK_H__
#define __FLACTRACK_H__

#include "abstract/track.h"

class flactrack : public idtrack {
    private:
	bool paused;

	mutable int length, currentpos;
	mutable string sbuffer;

	void readtag();
	void runplayer();

    public:
	flactrack(const string &afname);
	~flactrack();

	void play();
	void pause();
	void stop();

	void fwd(bool big = false) {}
	void rwd(bool big = false) {}

	bool terminated() const;

	int getlength() const;
	string getdescription() const;
	vector<string> getstatus() const;

	void setidtags(string &aartist, string &atitle, string &aalbum, string &ayear, string &acomment);
	void toplaylist(ofstream &f) const;
};

#endif
