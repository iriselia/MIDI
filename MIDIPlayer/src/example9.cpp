/*
 * example9.c
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

#define MAX_STREAM_BUFFER_SIZE (512 * 12)
HANDLE event;

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

static void CALLBACK example9_callback(HMIDIOUT out, UINT msg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    switch (msg)
    {
	case MOM_DONE:
 		SetEvent(event);
		break;
	case MOM_POSITIONCB:
	case MOM_OPEN:
	case MOM_CLOSE:
		break;
    }
}

static unsigned int get_buffer(struct MIDITrack* tracks, unsigned int ntracks, unsigned int* out, unsigned int* outlen)
{
	MIDIEVENT e, *p;
	unsigned int streamlen = 0;
	unsigned int i;
	static unsigned int current_time = 0;

	if(tracks == NULL || out == NULL || outlen == NULL)
		return 0;

	*outlen = 0;

	while(TRUE)
	{
		unsigned int time = (unsigned int)-1;
		unsigned int idx = -1;
		MIDIEvent MIDIEvent;
		unsigned char c;

		if(((streamlen + 3) * sizeof(unsigned int)) >= MAX_STREAM_BUFFER_SIZE)
			break;

		// get the next event
		for(i = 0; i < ntracks; i++)
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

		e.dwStreamID = 0; // always 0

		MIDIEvent = getNextEvent(&tracks[idx]);

		tracks[idx].m_absTime = MIDIEvent.m_absTime;
		e.dwDeltaTime = tracks[idx].m_absTime - current_time;
		current_time = tracks[idx].m_absTime;

		if(!(MIDIEvent.m_event & 0x80)) // running mode
		{
			unsigned char last = tracks[idx].m_lastEvent;
			c = *MIDIEvent.m_pData++; // get the first data byte
			e.dwEvent = ((unsigned long)MEVT_SHORTMSG << 24) |
						((unsigned long)last) |
						((unsigned long)c << 8);
			if(!((last & 0xf0) == 0xc0 || (last & 0xf0) == 0xd0))
			{
				c = *MIDIEvent.m_pData++; // get the second data byte
				e.dwEvent |= ((unsigned long)c << 16);
			}

			p = (MIDIEVENT*)&out[streamlen];
			*p = e;

			streamlen += 3;

			tracks[idx].m_pBuffer = MIDIEvent.m_pData;
		}
		else if(MIDIEvent.m_event == 0xff) // meta-event
		{
			MIDIEvent.m_pData++; // skip the event byte
			unsigned char meta = *MIDIEvent.m_pData++; // read the meta-event byte
			unsigned int len;

			switch(meta)
			{
			case 0x51: // only care about tempo events
				{
					unsigned char a, b, c;
					len = *MIDIEvent.m_pData++; // get the length byte, should be 3
					a = *MIDIEvent.m_pData++;
					b = *MIDIEvent.m_pData++;
					c = *MIDIEvent.m_pData++;

					e.dwEvent = ((unsigned long)MEVT_TEMPO << 24) |
								((unsigned long)a << 16) |
								((unsigned long)b << 8) |
								((unsigned long)c << 0);

					p = (MIDIEVENT*)&out[streamlen];
					*p = e;

					streamlen += 3;
				}
				break;
			default: // skip all other meta events
				len = *MIDIEvent.m_pData++; // get the length byte
				MIDIEvent.m_pData += len;
				break;
			}

			tracks[idx].m_pBuffer = MIDIEvent.m_pData;
		}
		else if((MIDIEvent.m_event & 0xf0) != 0xf0) // normal command
		{
			tracks[idx].m_lastEvent = MIDIEvent.m_event;
			MIDIEvent.m_pData++; // skip the event byte
			c = *MIDIEvent.m_pData++; // get the first data byte
			e.dwEvent = ((unsigned long)MEVT_SHORTMSG << 24) |
						((unsigned long)MIDIEvent.m_event << 0) |
						((unsigned long)c << 8);
			if(!((MIDIEvent.m_event & 0xf0) == 0xc0 || (MIDIEvent.m_event & 0xf0) == 0xd0))
			{
				c = *MIDIEvent.m_pData++; // get the second data byte
				e.dwEvent |= ((unsigned long)c << 16);
			}

			p = (MIDIEVENT*)&out[streamlen];
			*p = e;

			streamlen += 3;

			tracks[idx].m_pBuffer = MIDIEvent.m_pData;
		}

	}

	*outlen = streamlen * sizeof(unsigned int);

	return 1;
}

unsigned int example9()
{
	unsigned char* midibuf = NULL;
	unsigned int midilen = 0;

	struct MIDIHeaderInfo* hdr = NULL;

	unsigned int i;

	unsigned short ntracks = 0;
	struct MIDITrack* tracks = NULL;

	unsigned int streambufsize = MAX_STREAM_BUFFER_SIZE;
	unsigned int* streambuf = NULL;
	unsigned int streamlen = 0;

	HMIDISTRM out;
	MIDIPROPTIMEDIV prop;
	MIDIHDR mhdr;
	unsigned int device = 0;

	midibuf = load_file((unsigned char*)"LTE111.mid", &midilen);
	if(midibuf == NULL)
	{
		printf("could not open example9.mid\n");
		return 0;
	}

	hdr = (struct MIDIHeaderInfo*)midibuf;
	midibuf += sizeof(struct MIDIHeaderInfo);
	ntracks = byteSwapShort(hdr->tracks);

	tracks = (struct MIDITrack*)malloc(ntracks * sizeof(struct MIDITrack));
	if(tracks == NULL)
		goto error1;

	for(i = 0; i < ntracks; i++)
	{
		tracks[i].m_pTrackInfo = (struct MIDITrackInfo*)midibuf;
		tracks[i].m_pBuffer = midibuf + sizeof(struct MIDITrackInfo);
		tracks[i].m_absTime = 0;
		tracks[i].m_lastEvent = 0;

		midibuf += sizeof(struct MIDITrackInfo) + byteSwapInt(tracks[i].m_pTrackInfo->length);
	}

	streambuf = (unsigned int*)malloc(sizeof(unsigned int) * streambufsize);
	if(streambuf == NULL)
		goto error2;

	memset(streambuf, 0, sizeof(unsigned int) * streambufsize);

    if ((event = CreateEvent(0, FALSE, FALSE, 0)) == NULL)
    	goto error3;

	if (midiStreamOpen(&out, &device, 1, (DWORD)example9_callback, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
		goto error4;

	prop.cbStruct = sizeof(MIDIPROPTIMEDIV);
	prop.dwTimeDiv = byteSwapShort(hdr->ticks);
	if(midiStreamProperty(out, (LPBYTE)&prop, MIDIPROP_SET|MIDIPROP_TIMEDIV) != MMSYSERR_NOERROR)
		goto error5;

	mhdr.lpData = (char*)streambuf;
	mhdr.dwBufferLength = mhdr.dwBytesRecorded = streambufsize;
	mhdr.dwFlags = 0;

	if(midiOutPrepareHeader((HMIDIOUT)out, &mhdr, sizeof(MIDIHDR)) != MMSYSERR_NOERROR)
		goto error5;

	if(midiStreamRestart(out) != MMSYSERR_NOERROR)
		goto error6;

	printf("buffering...\n");
	get_buffer(tracks, ntracks, streambuf, &streamlen);
	while(streamlen > 0)
	{
		mhdr.dwBytesRecorded = streamlen;

		if(midiStreamOut(out, &mhdr, sizeof(MIDIHDR)) != MMSYSERR_NOERROR)
			goto error7;

		WaitForSingleObject(event, INFINITE);

		printf("buffering...\n");
		get_buffer(tracks, ntracks, streambuf, &streamlen);
	}
	printf("done.\n");

error7:
	midiOutReset((HMIDIOUT)out);

error6:
	midiOutUnprepareHeader((HMIDIOUT)out, &mhdr, sizeof(MIDIHDR));

error5:
	midiStreamClose(out);

error4:
	CloseHandle(event);

error3:
	free(streambuf);

error2:
	free(tracks);

error1:
	free(hdr);

	return(0);
}

