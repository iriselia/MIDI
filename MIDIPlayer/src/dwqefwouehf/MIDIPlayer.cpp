#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <windows.h>
#include "MIDIPlayer.h"
#include "MIDIFile.h"
#include "Utility.h"

HANDLE g_event;

MIDIPlayer::MIDIPlayer()
{
	m_isRequestingExit = false;
	m_isPlaying = false;
	m_PulsesPerQuarterNote = 0;
	m_streamBufSize = 0;
	m_currFileIndex = 0;
	m_HMIDIOutStream = 0;
	m_DeviceID = 0;
}

MIDIPlayer::~MIDIPlayer()
{

}

void MIDIPlayer::Init()
{
	m_pStreamBuf = (MIDIEVENT*)malloc(sizeof(MIDIEVENT)* MAX_STREAM_BUFFER_SIZE);
	if (!m_pStreamBuf)
	{
		printf("Fatal Error: Unable to initialize buffer for MIDIPlayer.\n");
	}
	memset(m_pStreamBuf, NULL, sizeof(MIDIEVENT)* MAX_STREAM_BUFFER_SIZE);

	int hr = 0;

	//hr |= midiOutOpen(&m_HOutDevice, 0, 0, 0, CALLBACK_NULL);
	if (hr != MMSYSERR_NOERROR)
	{
		printf("error opening default MIDI stream: %d\n", hr);

	}
	else
	{
		printf("successfully opened default MIDI stream\n");
	}
}

void MIDIPlayer::Run()
{
	while (!m_isRequestingExit)
	{
		ProcessInput();
		if (m_isPlaying)
		{
			DecodeCurrentB();

			int i = 0;


			int err;
			HMIDIOUT out;
			unsigned int msg;

			err = midiOutOpen(&out, 0, 0, 0, CALLBACK_NULL);
			if (err != MMSYSERR_NOERROR)
				printf("error opening default MIDI device: %d\n", err);
			else
				printf("successfully opened default MIDI device\n");

			i = 0;
			while (i < m_streamBufSize)
			{
				unsigned int time = m_pStreamBuf[i].dwDeltaTime;

				USleep(time * m_PulsesPerQuarterNote);

				//get midi command
				MIDIEVENT event = *(MIDIEVENT*)&m_pStreamBuf[i];
				msg = event.dwEvent;
				if (msg & 0xff000000) // tempo change
				{
					msg = msg & 0x00ffffff;
					m_PulsesPerQuarterNote = msg / byteSwapShort((m_fileList[m_currFileIndex])->m_pHeader->ticks);
				}
				else
				{
					err = midiOutShortMsg(out, msg);
					if (err != MMSYSERR_NOERROR)
						printf("error sending command: %08x error: %d\n", msg, err);
				}
				// Onto next message
				i++;
			}





			while (i < m_streamBufSize)
			{
				//ProcessInput();
				// get delta time from message
				unsigned int dt = m_pStreamBuf[i].dwDeltaTime;

				//wait before submitting the message
				USleep(dt * m_PulsesPerQuarterNote);

				//get midi command
				MIDIEVENT event = *(MIDIEVENT*)&m_pStreamBuf[i];
				unsigned int msg = event.dwEvent;
				if (msg & 0xff000000) // tempo change
				{
					msg = msg & 0x00ffffff; // microseconds/quarter note
					m_PulsesPerQuarterNote = msg / byteSwapShort((m_fileList[m_currFileIndex])->m_pHeader->ticks);
				}
				else
				{
					int hr = midiOutShortMsg(m_HOutDevice, msg);
					if (hr != MMSYSERR_NOERROR)
					{
						printf("error sending command: %d\n", hr);
					}
				}

				// Onto next message
				i++;
			}


			//finished playing
			m_isPlaying = false;
		}

	}
}

void MIDIPlayer::Halt()
{
	midiOutClose(m_HOutDevice);
	free(m_pStreamBuf);
}

void MIDIPlayer::AddTrack()
{
	int hr;
	std::string trackName;
	//getline(std::cin, trackName);
	trackName = "example8.mid";
	auto newFile = new MIDIFile();
	hr = newFile->initMIDIFile(trackName.c_str());
	if (hr)
	{
		//error checking
		printf("File not found.\n");
		return;
	}
	m_fileList.push_back(newFile);
	printf("Track added: %s\n", trackName.c_str());
}

