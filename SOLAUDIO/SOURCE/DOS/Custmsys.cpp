//------------------------------------------------------------------------------------------
//	@doc AUDLIB	
//
// @module CUSTMSYS.CPP -- Customizable audio support function |
//
//	Date:			 04/11/96
// Created by:  Jay Lee
//
// This file must be included in the build to use the Sierra Audio Library.  You may
// leave it untouched, but this will give you a very unsophisticated implementation of
// resource acquisition and memory management.  Based on your needs, feel free to change
// the implementation of any of the functions, just as long as the function prototypes
// stay as they are.
//------------------------------------------------------------------------------------------

//#include <windows.h>  
//#include <mmsystem.h>	 //if you want to use the code in GetElapsedTicks, bring these back in

#include <stdio.h>
#include <dos.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <string.h>

#include "custmsys.hpp"
#include "audio.hpp"

//-------------------------------------------------------------------------------
// Input: sample number
// Return: pointer to memory-resident audio sample; or
//         0 if specified sample is not memory-resident
// Note: Code to load the specified sample can be placed in this routine.
//-------------------------------------------------------------------------------
void* GetAdr(int num)
{
	num=num;
	return 0;
}

//-------------------------------------------------------------------------------
// Input: sample number
// The specified sample is complete and its memory footprint can be freed.
// Note: This function will be called (and only called) if GetAdr() for the
//       same sample number returned nonzero.
//-------------------------------------------------------------------------------
void FreeAdr(int num)
{
	num=num;
}

//-------------------------------------------------------------------------------
// Input: sample number
// Return: file handle to open file whose current file pointer is positioned
//         to the start of the audio sample; or
//         -1 if the specified sample is not found
// Note: Code to open and position the file can be placed in this routine.
//       This function will be called (and only called) if GetAdr() for the
//       same sample number returned zero.
//-------------------------------------------------------------------------------
int Open(int num)
{
	char	pathName[MaxPath + 1];

	sprintf(pathName,"%d.WAV",num);
	return open(pathName,O_BINARY|O_RDONLY);
}

//-------------------------------------------------------------------------------
// Input: file handle
// Note: This function will be called (and only called) if Open() did not
//       return -1.
//-------------------------------------------------------------------------------
int Close(int fd)
{
	return close(fd);
}

//-------------------------------------------------------------------------------
// Input: file handle, file offset, file origin
// Return: file position
//-------------------------------------------------------------------------------
int LSeek(int fd, long offset, int fromWhere)
{
	return lseek(fd, offset, fromWhere);
}

//-------------------------------------------------------------------------------
// Input: file handle
// Return: length of file
//-------------------------------------------------------------------------------
int FileLength(int fd)
{
	return filelength(fd);
}

//-------------------------------------------------------------------------------
// Input: file handle, destination pointer, #bytes to read
// Return: the number of bytes actually read
//-------------------------------------------------------------------------------
int Read(int fd, void* buf, int n)
{
	return read(fd, buf, n);
}

//-------------------------------------------------------------------------------
// Input: error number
// Return: Program must abort
// Note: Currently, three fatal conditions exist:
//             Unsupported wave format
//             Unsupported compression format
//             Windows submission error
//-------------------------------------------------------------------------------
void
Fatal(int code,int num)
{
	switch (code) {
		case BAD_FORMAT:
//			printf("Unrecognized audio format for sample#%u\n",num);
			break;
		case COMPRESSED_WAVE:
//			printf("Can't play compressed waves; sample#%u\n",num);
			break;
		case SUBMIT:
//			printf("Error submitting audio\n");
			break;
		default:
			num=num;
//			printf("Unknown audio error\n");
	}
	exit(1);
}

//-------------------------------------------------------------------------------
// Input: A literal string; a default value
// Output: a numeric value associated with the input string
// Note: Currently the stand-alone audio version returns the default value.
// These literals/default values are currently in use:
// audioSize/65536 dacSize/4096 minSubmits/4 maxSubmits/24 leadSubmits/2
//-------------------------------------------------------------------------------
int GetNumEntry(const char *str, int defVal)
{
	if (str);
	return defVal;
}

//-------------------------------------------------------------------------------
// Input: none
// Output: ticks (60th secs) since the application commenced
// Note: This function needs to be changed onlyu if KernelAudioDuration() will be 
//       used to determine elapsed playback time
//-------------------------------------------------------------------------------
int GetElapsedTicks()
{
// Return the application's elapsed time in 60th seconds
	return 1;

	
// Note: I used the code below for Windows	

//	static int iStartTicks = 0;

//	if (iStartTicks == 0)
//	{
//		iStartTicks = GetTickCount();
//	}

//	return( ((GetTickCount() - iStartTicks)*3) / 50 );
}
//-------------------------------------------------------------------------------
// Input: none
// Output: Name of the DOS audio driver to use.  See driver list with package
//         for valid names
// Note: This function is DOS ONLY - it is irrelevant to WINDOWS
//-------------------------------------------------------------------------------
char* GetAudioDriverName()
{
	return "DACBLAST.DRV";
}

//-------------------------------------------------------------------------------
// Get method defined which currently uses malloc to allocate memory
// Realloc method defined which currently uses realloc to reallocate memory
// Free method defined which currently uses free to release memory
// Read method defined which uses Read() defined above.
//-------------------------------------------------------------------------------
MemID::MemID() : handle(0)
{
}

MemID::MemID(const MemID& id) : handle(id.handle)
{
}

MemID::MemID(void* h) : handle(h)
{
}

void MemID::Get(size_t size)
{
	handle = malloc(len = size);
}

void MemID::Realloc(size_t size)
{
	handle = realloc(handle, len = size);
}

void MemID::Free()
{
	if (handle) 
	{
		free(handle);
		handle = 0;
		len = 0;
	}
}

int MemID::Read(int fd, size_t ofs, size_t size) const
{
	return ::Read(fd, &(*this)[ofs], size);
}

size_t MemID::Size() const
{
	return handle ? len : 0;
}

void* MemID::operator=(void* p)
{
	return handle = p;
}

void* MemID::operator*() const
{
	return handle;
}

char& MemID::operator[](size_t s) const
{
	return *((char*)handle + s);
}

long MemID::IsAllocated() const
{
	if (handle)
		return 1;
	else
		return 0;
}
#if 0
MemID::operator int() const
{
	if (handle)
		return 1;
	else
		return 0;
}
#endif

// Load is only used in DOS, so you can ignore it for Windows
int MemID::Load(char* fileName)
{
	int fd;
	if ((fd = open(fileName, O_RDONLY | O_BINARY)) == -1)
		return 0;
	int size = FileLength(fd);
	Get(size);
	int rc = Read(fd,0,size);
	if (rc != size) {
		Free();
		rc = 0;
	}
	Close(fd);
	return rc;
}


