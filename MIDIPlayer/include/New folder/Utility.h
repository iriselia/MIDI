#pragma once
#include "MIDIFile.h"

unsigned char* loadFile(const char* _fileName, int& _fileSize);

static unsigned int read_var_int(unsigned char* buf, unsigned int* bytesread)
{
	unsigned int var = 0;
	unsigned char c;

	*bytesread = 0;

	do
	{
		c = buf[(*bytesread)++];
		var = (var << 7) + (c & 0x7f); 
	} while (c & 0x80); //1000 0000

	return var;
}

static unsigned short byteSwapShort(unsigned short in)
{
	return ((in << 8) | (in >> 8));
}

static unsigned int byteSwapInt(unsigned int in)
{
	unsigned short *p;
	p = (unsigned short*)&in;

	return ((((unsigned int)byteSwapShort(p[0])) << 16) |
		(unsigned int)byteSwapShort(p[1]));
}

static struct MIDIEvent getNextEvent(const struct MIDITrack* track)
{
	unsigned char* buf;
	struct MIDIEvent e;
	unsigned int bytesread;
	unsigned int dt;

	buf = track->m_pBuffer;

	dt = read_var_int(buf, &bytesread);
	buf += bytesread;

	e.m_absTime = track->m_absTime + dt;
	e.m_pData = buf;
	e.m_event = *e.m_pData;

	return e;
}

static int isTrackEnd(const struct MIDIEvent* e)
{
	if (e->m_event == 0xff) // meta-event?
	if (*(e->m_pData + 1) == 0x2f) // track end?
		return 1;

	return 0;
}