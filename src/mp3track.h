#ifndef __MP3TRACK_H__
#define __MP3TRACK_H__

#include "abstract/track.h"

class mp3track : public idtrack {
    private:
	int length;

	mutable bool aterminated;
	mutable int currentpos;

	void readtag();
	bool ismpegheader(unsigned char *h) const;

	static void runplayer();
	static void sendcommand(const char *fmt, ...);

    public:
	mp3track(const string &afname);
	~mp3track();

	void play();
	void pause();
	void stop();

	void fwd(bool big = false);
	void rwd(bool big = false);

	bool terminated() const;

	int getlength() const;
	string getdescription() const;
	vector<string> getstatus() const;

	void setidtags(string &aartist, string &atitle, string &aalbum, string &ayear, string &acomment);
	void toplaylist(ofstream &f) const;

	static void kill();
};

#endif
