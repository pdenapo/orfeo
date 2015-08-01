#ifndef __ORPHEUSCONF_H__
#define __ORPHEUSCONF_H__

#include <string>
#include <config.h>

using namespace std;

#ifdef ENABLE_NLS

#include <libintl.h>
#define _(s)    gettext(s)

#else

#define _(s)    (s)

#endif

#define ENUM_PLUSPLUS(tp) \
    inline tp& operator++(tp &p, int) { \
	int t = p; \
	return p = static_cast<tp>(++t); \
    }

enum PlayMode {
    Normal = 0,
    TrackRepeat,
    ListRepeat,
    Random,
    PlayMode_size
};

ENUM_PLUSPLUS(PlayMode)

enum SortOrder {
    byFileName = 0,
    byTrackName,
    byTrackLength,
    byLastModified,
    noSort,
    SortOrder_size
};

ENUM_PLUSPLUS(SortOrder)

class OrpheusConfiguration {
    private:
	string mp3player, oggplayer, streamplayer, cddev, mixerdev, proxy, dname, radioxml;
	bool cdautofetch, russian, autosavepl, autoplay, sortasc;
	PlayMode playmode;
	SortOrder sortorder;

    public:
	OrpheusConfiguration();
	~OrpheusConfiguration();

	void load();
	void save() const;

	string getbasedir() const { return dname; }

	string getmp3player() const { return mp3player; }
	void setmp3player(const string &amp3player) { mp3player = amp3player; }

	string getoggplayer() const { return oggplayer; }
	void setoggplayer(const string &aoggplayer) { oggplayer = aoggplayer; }

	string getstreamplayer() const { return streamplayer; }
	void setstreamplayer(const string &astreamplayer) { streamplayer = astreamplayer; }

	string getcddevice() const { return cddev; }
	void setcddevice(const string &adevice) { cddev = adevice; }

	string getmixerdevice() const { return mixerdev; }
	void setmixerdevice(const string &adevice) { mixerdev = adevice; }

	string getproxy() const { return proxy; }
	void setproxy(const string &aproxy) { proxy = aproxy; }

	string getradioxml() const { return radioxml; }
	void setradioxml(const string &aradioxml) { radioxml = aradioxml; }

	bool getautofetch() const { return cdautofetch; }
	void setautofetch(bool aautofetch) { cdautofetch = aautofetch; }

	bool getrussian() const { return russian; }
	void setrussian(bool arussian) { russian = arussian; }

	bool getautosaveplaylist() const { return autosavepl; }
	void setautosaveplaylist(bool aautosavepl) { autosavepl = aautosavepl; }

	bool getautoplay() const { return autoplay; }
	void setautoplay(bool aautoplay) { autoplay = aautoplay; }

	PlayMode getplaymode() const { return playmode; }
	void setplaymode(PlayMode m) { playmode = m; }

	SortOrder getsortorder() const { return sortorder; }
	void setsortorder(SortOrder m) { sortorder = m; }

	bool getsortasc() const { return sortasc; }
	void setsortasc(bool m) { sortasc = m; }
};

extern OrpheusConfiguration conf;

string rusconv(const string &tdir, const string &text);
string strplaymode(PlayMode mode);
string strsortorder(SortOrder order);
string up(string s);

#endif
