/*
 * example8.c
 *
 *  Created on: Dec 21, 2011
 *      Author: David J. Rager
 *       Email: djrager@fourthwoods.com
 *
 * This code is hereby released into the public domain per the Creative Commons
 * Public Domain dedication.
 *
 * http://http://creativecommons.org/publicdomain/zero/1.0/
 */
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

#define TEMPO_EVT 1

#pragma pack(push, 1)

struct MIDIHeaderInfo {
	unsigned int	id;		// identifier "MThd"
	unsigned int	size;	// always 6 in big-endian format
	unsigned short	format;	// big-endian format
	unsigned short  tracks;	// number of tracks, big-endian
	unsigned short	ticks;	// number of ticks per quarter note, big-endian
};

struct MIDITrackInfo {
	unsigned int	id;		// identifier "MTrk"
	unsigned int	length;	// track length, big-endian
};

#pragma pack(pop)

struct MIDITrack {
	struct MIDITrackInfo* m_pTrackInfo;
	unsigned char* m_pBuffer;
	unsigned char m_lastEvent;
	unsigned int m_absTime;
};

struct MIDIEvent {
	unsigned int m_absTime;
	unsigned char* m_pData;
	unsigned char m_event;
};

static unsigned char* load_file(const unsigned char* filename, unsigned int* len)
{
	unsigned char* buf;
	unsigned int ret;
	FILE* f = fopen((char*)filename, "rb");
	if(f == NULL)
		return 0;

	fseek(f, 0, SEEK_END);
	*len = ftell(f);
	fseek(f, 0, SEEK_SET);

	buf = (unsigned char*)malloc(*len);
	if(buf == 0)
	{
		fclose(f);
		return 0;
	}

	ret = fread(buf, 1, *len, f);
	fclose(f);

	if(ret != *len)
	{
		free(buf);
		return 0;
	}

	return buf;
}

static unsigned long read_var_int(unsigned char* buf, unsigned int* bytesread)
{
	unsigned long var = 0;
	unsigned char c;

	*bytesread = 0;

	do
	{
		c = buf[(*bytesread)++];
		var = (var << 7) + (c & 0x7f);
	}
	while(c & 0x80);

	return var;
}

static unsigned short byteSwapShort(unsigned short in)
{
	return ((in << 8) | (in >> 8));
}

static unsigned long byteSwapInt(unsigned long in)
{
	unsigned short *p;
	p = (unsigned short*)&in;

	return (  (((unsigned long)byteSwapShort(p[0])) << 16) |
				(unsigned long)byteSwapShort(p[1]));
}

static struct MIDIEvent getNextEvent(const struct MIDITrack* track)
{
	unsigned char* buf;
	struct MIDIEvent e;
	unsigned int bytesread;
	unsigned int time;

	buf = track->m_pBuffer;

	time = read_var_int(buf, &bytesread);
	buf += bytesread;

	e.m_absTime = track->m_absTime + time;
	e.m_pData = buf;
	e.m_event = *e.m_pData;

	return e;
}

static int isTrackEnd(const struct MIDIEvent* e)
{
	if(e->m_event == 0xff) // meta-event?
		if(*(e->m_pData + 1) == 0x2f) // track end?
			return 1;

	return 0;
}

