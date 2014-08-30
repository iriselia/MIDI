/*
 * example5.c
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

unsigned int example5()
{
	unsigned int err;
	HMIDIOUT out;
	const unsigned int PPQN_CLOCK = 5;
	unsigned int i;
	unsigned int cmds[] = {
			0x007f3c90,
			0x60003c90,
			0x007f3e90,
			0x60003e90,
			0x007f4090,
			0x60004090,
			0x007f4190,
			0x60004190,
			0x007f4390,
			0x60004390,
			0x007f4590,
			0x60004590,
			0x007f4790,
			0x60004790,
			0x007f4890,
			0x60004890,
			0};

	err = midiOutOpen(&out, 0, 0, 0, CALLBACK_NULL);
	if (err != MMSYSERR_NOERROR)
	   printf("error opening default MIDI device: %d\n", err);
	else
		printf("successfully opened default MIDI device\n");

	i = 0;
	while(cmds[i])
	{
	    unsigned int time = cmds[i] >> 24;

		Sleep(time * PPQN_CLOCK);

		err = midiOutShortMsg(out, cmds[i]);
		if(err != MMSYSERR_NOERROR)
			printf("error sending command: %d\n", err);

		i++;
	}

	midiOutClose(out);
	return 0;
}


