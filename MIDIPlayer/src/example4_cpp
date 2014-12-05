/*
 * example4.c
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

unsigned int example4()
{
	unsigned int err;
	HMIDIOUT out;

	err = midiOutOpen(&out, 0, 0, 0, CALLBACK_NULL);
	if (err != MMSYSERR_NOERROR)
	{
	   printf("error opening default MIDI device: %d\n", err);
	   return 0;
	}
	else
		printf("successfully opened default MIDI device\n");

	midiOutShortMsg(out, 0x00403C90);
	midiOutShortMsg(out, 0x00404090);
	midiOutShortMsg(out, 0x00404390);

	Sleep(1000);

	midiOutShortMsg(out, 0x00003C90);
	midiOutShortMsg(out, 0x00004090);
	midiOutShortMsg(out, 0x00004390);

	midiOutClose(out);
	return 0;
}