void MIDIPlayer::DecodeCurrentA()
{
	//reset stream buf size
	m_streamBufSize = 0;
	m_PulsesPerQuarterNote = m_fileList[m_currFileIndex]->m_PulsesPerQuarterNote;
	unsigned char* pFileBuf = m_fileList[m_currFileIndex]->m_pFileBuf;
	pFileBuf += sizeof(MIDITrackInfo);


	while (pFileBuf < m_fileList[m_currFileIndex]->m_pFileBuf + m_fileList[m_currFileIndex]->m_fileSize)
	{
		unsigned int msg = 0;

		// |       X - 0      |
		// --------------------------------------------------
		// |  Variable Sized  |
		// Read the delta time variable first
		unsigned int bytesread;
		unsigned int dt = read_var_int(pFileBuf, &bytesread);
		pFileBuf += bytesread;

		// | 31 - 24 | 23 - 16 | 15 - 8 |    7 - 0    |
		// --------------------------------------------------
		// | Unused  | Data 2  | Data 1 | Status byte |
		// Read midi command into the message
		unsigned char cmd = *pFileBuf;
		msg |= cmd;										// 0x000000FF

		// Onto Data1 byte
		pFileBuf++;
		unsigned char data1 = *pFileBuf;

		// if normal command
		if ((cmd & 0xf0) != 0xf0)
		{
			// all normal commands have data1
			msg |= ((unsigned int)(data1 << 8));		// 0x0000FF00

			//Onto Data2 byte
			pFileBuf++;
			unsigned char data2 = *pFileBuf;

			// 0xc0 (Patch change) and 0xd0 (Channel pressure) only take a single byte
			if (!((cmd & 0xf0) == 0xc0 || (cmd & 0xf0) == 0xd0))
			{
				// Does have data2
				msg |= ((unsigned int)(data2 << 16));	// 0x00FF0000

				//Onto the next message
				pFileBuf++;
			}

			//m_pStreamBuf[m_streamBufSize++] = dt;
			//m_pStreamBuf[m_streamBufSize++] = msg;
		}
		// else if meta-event
		else if (cmd == 0xff)
		{
			cmd = *pFileBuf++; // cmd should be meta-event (0x2f for end of track)
			cmd = *pFileBuf++; // cmd should be meta-event length
			pFileBuf += cmd;
		}
	}
}

void MIDIPlayer::Alloc2Slots(unsigned int& _currStreamBufLen)
{
	while (m_streamBufSize + 1 > _currStreamBufLen)
	{
		unsigned int* tmp = NULL;
		_currStreamBufLen *= 2;
		tmp = (unsigned int*)realloc(m_pStreamBuf, sizeof(MIDIEVENT) * _currStreamBufLen);
		if (tmp != NULL)
		{
			//m_pStreamBuf = tmp;
		}
		else
		{
			if (m_pStreamBuf)
			{
				free(m_pStreamBuf);
			}
			return;
		}
	}
}

