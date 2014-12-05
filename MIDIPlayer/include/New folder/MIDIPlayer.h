#pragma once

#include <vector>
#include <windows.h>
#include "MIDIFile.h"


#define MAX_STREAM_BUFFER_SIZE (512)

class MIDIPlayer
{
public:
	MIDIPlayer();
	~MIDIPlayer();

	void ProcessInput();

	void AddTrack();
	void Alloc2Slots(unsigned int& _currStreamBufLen);
	void DecodeCurrentA();
	void DecodeCurrentB();
	void USleep(int waitTime);
	static void CALLBACK MIDICallback(HMIDIOUT out, UINT msg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

	void Init();
	void Run();
	void Halt();
private:
	HMIDIOUT m_HOutDevice;
	unsigned int m_DeviceID;
	HMIDISTRM m_HMIDIOutStream;

	int m_currFileIndex;

	MIDIEVENT* m_pStreamBuf;
	unsigned int m_PulsesPerQuarterNote;
	int m_streamBufSize;


	bool m_isRequestingExit;
	bool m_isPlaying;
	std::vector<MIDIFile*> m_fileList;
};