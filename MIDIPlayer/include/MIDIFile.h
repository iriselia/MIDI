#pragma once
#include <vector>

#pragma pack(push, 1)

struct MIDIHeaderInfo
{
	unsigned int	id;		// identifier "MThd"
	unsigned int	size;	// always 6 in big-endian format
	unsigned short	format;	// big-endian format
	unsigned short  tracks;	// number of tracks, big-endian
	unsigned short	ticks;	// number of ticks per quarter note, big-endian
};

struct MIDITrackInfo
{
	unsigned int	id;		// identifier "MTrk"
	unsigned int	length;	// track length, big-endian
};

#pragma pack(pop)

struct MIDITrack
{
	MIDITrackInfo* m_pTrackInfo;
	unsigned char* m_pBuffer;
	unsigned char m_lastEvent;
	unsigned int m_absTime;
};

struct MIDIEvent
{
	unsigned int m_absTime;
	unsigned char* m_pData;
	unsigned char m_event;
};

class MIDIFile
{
public:
	MIDIFile();

	~MIDIFile();

	int initMIDIFile(const char* _fileName);

public:
	int m_fileSize;
	unsigned char* m_pFileBuf;
	int m_PulsesPerQuarterNote;
	
	MIDIHeaderInfo* m_pHeader;
	MIDITrackInfo* m_pBody;
	
	std::vector<MIDITrack> m_tracks;
};