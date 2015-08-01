/*
*
* orpheus sound mixer controller class implementation
* $Id: mixerctl.cc,v 1.4 2006/05/13 09:14:28 konst Exp $
*
* Copyright (C) 2002-2004 by Konstantin Klyagin <konst@konst.org.ua>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
* USA
*
*/

#include "mixerctl.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <config.h>

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#else
#ifdef HAVE_SOUNDCARD_H 
#include <soundcard.h>
#else
#ifdef HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#endif
#endif
#endif

using namespace std;

void mixerctl::open() {
    int devmask;
    channeltype ct;

    if((fd = ::open(devname.c_str(), O_RDWR)) != -1) {
	if(ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devmask) != -1) {
	    for(ct = ctVolume; ct != channeltype_end; ct++) {
		if((1 << (int) ct) & devmask) {
		    channels.insert(ct);
		}
	    }
	} else {
	    close();
	}
    }
}

void mixerctl::close() {
    if(fd > 0) {
	::close(fd);
	fd = -1;
    }
}

int mixerctl::readlevel(channeltype ct) {
    int r, left, right;
    ioctl(fd, MIXER_READ((int) ct), &r);

    left = r & 0x7F;
    right = (r >> 8) & 0x7F;
    r = (left > right) ? left : right;

    return r;
}

void mixerctl::writelevel(channeltype ct, int val) {
    val = (val << 8) | val;
    ioctl(fd, MIXER_WRITE((int) ct), &val);
}