void MIDIPlayer::DecodeCurrentB()
{
	int hr = 0;
	//Set up midi stream property based on new file
	MIDIPROPTIMEDIV MIDIStreamProperty;
	MIDIStreamProperty.cbStruct = sizeof(MIDIPROPTIMEDIV);
	MIDIStreamProperty.dwTimeDiv = byteSwapShort(m_fileList[m_currFileIndex]->m_pHeader->ticks);
	//hr = midiStreamProperty(m_HMIDIOutStream, (LPBYTE)&MIDIStreamProperty, MIDIPROP_SET | MIDIPROP_TIMEDIV);
	if (hr != MMSYSERR_NOERROR)
	{
		printf("error on setting MIDIStreamProperty: %d\n", hr);

	}
	else
	{
		printf("successfully set MIDIStreamProperty\n");
	}

	//allocate stream buffer
	MIDIHDR MIDIHeader;
	MIDIHeader.lpData = (char*)m_pStreamBuf;
	MIDIHeader.dwBufferLength = MIDIHeader.dwBytesRecorded = MAX_STREAM_BUFFER_SIZE;
	MIDIHeader.dwFlags = 0;
	//hr = midiOutPrepareHeader((HMIDIOUT)m_HMIDIOutStream, &MIDIHeader, sizeof(MIDIHDR));
	if (hr != MMSYSERR_NOERROR)
	{
		printf("error on setting MIDIHeader: %d\n", hr);

	}
	else
	{
		printf("successfully set MIDIHeader\n");
	}

	// Initialize default values for streaming
	m_PulsesPerQuarterNote = 500000 / m_fileList[m_currFileIndex]->m_PulsesPerQuarterNote;
	unsigned int currTime = 0;
	unsigned int currStreamSize = MAX_STREAM_BUFFER_SIZE;
	// windows format midi event
	MIDIEVENT newEvent;

	static int vivi = 0;

#define TEMPO_EVT 1
	while (TRUE)
	{	
		unsigned int dt = (unsigned int)-1;
		unsigned int nearestIndex = -1;
		MIDIEvent MIDIEvent;
		std::vector<MIDITrack>& tracks = m_fileList[m_currFileIndex]->m_tracks;

		// get the nearest next event from all tracks
		for (int i = 0; i < tracks.size(); i++)
		{
			auto tmpEvent = getNextEvent(&tracks[i]);

			if (!(isTrackEnd(&tmpEvent)) && (tmpEvent.m_absTime < dt))
			{
				MIDIEvent = tmpEvent;
				dt = tmpEvent.m_absTime;
				nearestIndex = i;
			}
		}

		// if nearestIndex == -1 then all the tracks have been read up to the end of track mark
		if (nearestIndex == -1)
		{
			break;
		}

		vivi++;
		printf("itr:%d\n", vivi);
		if (tracks.size() > 1)
		{
			printf("wtf\n");
		}

		newEvent.dwStreamID = 0; // always 0
		newEvent.dwParms[0] = 0;

		// Update track abs time to match the latest processed event
		tracks[nearestIndex].m_absTime = MIDIEvent.m_absTime;
		// update global delta time and current time
		newEvent.dwDeltaTime = tracks[nearestIndex].m_absTime - currTime;
		currTime = tracks[nearestIndex].m_absTime;

		// running mode
		if (!(MIDIEvent.m_event & 0x80)) // not above 1000 0000, so not 0x80 or 0x90
		{
			//get last command
			unsigned char lastEvent = tracks[nearestIndex].m_lastEvent;
			newEvent.dwEvent = ((unsigned long)lastEvent);

			// get Data1 byte
			unsigned char data1 = *(MIDIEvent.m_pData);

			// all normal commands have data1
			newEvent.dwEvent |= ((unsigned int)(data1 << 8));		// 0x0000FF00

			//Onto Data2 byte
			(MIDIEvent.m_pData)++;
			unsigned char data2 = *(MIDIEvent.m_pData);

			// 0xc0 (Patch change) and 0xd0 (Channel pressure) only take a single byte
			if (!((lastEvent & 0xf0) == 0xc0 || (lastEvent & 0xf0) == 0xd0))
			{
				// Does have data2
				newEvent.dwEvent |= ((unsigned int)(data2 << 16));	// 0x00FF0000

				//Onto the next message
				(MIDIEvent.m_pData)++;
			}

			// cast stream buffer pointer to windows MIDIEVENT pointer
			Alloc2Slots(currStreamSize);
			*(MIDIEVENT*)(&m_pStreamBuf[m_streamBufSize]) = newEvent;
			m_streamBufSize++;

		}
		else if (MIDIEvent.m_event == 0xff) // meta-event
		{
			MIDIEvent.m_pData++; // skip the event byte
			unsigned char meta = *MIDIEvent.m_pData++; // read the meta-event byte
			unsigned int len;

			switch (meta)
			{
			case 0x51: // set tempo
			{
						 unsigned char a, b, c;
						 len = *MIDIEvent.m_pData++; // get the length byte, should be 3
						 a = *MIDIEvent.m_pData++;
						 b = *MIDIEvent.m_pData++;
						 c = *MIDIEvent.m_pData++;

						 newEvent.dwEvent = ((unsigned long)TEMPO_EVT << 24) |
							 ((unsigned long)a << 16) |
							 ((unsigned long)b << 8) |
							 ((unsigned long)c << 0);

						 // cast stream buffer pointer to windows MIDIEVENT pointer
						 Alloc2Slots(currStreamSize);
						 *(MIDIEVENT*)(&m_pStreamBuf[m_streamBufSize]) = newEvent;
						 m_streamBufSize++;
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
			case 0x7f: // sequencer specific information
			default:
				len = *MIDIEvent.m_pData++; // ignore event, skip all data
				MIDIEvent.m_pData += len;
				break;
			}
		}
		// if normal command
		else if ((MIDIEvent.m_event & 0xf0) != 0xf0)
		{
			tracks[nearestIndex].m_lastEvent = MIDIEvent.m_event;

			// | 31 - 24 | 23 - 16 | 15 - 8 |    7 - 0    |
			// --------------------------------------------------
			// | Unused  | Data 2  | Data 1 | Status byte |
			// Read midi command into the message
			unsigned char cmd = *(MIDIEvent.m_pData);
			newEvent.dwEvent = cmd;										// 0x000000FF

			// Onto Data1 byte
			(MIDIEvent.m_pData)++;
			unsigned char data1 = *(MIDIEvent.m_pData);


			// all normal commands have data1
			newEvent.dwEvent |= ((unsigned int)(data1 << 8));		// 0x0000FF00

			//Onto Data2 byte
			(MIDIEvent.m_pData)++;
			unsigned char data2 = *(MIDIEvent.m_pData);

			// 0xc0 (Patch change) and 0xd0 (Channel pressure) only take a single byte
			if (!((cmd & 0xf0) == 0xc0 || (cmd & 0xf0) == 0xd0))
			{
				// Does have data2
				newEvent.dwEvent |= ((unsigned int)(data2 << 16));	// 0x00FF0000

				//Onto the next message
				(MIDIEvent.m_pData)++;
			}

			// cast stream buffer pointer to windows MIDIEVENT pointer
			Alloc2Slots(currStreamSize);
			*(MIDIEVENT*)(&m_pStreamBuf[m_streamBufSize]) = newEvent;
			m_streamBufSize ++;

		}
		else
		{
			// not handling sysex events yet
			printf("unknown event %2x", MIDIEvent.m_event);
			exit(1);
		}

		//increment pointer
		tracks[nearestIndex].m_pBuffer = MIDIEvent.m_pData;

	}
}


void MIDIPlayer::ProcessInput()
{
	int key;

	if (kbhit())
	{   // If a key on the computer keyboard has been pressed
		key = _getch();

		if (m_isPlaying)
		{
			switch (key)
			{
			case 'a':
				//blocking
				printf("Adding new track...\n");
				printf("File name: ");
				AddTrack();
				break;
			case ' ':
				m_isPlaying = true;
				break;
			case 'q':
				//non-blocking
				printf("walawala\n");
				m_isRequestingExit = false;
				break;
			default:
				break;
			}
		}
		else
		{
			switch (key)
			{
			case 'a':
				//blocking
				printf("Adding new track...\n");
				printf("File name: ");
				AddTrack();
				break;
			case ' ':
				m_isPlaying = true;
				break;
			case 'q':
				//non-blocking
				printf("walawala\n");
				m_isRequestingExit = false;
				break;
			default:
				break;
			}
		}
	}


}

void MIDIPlayer::USleep(int waitTime)
{
	LARGE_INTEGER time1, time2, freq;

	if (waitTime == 0)
		return;

	QueryPerformanceCounter(&time1);
	QueryPerformanceFrequency(&freq);

	do
	{
		QueryPerformanceCounter(&time2);
	} while ((time2.QuadPart - time1.QuadPart) * 1000000ll / freq.QuadPart < waitTime);
}

void CALLBACK MIDIPlayer::MIDICallback(HMIDIOUT out, UINT msg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	switch (msg)
	{
	case MOM_DONE:
		SetEvent(g_event);
		break;
	case MOM_POSITIONCB:
	case MOM_OPEN:
	case MOM_CLOSE:
		break;
	}
}
