/*
 * example2.c
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

unsigned int example2()
{
	unsigned int err;
	HMIDIOUT out;

	err = midiOutOpen(&out, 0, 0, 0, CALLBACK_NULL);
	if (err != MMSYSERR_NOERROR)
	   printf("error opening default MIDI device: %d\n", err);
	else
		printf("successfully opened default MIDI device\n");

	midiOutClose(out);
	return 0;
}