static unsigned int get_buffer(unsigned char* midibuf, unsigned int midilen, unsigned int** out, unsigned int* outlen)
{
	struct MIDIHeaderInfo* hdr = NULL;
	struct MIDITrack* tracks = NULL;

	unsigned short nTracks = 0;
	unsigned int i;

	unsigned int* streambuf = NULL;
	unsigned int streambuflen = 1024;
	unsigned int streamlen = 0;

	unsigned int currTime = 0;

	*out = NULL;
	*outlen = 0;

	streambuf = (unsigned int*)malloc(sizeof(unsigned int) * streambuflen);
	memset(streambuf, 0, sizeof(unsigned int) * streambuflen);

	hdr = (struct MIDIHeaderInfo*)midibuf;
	midibuf += sizeof(struct MIDIHeaderInfo);
	nTracks = byteSwapShort(hdr->tracks);

	tracks = (struct MIDITrack*)malloc(nTracks * sizeof(struct MIDITrack));
	for(i = 0; i < nTracks; i++)
	{
		tracks[i].m_pTrackInfo = (struct MIDITrackInfo*)midibuf;
		tracks[i].m_pBuffer = midibuf + sizeof(struct MIDITrackInfo);
		tracks[i].m_absTime = 0;
		tracks[i].m_lastEvent = 0;

		midibuf += sizeof(struct MIDITrackInfo) + byteSwapInt(tracks[i].m_pTrackInfo->length);
	}

	while(TRUE)
	{
		unsigned int time = (unsigned int)-1;
		unsigned int msg = 0;

		unsigned int idx = -1;

		struct MIDIEvent MIDIEvent;

		// get the next event
		for(i = 0; i < nTracks; i++)
		{
			MIDIEvent = getNextEvent(&tracks[i]);
			if(!(isTrackEnd(&MIDIEvent)) && (MIDIEvent.m_absTime < time))
			{
				time = MIDIEvent.m_absTime;
				idx = i;
			}
		}

		// if idx == -1 then all the tracks have been read up to the end of track mark
		if(idx == -1)
			break; // we're done

		MIDIEvent = getNextEvent(&tracks[idx]);

		tracks[idx].m_absTime = MIDIEvent.m_absTime;
		time = tracks[idx].m_absTime - currTime;
		currTime = tracks[idx].m_absTime;

		if(MIDIEvent.m_event == 0xff) // meta-event
		{
			MIDIEvent.m_pData++; // skip the event byte
			unsigned char meta = *MIDIEvent.m_pData++; // read the meta-event byte
			unsigned int len;

			switch(meta)
			{
			case 0x51:
				{
					unsigned char a, b, c;
					len = *MIDIEvent.m_pData++; // get the length byte, should be 3
					a = *MIDIEvent.m_pData++;
					b = *MIDIEvent.m_pData++;
					c = *MIDIEvent.m_pData++;

					msg = ((unsigned long)TEMPO_EVT << 24) |
						  ((unsigned long)a << 16) |
						  ((unsigned long)b << 8) |
						  ((unsigned long)c << 0);

					while(streamlen + 2 > streambuflen)
					{
						unsigned int* tmp = NULL;
						streambuflen *= 2;
						tmp = (unsigned int*)realloc(streambuf, sizeof(unsigned int) * streambuflen);
						if(tmp != NULL)
						{
							streambuf = tmp;
						}
						else
						{
							goto error;
						}
					}
					streambuf[streamlen++] = time;
					streambuf[streamlen++] = msg;
				}
				break;
			case 0x00:
			case 0x01:
			case 0x02:
			case 0x03:
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
			case 0x21:
			case 0x2f: // end of track
			case 0x54:
			case 0x58: // time signature
			case 0x59: // key signature
			case 0x7f:
			default:
				len = *MIDIEvent.m_pData++; // ignore event, skip all data
				MIDIEvent.m_pData += len;
				break;
			}

			tracks[idx].m_pBuffer = MIDIEvent.m_pData;
		}
		else if(!(MIDIEvent.m_event & 0x80)) // running mode
		{
			unsigned char last = tracks[idx].m_lastEvent;
			msg = ((unsigned long)last) |
				((unsigned long)*MIDIEvent.m_pData++ << 8);

			if(!((last & 0xf0) == 0xc0 || (last & 0xf0) == 0xd0))
				msg |= ((unsigned long)*MIDIEvent.m_pData++ << 16);

			while(streamlen + 2 > streambuflen)
			{
				unsigned int* tmp = NULL;
				streambuflen *= 2;
				tmp = (unsigned int*)realloc(streambuf, sizeof(unsigned int) * streambuflen);
				if(tmp != NULL)
				{
					streambuf = tmp;
				}
				else
				{
					goto error;
				}
			}

			streambuf[streamlen++] = time;
			streambuf[streamlen++] = msg;

			tracks[idx].m_pBuffer = MIDIEvent.m_pData;
		}
		else if ((MIDIEvent.m_event & 0xf0) != 0xf0) // normal command
		{
			tracks[idx].m_lastEvent = MIDIEvent.m_event;
			MIDIEvent.m_pData++; // skip the event byte
			msg = ((unsigned long)MIDIEvent.m_event) |
				((unsigned long)*MIDIEvent.m_pData++ << 8);

			if (!((MIDIEvent.m_event & 0xf0) == 0xc0 || (MIDIEvent.m_event & 0xf0) == 0xd0))
				msg |= ((unsigned long)*MIDIEvent.m_pData++ << 16);

			while(streamlen + 2 > streambuflen)
			{
				unsigned int* tmp = NULL;
				streambuflen *= 2;
				tmp = (unsigned int*)realloc(streambuf, sizeof(unsigned int) * streambuflen);
				if(tmp != NULL)
				{
					streambuf = tmp;
				}
				else
				{
					goto error;
				}
			}

			streambuf[streamlen++] = time;
			streambuf[streamlen++] = msg;

			tracks[idx].m_pBuffer = MIDIEvent.m_pData;
		}
		else
		{
			// not handling sysex events yet
			printf("unknown event %2x", MIDIEvent.m_event);
			exit(1);
		}

	}

	*out = streambuf;
	*outlen = streamlen;

	free(tracks);

	return 0;

error:
	if(streambuf)
		free(streambuf);

	free(tracks);

	return -1;
}

