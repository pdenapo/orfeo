#ifndef __STREAMTRACK_H__
#define __STREAMTRACK_H__

#include "abstract/track.h"

#include <map>

class streamtrack: public track {
    public:
	struct Station {
	    Station(const string &aname): name(aname) {}
	    string name;
	    vector<string> urls;
	};

	struct SubGroup {
	    SubGroup(const string &aname): name(aname) {}
	    string name;
	    vector<Station> stations;
	};

	struct TopGroup {
	    TopGroup(const string &aname): name(aname) {}
	    string name;
	    vector<SubGroup> subgroups;
	};

	mutable string station;

    private:
	string url;
	bool paused;

	mutable string title, genre, bitrate, sbuffer;

    private:
	void runplayer();
	static string makerequest(const string &url);

    public:
	streamtrack(const string &aurl);
	~streamtrack();

	void play();
	void pause();
	void stop();

	bool terminated() const;

	int getlength() const { return -1; }
	string getdescription() const;
	vector<string> getstatus() const;
	const string getURL() const { return url; }

	void toplaylist(ofstream &f) const;

	static vector<TopGroup> loadPresets();
	static map<string, string> loadPresetNames();
};

#endif
