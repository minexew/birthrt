// ===========================================================================
//		RESUTIL.H
// ===========================================================================

#if !defined (_RESUTIL_)
#define _RESUTIL_


// #defines

// #define BUILD_LONG(s1, s2)  ((0x00000000 | (s1)<<16) | (s2))
// #define ARRAYSIZE 0x00ffffff
#define CMDLINE_LENGTH 255
#define NO_COMPRESSION		0				// uncompressed data
#define RLE_COMPRESSION		1				// RLE is not supported in this version
#define LZSS_COMPRESSION	2

// Versions
// 	Version 1 simply concatenated resources into one big file.  You skipped
// through the file from resource to resource to find out where everything was.
// 	Version 2 added a file header at the beginning of the file and a
// resource directory at the end of the file.
// 	Version 3 added the flags field and converted PCX files to BITMAP
// files before storing them.
// 	Version 4 added the hc/hi flags for CRC hashing (default) and ID
// hashing.

#define RESUTIL_VERSION		0x00000400

#define szSTART_CODE			"RSRC"		// code that should be at start of
													// resource (for validity check)
#define cMAX_EXTENSIONS		4
#define cMAX_RESNAME			13

#define PCX_TYPE				1
#define FLC_TYPE				2
#define WAV_TYPE				3

// Flags.  RESUTIL sets the RFF_PCX_UNCOMP and RFF_ROTATED flags.
// (RFF stands for Resource File Flag.)
#define RFF_PCX_UNCOMP		1				// TRUE if PCX uncompressed
													// (= converted to BITM format)
#define RFF_ROTATED			2           // TRUE if rotated
// The following flags are not relevant to resources.  RESMANAG should
// keep track of these states at runtime.
// #define RFF_SCALED			4           // TRUE if scaled
// #define RFF_LOCKED			8           // TRUE if locked
#define RFF_HASH_ID			16				// TRUE if hash is ID, FALSE if
													// hash is CRC (the default)
													// possibly useful for debugging,
													// error checking

// Resource file utility operation codes.
typedef enum {
	RFU_ADD,
	RFU_REMOVE,
	RFU_EXTRACT,
	RFU_LIST,
	RFU_RESPONSEFILE,
	RFU_VERIFY
} RFU_OP;


// Structures
// Header for an individual resource.
typedef struct sResourceHeader {
	ULONG		startcode;						// RSRC string for validity check
	ULONG		cbChunk;							// total size of this chunk
	ULONG		cbCompressedData;				// size of compressed data
	ULONG		cbUncompressedData;			// size of uncompressed data
	ULONG		hashValue;						// hash value of file name
	UBYTE		flags;							// [d4-03-97 JPC] new field
	UBYTE		compressionCode;				// 0 = none, 1 = RLE, 2 = LZSS
													// (note: RLE is not currently supported)
	UBYTE		fileExtension;					// index to file extension type
	char		szName[cMAX_RESNAME];		// 8.3 filename (no path)
} RESOURCE_HEADER;

// Header for a resource file.
typedef struct strResfileHeader {
   ULONG    versionResFile;            // resource file version
   ULONG    oDirectory;                // offset to directory from beginning of file
   ULONG    cResources;                // total resources in file
} RESFILE_HEADER;

// Directory consists of an array of DIRENTRY structures.
typedef struct strDirEntry {
	ULONG		hashValue;						// hash value of file name
	ULONG		resOffset;						// offset to resource from beginning of file
	UBYTE		fileExtension;					// index to file extension type
	char		szName[cMAX_RESNAME];		// 8.3 filename (no path)
} DIRENTRY, *DIRENTRY_PTR;

// prototypes

void AddExtension (char * szFilename, char * szExtension);
void ChangeExtension (char * szFilename, char * szExtension);
void GetExtension (char * szFilename, char * szExtension, int maxChars);
UBYTE GetExtensionCode (char * szFilename);
int RenameFileExtension (char * szFilename, char * szExtension);
ULONG HashCRC(register CSTRPTR szIn);
ULONG HashID(CSTRPTR szFileName);
ULONG GetNextLine(FILE *fp, char *cpNewLine, LONG len);
BOOL Exists( char *);

#endif