static void usleep(int waitTime) {
    LARGE_INTEGER time1, time2, freq;

    if(waitTime == 0)
    	return;

    QueryPerformanceCounter(&time1);
    QueryPerformanceFrequency(&freq);

    do {
        QueryPerformanceCounter(&time2);
    } while((time2.QuadPart - time1.QuadPart) * 1000000ll / freq.QuadPart < waitTime);
}

unsigned int example8()
{
	unsigned char* midibuf = NULL;
	unsigned int midilen = 0;

	unsigned int* streambuf = NULL;
	unsigned int streamlen = 0;

	unsigned int err, msg;
	HMIDIOUT out;
	unsigned int PPQN_CLOCK;
	unsigned int i;

	struct MIDIHeaderInfo* hdr;

	err = midiOutOpen(&out, 0, 0, 0, CALLBACK_NULL);
	if (err != MMSYSERR_NOERROR)
	   printf("error opening default MIDI device: %d\n", err);
	else
		printf("successfully opened default MIDI device\n");

	midibuf = load_file((unsigned char*)"example8.mid", &midilen);
	if(midibuf == NULL)
	{
		printf("could not open example8.mid\n");
		return 0;
	}

	hdr = (struct MIDIHeaderInfo*)midibuf;

	PPQN_CLOCK = 1 / byteSwapShort(hdr->ticks);

	get_buffer(midibuf, midilen, &streambuf, &streamlen);

	i = 0;
	while(i < streamlen)
	{
	    unsigned int time = streambuf[i++];

		usleep(time * PPQN_CLOCK);

		msg = streambuf[i++];
		if(msg & 0xff000000) // tempo change
		{
			msg = msg & 0x00ffffff;
			PPQN_CLOCK = msg / byteSwapShort(hdr->ticks);
		}
		else
		{
			err = midiOutShortMsg(out, msg);
			if(err != MMSYSERR_NOERROR)
				printf("error sending command: %08x error: %d\n", msg, err);
		}
	}

	midiOutClose(out);
	free(streambuf);
	free(midibuf);

	return 0;
}
