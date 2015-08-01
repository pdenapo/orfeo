#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__

#include "abstract/track.h"

#include <vector>
#include <string>

class OrpheusPlayList : public vector<track *> {
    private:
	string fname;
	static short compare(const track *t1, const track *t2);

    public:
	bool save(const string &afname, bool askifany = false);
	bool load(const string &afname);

	void clear();
	void sort();

	string getfname() const { return fname; }
};

class OrpheusRecentLists : public vector<string> {
    public:
	void load();
	void save() const;
	void tweak(const string &afname);
};

extern OrpheusPlayList plist;
extern OrpheusRecentLists recent;

#endif
