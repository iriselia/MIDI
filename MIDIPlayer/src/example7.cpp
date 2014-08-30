/*
 * example7.c
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

#pragma pack(push, 1)

struct _mid_header {
	unsigned int	id;		// identifier "MThd"
	unsigned int	size;	// always 6 in big-endian format
	unsigned short	format;	// big-endian format
	unsigned short  tracks;	// number of tracks, big-endian
	unsigned short	ticks;	// number of ticks per quarter note, big-endian
};

struct _mid_track {
	unsigned int	id;		// identifier "MTrk"
	unsigned int	length;	// track length, big-endian
};

#pragma pack(pop)

struct trk {
	struct _mid_track* track;
	unsigned char* buf;
	unsigned char last_event;
	unsigned int absolute_time;
};

struct evt {
	unsigned int absolute_time;
	unsigned char* data;
	unsigned char event;
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

static unsigned long read_var_long(unsigned char* buf, unsigned int* bytesread)
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

static unsigned short swap_bytes_short(unsigned short in)
{
	return ((in << 8) | (in >> 8));
}

static unsigned long swap_bytes_long(unsigned long in)
{
	unsigned short *p;
	p = (unsigned short*)&in;

	return (  (((unsigned long)swap_bytes_short(p[0])) << 16) |
				(unsigned long)swap_bytes_short(p[1]));
}

static struct evt get_next_event(const struct trk* track)
{
	unsigned char* buf;
	struct evt e;
	unsigned int bytesread;
	unsigned int time;

	buf = track->buf;

	time = read_var_long(buf, &bytesread);
	buf += bytesread;

	e.absolute_time = track->absolute_time + time;
	e.data = buf;
	e.event = *e.data;

	return e;
}

static int is_track_end(const struct evt* e)
{
	if(e->event == 0xff) // meta-event?
		if(*(e->data + 1) == 0x2f) // track end?
			return 1;

	return 0;
}

static unsigned int get_buffer(unsigned char* buf, unsigned int len, unsigned int** out, unsigned int* outlen)
{
	struct _mid_header* hdr = NULL;
	struct trk* tracks = NULL;

	unsigned short nTracks = 0;
	unsigned int i;

	unsigned int* streambuf = NULL;
	unsigned int streambuflen = 1024;
	unsigned int streamlen = 0;

	unsigned char* tmp = buf;

	unsigned int currTime = 0;

	streambuf = (unsigned int*)malloc(sizeof(unsigned int) * streambuflen);
	memset(streambuf, 0, sizeof(unsigned int) * streambuflen);

	hdr = (struct _mid_header*)tmp;
	tmp += sizeof(struct _mid_header);
	nTracks = swap_bytes_short(hdr->tracks);

	tracks = (struct trk*)malloc(nTracks * sizeof(struct trk));
	for(i = 0; i < nTracks; i++)
	{
		tracks[i].track = (struct _mid_track*)tmp;
		tracks[i].buf = tmp + sizeof(struct _mid_track);
		tracks[i].absolute_time = 0;
		tracks[i].last_event = 0;

		tmp += sizeof(struct _mid_track) + swap_bytes_long(tracks[i].track->length);
	}

	while(TRUE)
	{
		unsigned int time = (unsigned int)-1;
		unsigned char cmd;
		unsigned int msg = 0;

		unsigned int idx = -1;

		struct evt evt;

		// get the next event
		for(i = 0; i < nTracks; i++)
		{
			evt = get_next_event(&tracks[i]);
			if(!(is_track_end(&evt)) && (evt.absolute_time < time))
			{
				time = evt.absolute_time;
				idx = i;
			}
		}

		if(idx == -1)
			break; // we're done

		evt = get_next_event(&tracks[idx]);

		tracks[idx].absolute_time = evt.absolute_time;
		time = tracks[idx].absolute_time - currTime;
		currTime = tracks[idx].absolute_time;

		cmd = *evt.data++;
		if((cmd & 0xf0) != 0xf0) // normal command
		{
			msg = ((unsigned long)cmd) |
				  ((unsigned long)*evt.data++ << 8);

			if(!((cmd & 0xf0) == 0xc0 || (cmd & 0xf0) == 0xd0))
				msg |= *evt.data++ << 16;

			streambuf[streamlen++] = time;
			streambuf[streamlen++] = msg;

			tracks[idx].buf = evt.data;
		}
	}

	*out = streambuf;
	*outlen = streamlen;

	free(tracks);

	return 0;
}

unsigned int example7()
{
	unsigned char* midibuf = NULL;
	unsigned int midilen = 0;

	unsigned char* midi2buf = NULL;
	unsigned int midi2len = 0;

	unsigned int* streambuf = NULL;
	unsigned int streamlen = 0;

	unsigned int* stream2buf = NULL;
	unsigned int stream2len = 0;

	unsigned int err;
	HMIDIOUT out;
	const unsigned int PPQN_CLOCK = 5;
	unsigned int i;

	err = midiOutOpen(&out, 0, 0, 0, CALLBACK_NULL);
	if (err != MMSYSERR_NOERROR)
	   printf("error opening default MIDI device: %d\n", err);
	else
		printf("successfully opened default MIDI device\n");

	midibuf = load_file((unsigned char*)"example7.mid", &midilen);
	midi2buf = load_file((unsigned char*)"example8.mid", &midilen);
	if(midibuf == NULL)
	{
		printf("could not open example7.mid\n");
		return 0;
	}

	get_buffer(midibuf, midilen, &streambuf, &streamlen);
	get_buffer(midi2buf, midi2len, &stream2buf, &stream2len);

	i = 0;
	while (i < streamlen)
	{
	    unsigned int time = streambuf[i++];

		Sleep(time * PPQN_CLOCK);

		err = midiOutShortMsg(out, streambuf[i++]);
		if(err != MMSYSERR_NOERROR)
			printf("error sending command: %d\n", err);
	}

	while (i < stream2len)
	{
		unsigned int time = stream2buf[i++];

		Sleep(time * PPQN_CLOCK);

		err = midiOutShortMsg(out, stream2buf[i++]);
		if (err != MMSYSERR_NOERROR)
			printf("error sending command: %d\n", err);
	}

	midiOutClose(out);
	free(streambuf);
	free(midibuf);

	return 0;
}

