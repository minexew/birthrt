// CUSTMSYS.HPP - Customizable functions that support breaking out parts
//						of the interpreter into stand-alone libraries.

#ifndef CUSTMSYS_HPP
#define CUSTMSYS_HPP

#include <stdlib.h>

#define MaxPath	_MAX_PATH
typedef unsigned char	uchar;
typedef unsigned char	MemType;
typedef short				Int16;
typedef unsigned short	ushort;
typedef unsigned short	UInt16;
typedef unsigned short	ResNum;
typedef unsigned int		uint;
typedef unsigned long	ulong;
typedef int Boolean;
const Boolean	True = 1;
const Boolean	False = 0;
const uchar		MemResAudio = 0x0d;
#ifdef DEBUG
#include <assert.h>
#else
#define assert(expr)
#endif
template<class T, class S>
inline S Min(S	a, T b)
{
	return a < b ? a : b;
}
template<class T, class S>
inline S Max(S	a, T b)
{
	return a > b ? a : b;
}

enum audio_fatal 
{
	BAD_FORMAT=1,
	COMPRESSED_WAVE,
	SUBMIT
};

void* GetAdr(int);
void FreeAdr(int);
int Open(int);
int Close(int);
int LSeek(int,long,int);
int FileLength(int);
int Read(int,void*,int);
void Fatal(int,int);
int GetNumEntry(const char*,int);
int GetElapsedTicks();
char* GetAudioDriverName();

class MemID 
{
public:
	MemID();
	MemID(const MemID& id);
	MemID(void* h); 
	void Get(size_t size);
	void Realloc(size_t size);
	void Free();
	int Read(int fd, size_t ofs, size_t size) const;
	size_t Size() const;
	void* operator=(void* p);
	void* operator*() const;
	char& operator[](size_t s) const;
	operator int() const;
	int Load(char* fileName); // Only used in DOS, so you can ignore it for Windows

protected:
	void*	handle;
	size_t len;
};

#endif 
