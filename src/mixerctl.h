#ifndef __MIXERCTL_H__
#define __MIXERCTL_H__

#include <set>
#include <string>

#include "orpheusconf.h"

enum channeltype {
    ctVolume = 0, ctBass, ctTreble, ctSynth, ctPCM, ctSpeaker, ctLine,
    ctMic, ctCD, ctMix, ctPCM2, ctRecord, ctInput, ctOutput, ctLine1,
    ctLine2, ctLine3, ctDigital1, ctDigital2, ctDigital3, ctPhoneIn,
    ctPhoneOut, ctVideo, ctRadio, ctMonitor, channeltype_end
};

ENUM_PLUSPLUS(channeltype)

class mixerctl {
    protected:
	int fd;
	std::string devname;
	std::set<channeltype> channels;

    public:
	mixerctl(): fd(-1)
	    { }
	mixerctl(const std::string &adevname)
	    { devname = adevname; open(); }
	~mixerctl()
	    { close(); }

	void open();
	void close();

	bool is_open() const
	    { return fd > 0; }

	std::set<channeltype> getchannels() const
	    { return channels; }

	int readlevel(channeltype ct);
	void writelevel(channeltype ct, int val);
};

#endif
