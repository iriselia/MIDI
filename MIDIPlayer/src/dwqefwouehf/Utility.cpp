#include "Utility.h"
#include <iostream>

unsigned char* loadFile(const char* _fileName, int& _fileSize)
{
	unsigned int hr;
	unsigned char* fileBuf;
	int fileSize;
	FILE* fd = fopen((char*)_fileName, "rb");
	if (fd == NULL)
		return nullptr;

	fseek(fd, 0, SEEK_END);
	fileSize = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	fileBuf = (unsigned char*)malloc(fileSize);
	if (fileBuf == 0)
	{
		fclose(fd);
		return nullptr;
	}

	hr = fread(fileBuf, 1, fileSize, fd);
	fclose(fd);

	if (hr != fileSize)
	{
		printf("fread returned wrong file size.\n");
		free(fileBuf);
		return nullptr;
	}

	_fileSize = fileSize;
	return fileBuf;
}

