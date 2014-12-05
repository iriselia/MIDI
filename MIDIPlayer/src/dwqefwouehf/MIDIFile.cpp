#include <stdio.h>
#include <stdlib.h>
#include "MIDIFile.h"
#include "Utility.h"

MIDIFile::MIDIFile()
{}

MIDIFile::~MIDIFile()
{
	free(m_pFileBuf);
}

int MIDIFile::initMIDIFile(const char* _fileName)
{
	m_pFileBuf = loadFile(_fileName, m_fileSize);
	if (!m_pFileBuf)
	{
		printf("something went wrong during file load.\n");
		return 1;
	}

	//1: process header
	m_pHeader = (MIDIHeaderInfo*)m_pFileBuf;
	int numTracks = byteSwapShort(m_pHeader->tracks);
	m_PulsesPerQuarterNote = byteSwapShort(m_pHeader->ticks);

	//2: process body
	unsigned char* pBody = m_pFileBuf += sizeof(MIDIHeaderInfo);
	auto pCurrentTrack = (char*) pBody;
	auto pCurrentTrackInfo = (MIDITrackInfo*)pCurrentTrack;
	for (int i = 0; i < numTracks; i++)
	{
		if (pCurrentTrackInfo->id != 0x6b72544d)//magic number: 0xkrTM  -> big endian MTrk
		{
			printf("Fatal Error: MTrk identifier not found when parsing file: %s\n", _fileName);
			return 1;
		}

		MIDITrack currentTrack;
		currentTrack.m_pTrackInfo = pCurrentTrackInfo;
		currentTrack.m_pBuffer = (unsigned char*)pCurrentTrack + sizeof(MIDITrackInfo);
		currentTrack.m_absTime = 0;
		currentTrack.m_lastEvent = 0;
		m_tracks.push_back(currentTrack);
		
		//increment pCurrentTrack to point to the next track
		pCurrentTrack += sizeof(MIDITrackInfo) + byteSwapInt(m_tracks[i].m_pTrackInfo->length);
	}
	return 0;
}