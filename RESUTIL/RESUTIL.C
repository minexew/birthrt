// ===========================================================================
//
//		RESUTIL.C
//
//		Resource file construction utility.
//
//		Author: John Conley (from a program by Chris Barker)
//
//		Copyright 1997 Sierra On-Line, Inc.
//
//	Notes:
//
// This is a utility for Birthright I.
//
//	A resource file is just a bunch of compressed files concatenated.
// There is a directory at the end of the file that contains some
// information about the resources.
//
// Resource files are preprocessed in RESMANAG.C: OpenResFile_.
// ===========================================================================


#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>
#include <mem.h>
// #include <assert.h>
#include <dos.h>

#include "system.h"
#include "resutil.h"
#include "lzss.h"

// Defines that are not needed by any other file.
#define cRES_SLOTS_IN_BLOCK	64
#define MODE_MAKEFILE	O_CREAT|O_RDWR|O_BINARY,S_IREAD|S_IWRITE
#define MODE_READFILE	O_RDONLY|O_BINARY
#define MODE_NEWFILE		O_CREAT|O_WRONLY|O_BINARY|O_TRUNC,S_IWRITE
#define TOOBIG_FILE		"TOOBIG.BAT"

#define HASH_CRC			0
#define HASH_ID			1

// Global data.
// [d4-11-97 JPC] Added WAV type, now that it is officially in RESMANAG.C
// (in the global ResExtentions array).
char * gszExtensions[cMAX_EXTENSIONS] = {
	"",
	"PCX",
	"FLC",
	"WAV"
};


LONG	gMaxLen = 0x7FFFFFFF;				// max length permitted for resource
													// (used to exclude excessively large
													// WAV files)

RESFILE_HEADER	grfHeader;					// resource file header
LONG	gcResSlots;								// total resource slots malloced for gpaDirentry
DIRENTRY_PTR gpaDirentry;					// pointer to an array of DIRENTRY structures

BOOL gfCompress = TRUE;						// compression is ON by default
BOOL gfRotate = FALSE;						// if TRUE, rotate PCX resources (OFF by default)
BOOL gfInited = FALSE;						// for writing caption for TOOBIG file
BOOL giHashType = HASH_CRC;				// [d4-24-97 JPC] allow HASH_CRC or HASH_ID

// For reporting what's in the resource file.
char * gszCompressionType[3] = {
	"not  compressed",
	"RLE  compressed",
	"LZSS compressed"
};


// PCX header structure.
typedef struct pcxhdr
{
	ULONG		code;
	SHORT		xo;
	SHORT		yo;
   SHORT    w;                         // raster width in pixels
   SHORT    h;                         // raster height in pixels
	UBYTE		pad1[66L-12L];
	USHORT	cbLine;
} PCXHDR;

// Prototypes for static functions.
static BOOL SaveFileNameTooBig (CSTRPTR szFilename);
static PTR DecompressPCX (PTR pInput, RESOURCE_HEADER * prh);

// ---------------------------------------------------------------------------
//    Function    -	AddOneFile
//		Description	-	Process the "+" command: add a data file to the
//							resource file.
// ---------------------------------------------------------------------------
void AddOneFile (int resfile, char * szPath, char * szFilename)
{
	int					infile;
	ULONG					file_len;
	ULONG					cbSafety;
	PTR					pInput, pOutput, pEnd;
	char * 				szNameNoPath;
	RESOURCE_HEADER	rh;
	char					szFilespec[_MAX_PATH];

	sprintf (szFilespec, "%s%s", szPath, szFilename);
	pInput  = NULL;
	pOutput = NULL;

	rh.flags = 0;

	if ((infile = open (szFilespec, MODE_READFILE)) == fERROR)
	{
		printf("Can't open add file %s\n", szFilespec);
		return;									// premature return
	}


   // Get the simple file name, no path.  Store it in the resource file
	// so we can extract the file by name later.
	szNameNoPath = strrchr (szFilespec, '\\');
	if (szNameNoPath == NULL)
	{
		szNameNoPath = szFilespec;
	}
	else
	{
      szNameNoPath++;                  // next character after path char (\)
	}

   rh.cbUncompressedData = lseek (infile, 0L, SEEK_END);

	if (rh.cbUncompressedData > gMaxLen)
	{
		printf("File size (%d) is greater than max (%d): not adding to file\n",
			rh.cbUncompressedData, gMaxLen);
		SaveFileNameTooBig (szFilespec);
		goto Exit;
	}


   // Allocate space for input.
   if ((pInput = malloc (rh.cbUncompressedData)) == NULL)
	{
		printf("Unable to malloc input buffer\n");
		goto Exit;
	}

	strncpy (rh.szName, szNameNoPath, cMAX_RESNAME);
	rh.fileExtension = GetExtensionCode (szFilename);
	if (giHashType == HASH_CRC)
	{
		rh.hashValue = HashCRC (szNameNoPath);
	}
	else if (giHashType == HASH_ID)
	{
		rh.hashValue = HashID (szNameNoPath);
		rh.flags |= RFF_HASH_ID;
	}
	else
	{
		printf ("Invalid hash type (%d) for resource %s\n", giHashType, szFilename);
		goto Exit;
	}

   // Read in the data.
	printf ("Reading %ld bytes of %s\n", rh.cbUncompressedData, szFilespec);

	lseek (infile, 0L, SEEK_SET);
	file_len = read (infile, pInput, (int)rh.cbUncompressedData);

	if (file_len == -1)
	{
		printf("Error reading file %s.\n", szFilespec);
		goto Exit;
	}

	if (rh.fileExtension == PCX_TYPE)
	{
		PTR newPtr = DecompressPCX (pInput, &rh);
		free (pInput);
		if (newPtr != NULL)
		{
			pInput = newPtr;
		}
		else
		{
			printf ("Error: could not decompress PCX file\n");
			goto Exit;
		}
		rh.flags |= RFF_PCX_UNCOMP;
		if (gfRotate)
			rh.flags |= RFF_ROTATED;

      // NOTE: At this point pInput should point to a BITMAP object that we
      // can load and use as a lump.  (We still do LZSS compression on it.)
   }
	
	strncpy ((char *)&rh.startcode, szSTART_CODE, 4);


   // Allocate space for output.
   cbSafety = rh.cbUncompressedData/4; // 25% safety factor in case LZSS
													 // compression makes it BIGGER!

	if ((pOutput = malloc(rh.cbUncompressedData + cbSafety)) == NULL)
	{
		printf("Unable to malloc output buffer\n");
		goto Exit;
	}

	memset (pOutput, 0, rh.cbUncompressedData);
	// Do the compression.-----------------------------------------------------
	if (gfCompress)
	{
		pEnd = _LZSSEncode_ (pInput, pOutput, rh.cbUncompressedData);

		printf ("Start is 0x%x end is 0x%x\n", pOutput, pEnd);
		rh.cbCompressedData = (unsigned long) pEnd - (unsigned long) pOutput + 2;

		printf ("AddOneFile: rh.cbCompressedData = %d, rh.cbUncompressedData = %d\n",
			rh.cbCompressedData, rh.cbUncompressedData);
	}
	else
	{
		rh.cbCompressedData = rh.cbUncompressedData;
	}

	if (rh.cbCompressedData >= rh.cbUncompressedData)
	{
		// If LZSS compression accomplished nothing or actually made the file
		// bigger, just save it uncompressed.
		rh.compressionCode = NO_COMPRESSION;
		rh.cbCompressedData = rh.cbUncompressedData;
		rh.cbChunk = sizeof (rh) + rh.cbUncompressedData;
		printf ("  Writing %ld UNCOMPRESSED bytes to resource file.\n", rh.cbUncompressedData);
		// We are going to add this resource to the end of the file.
		// That will overwrite the directory.
		lseek (resfile, grfHeader.oDirectory, SEEK_SET);
		write (resfile, &rh, sizeof (rh));
		write (resfile, pInput, (int)rh.cbUncompressedData);
	}
	else
	{
		rh.compressionCode = LZSS_COMPRESSION;
		rh.cbChunk = sizeof (rh) + rh.cbCompressedData;
		printf ("  Writing %ld COMPRESSED bytes to resource file.\n", rh.cbCompressedData);
		// We are going to add this resource to the end of the file.
		// That will overwrite the directory.
		lseek (resfile, grfHeader.oDirectory, SEEK_SET);
		write (resfile, &rh, sizeof (rh));
		write (resfile, pOutput, (int)rh.cbCompressedData);
	}
	// Update the directory and the resfile header.
	AddDirectoryEntry (&rh);

Exit:
	if (pOutput != NULL)
		free (pOutput);
	if (pInput != NULL)
		free (pInput);
	close (infile);
}


// ---------------------------------------------------------------------------
//    Function    -	AddFile
//		Description	-	Process the "+" command: add a data file to the
//							resource file.  Allows wildcards.
// ---------------------------------------------------------------------------
void AddFile (int resfile, char * szFilespec)
{
	char				szPath[_MAX_PATH];
	char				szDrive[_MAX_DRIVE];
	char				szDir[_MAX_DIR];
	char				szFname[_MAX_FNAME];
	char				szExt[_MAX_EXT];
	struct find_t	fileinfo;
	int				rc;

	_splitpath (szFilespec, szDrive, szDir, szFname, szExt);

	sprintf (szPath, "%s%s", szDrive, szDir);

	rc = _dos_findfirst (szFilespec, _A_NORMAL, &fileinfo);

	while (rc == 0)
	{
		AddOneFile (resfile, szPath, fileinfo.name);
		rc = _dos_findnext (&fileinfo);
	}
	_dos_findclose (&fileinfo);
	SaveResfileHeader (resfile);
	SaveDirectory (resfile);
}


// ---------------------------------------------------------------------------
//    Function    -	RemoveResource
//		Description	-	Copy resources from oldfile to newfile,
//							skipping specified resource.  Uses hash value
//							to find match.
//		Returns		-	TRUE if copy worked, FALSE if not (malloc failure, invalid resource).
//							Note that this routine returns TRUE even if the
//							specified resource did not exist, as long as the
//							copy worked.
// ---------------------------------------------------------------------------
BOOL RemoveResource (int oldResfile, int newResfile, char * szFilename)
{
	ULONG			hash;
	LONG			i;
	LONG			cFound = 0;

	if (giHashType == HASH_CRC)
	{
		hash = HashCRC (szFilename);
	}
	else if (giHashType == HASH_ID)
	{
		hash = HashID (szFilename);
	}
	else
	{
		printf ("Invalid hash type (%d) for resource %s\n", giHashType, szFilename);
		return FALSE;
	}

	lseek (oldResfile, sizeof (RESFILE_HEADER), SEEK_SET);
	for (i = 0; i < grfHeader.cResources; i++)
	{
		RESOURCE_HEADER	rh;

		read (oldResfile, &rh, sizeof (rh));
		printf ("Read rh in RemoveResource\n");
		if (strncmp ((char *)&rh.startcode, szSTART_CODE, 4) != 0)
		{
			printf ("Error! Invalid resource in RemoveResource at %d\n",
				tell (oldResfile));
			return FALSE;
		}

		if (rh.hashValue != hash)
		{
			PTR	pBuffer;

			if ((pBuffer = malloc (rh.cbCompressedData)) == NULL)
			{
				printf("Unable to malloc input buffer\n");
				return FALSE;
			}
			printf ("About to read %d bytes of data\n", rh.cbCompressedData);
			read (oldResfile, pBuffer, rh.cbCompressedData);
			write (newResfile, &rh, sizeof (rh));
			write (newResfile, pBuffer, rh.cbCompressedData);
			free (pBuffer);
		}
		else
		{
			// Skip matching resource.
			cFound++;
			lseek (oldResfile, rh.cbChunk - sizeof (rh), SEEK_CUR);
		}
	}

	if (cFound == 0)
		printf ("Resource not found\n");
	else if (cFound == 1)
		printf ("Removed resource %s\n", szFilename);
	else
		printf ("Removed %d copies of resource %s\n", cFound, szFilename);
	return TRUE;
}


// ---------------------------------------------------------------------------
//    Function    -	
//		Description	-	Set everything up for the copy from current res file to
//							temp file, skipping the unwanted resource.  When done,
//							the old res file is renamed .BAK and the temp file is
//							renamed .RES.
//    Returns		-  Return code indicates whether we closed resfile (which is
//                   necessary to rename it).
//
//	NOTE: No longer working due to DIRECTORY!  Turned out not to be very
// useful anyway.
//
// ---------------------------------------------------------------------------
BOOL RemoveFile (int resfile, char * szFilename, char * szResfile)
{

	int	tempfile;
	char	szTempfile[_MAX_PATH];
	char	szExtension[_MAX_EXT];

	GetExtension (szResfile, szExtension, _MAX_EXT-1);
	if (strcmpi (szExtension, "BAK") == 0)
	{
		printf ("Cannot remove resources from files with extension .BAK\n");
		printf ("Please rename the file and try again.\n");
      return FALSE;                    // premature return
	}

	if (strcmpi (szExtension, "TMP") == 0)
	{
		printf ("Cannot remove resources from files with extension .TMP\n");
		printf ("Please rename the file and try again.\n");
      return FALSE;                    // premature return
	}

	strcpy (szTempfile, szResfile);
	ChangeExtension (szTempfile, "TMP");

	if ((tempfile = open (szTempfile, MODE_NEWFILE)) == fERROR)
	{
		printf ("Can't open file %s\n", szTempfile);
      return FALSE;                    // premature return
	}

	if (RemoveResource (resfile, tempfile, szFilename))
	{
		char	szBackup[_MAX_PATH];

		// Delete existing backup file if there is one.
		strcpy (szBackup, szResfile);
		ChangeExtension (szBackup, "BAK");
		if (Exists (szBackup))
		{
			remove (szBackup);
		}

		// Now change the filenames:
		//		resfile.res	-> resfile.bak
		//		resfile.tmp -> resfile.res
		close (resfile);
		close (tempfile);
		if (RenameFileExtension (szResfile, "BAK") != 0)
		{
			printf ("Could not give %d the extension BAK\n", szResfile);
		}
		if (RenameFileExtension (szTempfile, "RES") != 0)
		{
			printf ("Could not give %d the extension RES\n", szTempfile);
		}
		return TRUE;							// indicates we closed resfile
	}
	else
	{
		// Copy operation failed due to malloc failure.
		// TMP file is invalid, so delete it.
		close (tempfile);
		remove (szTempfile);
		return FALSE;
	}
}


// ---------------------------------------------------------------------------
//    Function    -	ExtractFile
//		Description	-	Handle the "e" command: extract the specified file
//							from the resource file and create it in the current
//							working directory.
//		Note			-	Does not remove resource from the resource file.
// ---------------------------------------------------------------------------
void ExtractFile (int resfile, char * szFilename)
{
	ULONG			hash;
	LONG			i;

	if (giHashType == HASH_CRC)
	{
		hash = HashCRC (szFilename);
	}
	else if (giHashType == HASH_ID)
	{
		hash = HashID (szFilename);
	}
	else
	{
		printf ("Invalid hash type (%d) for resource %s\n", giHashType, szFilename);
		return;
	}

	lseek (resfile, sizeof (RESFILE_HEADER), SEEK_SET);
	for (i = 0; i < grfHeader.cResources; i++)
	{
		RESOURCE_HEADER	rh;
		read (resfile, &rh, sizeof (rh));
		if (strncmp ((char *)&rh.startcode, szSTART_CODE, 4) != 0)
		{
			printf ("Error! Invalid resource in ExtractFile at %d\n",
				tell (resfile));
			return;
		}

		if (rh.hashValue == hash)
		{
			PTR	pInBuffer;
			PTR	pOutBuffer;
			PTR	pEnd;
			int	newfile;

			if ((newfile = open (szFilename, MODE_NEWFILE)) == fERROR)
			{
				printf ("Can't open file %s\n", newfile);
				return;                    // premature return
			}

			if ((pInBuffer = malloc (rh.cbCompressedData)) == NULL)
			{
				printf("Unable to malloc input buffer\n");
				close (newfile);
				return;                    // premature return
			}

			printf ("About to read %d bytes of data\n", rh.cbCompressedData);
			read (resfile, pInBuffer, rh.cbCompressedData);

			if (rh.compressionCode == NO_COMPRESSION)
			{
				printf ("About to write %d bytes of data\n", rh.cbCompressedData);
				write (newfile, pInBuffer, rh.cbCompressedData);
			}
			else if (rh.compressionCode == LZSS_COMPRESSION)
			{
				if ((pOutBuffer = malloc (rh.cbUncompressedData + 2048L)) == NULL)
				{
					printf("Unable to malloc input buffer\n");
					close (newfile);
					free (pInBuffer);
					return;                    // premature return
				}
	
				memset (pOutBuffer, 0xdc, rh.cbUncompressedData + 2);
				printf ("About to decompress %d bytes of data to %d\n",
					rh.cbUncompressedData, pOutBuffer);
				pEnd = pOutBuffer + rh.cbUncompressedData + 2;
				printf ("pMax = %P...decompressing...", pEnd);
				pEnd = _LZSSDecode_(pInBuffer, pOutBuffer, pEnd);
				printf ("pEnd = %P\n", pEnd);
	
				printf ("About to write %d bytes of data\n", rh.cbUncompressedData);
				write (newfile, pOutBuffer, rh.cbUncompressedData);
				free (pOutBuffer);
			}
			close (newfile);
			free (pInBuffer);
			break;
		}
		lseek (resfile, rh.cbChunk - sizeof (rh), SEEK_CUR);
	}
}


// ---------------------------------------------------------------------------
//    Function    -	ListContents
//		Description	-	Handle the "l" command: list the files in the resource file.
// ---------------------------------------------------------------------------
void ListContents (int resfile, char * szResfile, BOOL fVerify)
{
	int					i;

	if (filelength (resfile) == 0)
	{
		printf ("Resource file %s is empty\n", szResfile);
		return;									// premature return
	}

	printf ("Contents of %s:\n", szResfile);

   if (fVerify)
   {
      // Method 1: skip from resource header to resource header.
      RESFILE_HEADER rfh;

   	lseek (resfile, 0, SEEK_SET);
      read (resfile, &rfh, sizeof (rfh));
      if (rfh.versionResFile != RESUTIL_VERSION)
      {
         printf ("Error in resfile version, %d instead of %d\n",
            rfh.versionResFile, RESUTIL_VERSION);
         return;                       // premature return
      }

   	for (i = 0; i < grfHeader.cResources; i++)
   	{
   		RESOURCE_HEADER	rh;

   		read (resfile, &rh, sizeof (rh));
   		printf ("%4d %12s %6d %d %s",
   			i, rh.szName, rh.cbUncompressedData, rh.flags,
   			gszCompressionType[rh.compressionCode]);
   		if (rh.compressionCode == LZSS_COMPRESSION)
   			printf (" (%6d)", rh.cbCompressedData);
   		printf ("\n");
   		if (strncmp ((char *)&rh.startcode, szSTART_CODE, 4) != 0)
   		{
   			printf ("Error! Resource %d is invalid\n", i);
   			return;                    // premature return
   		}
   		lseek (resfile, rh.cbChunk - sizeof (rh), SEEK_CUR);
   	}
   }
   else
   {
      // Method 2: use the E-Z resource directory at the end of the file.
   	lseek (resfile, grfHeader.oDirectory, SEEK_SET);
   	for (i = 0; i < grfHeader.cResources; i++)
   	{
   		DIRENTRY	direntry;

   		read (resfile, &direntry, sizeof (direntry));
   		printf ("%4d %12s %6d\n", i, direntry.szName, direntry.resOffset);
   	}
	}
}


// ---------------------------------------------------------------------------
//    Function    -	ProcessResponseFile
//		Description	-	Handle the "@filename" command.
// 	Note:
//			Response file commands follow the strict rule of one command per line.
//			You can do this in a response file:
//				+ file1.pcx
//				r
//				+ file2.pcx
//				u
//				+ file3.flc
//
//			This adds file1.pcx to the resource file, then sets the global
//			gfRotate flag TRUE, then adds file2.pcx, rotated, to the resource
//			file.  Both files will be compressed if that makes them smaller.
//			The u command then sets the uncompressed flag and adds file3.flc to
//			the resource file without compressing it.
//
//			The flag commands that you can use are:
//				c		[allow compression (DEFAULT)]
//				n		[do not rotate PCX resources (DEFAULT)]
//				r		[rotate PCX resources]
//				u		[store data uncompressed]
//
//			These flags go on a line all by themselves.
//
//			Flags stay in effect for all following resources until changed
//			by the complementary flag (c and u are complements, n and r are
//			complements).
//
//			Only PCX-type resources pay attention to the gfRotate flag.
// ---------------------------------------------------------------------------
void ProcessResponseFile (int resfile, char * szFilename, char * szResfile)
{
	FILE *		responseFile;
	int			operation;
	int			iNextCommandChar;
	ULONG			result;
	char 			szCommandLine[CMDLINE_LENGTH+1];

	if ((responseFile = fopen (szFilename, "r")) == NULL)
	{
		printf ("Can't open file %s\n", szFilename);
		return;                    // premature return
	}

	printf ("Opened %s\n", szFilename);
	while (1)
	{
		result = GetNextLine (responseFile, szCommandLine, CMDLINE_LENGTH);
		if (result == EOF)
			break;
		iNextCommandChar = 1;
		switch (toupper (szCommandLine[0]))
		{
			case '+':
				operation = RFU_ADD;
				break;
			case '-':
				operation = RFU_REMOVE;
				break;
			case 'C':
				gfCompress = TRUE;
				break;
			case 'E':
				operation = RFU_EXTRACT;
				break;
			case 'H':
				if (toupper (szCommandLine[1]) == 'C')
					giHashType = HASH_CRC;
				else if (toupper (szCommandLine[1]) == 'I')
					giHashType = HASH_ID;
				else
					printf ("Invalid hash type requested\n");
				iNextCommandChar = 2;
				break;
			case 'N':
				gfRotate = FALSE;
				break;
			case 'R':
				gfRotate = TRUE;
				break;
			case 'U':
				gfCompress = FALSE;
				break;
			default:
				printf ("Invalid command in response file: %s\n", szCommandLine[0]);
				break;
		}

		while (szCommandLine[iNextCommandChar] == ' ')
			iNextCommandChar++;

		// printf ("Operation = %d, name = %s\n", operation, &szCommandLine[iNextCommandChar]);

		switch (operation)
		{
			case RFU_ADD:
				AddFile (resfile, &szCommandLine[iNextCommandChar]);
				break;
			// case RFU_REMOVE:
			// 	if (RemoveFile (resfile, &szCommandLine[iNextCommandChar], szResfile))
			// 	{
			// 		if ((resfile = open (szResfile, MODE_MAKEFILE)) == fERROR)
			// 		{
			// 			printf ("Can't open resfile %s in function ProcessResponseFile\n", szResfile);
			// 			fclose (responseFile);
			// 			exit (1);
			// 		}
			// 	}
			// 	break;
			case RFU_EXTRACT:
				ExtractFile (resfile, &szCommandLine[iNextCommandChar]);
				break;
		}
	}
	fclose (responseFile);
}


// ---------------------------------------------------------------------------
//    Function    -	StrPeriod
//		Description	-	Return pointer to period in filename.  (Need to
//                   distinguish the extension period from '..\'
//                   and can't use strrchr because there may not
//                   be an extension.
// ---------------------------------------------------------------------------
char * StrPeriod (char * sz)
{
	char	*	pch;

	pch = strstr (sz, "..\\");
	if (pch == NULL)
		return strchr (sz, '.');

	pch += 3;
	return strchr (pch, '.');
}


// ---------------------------------------------------------------------------
//    Function    -	AddExtension
//		Description	-	Add an extension to a filename that doesn't have one.
//							To defeat this function when you really don't want an
//                   extension, specify a filename of "name."
//							The extension string can have a period at the beginning.
// ---------------------------------------------------------------------------
void AddExtension (char * szFilename, char * szExtension)
{

	if (StrPeriod (szFilename) == NULL)
	{
		if (StrPeriod (szExtension) == NULL)
			strcat (szFilename, ".");
		strcat (szFilename, szExtension);
	}
}



// ---------------------------------------------------------------------------
//    Function    -	ChangeExtension
//		Description	-	Change a filename's extension.  If the original filename
//							does not have an extension, it will get the specified extension.
//							The extension string can have a period at the beginning.
// ---------------------------------------------------------------------------
void ChangeExtension (char * szFilename, char * szExtension)
{
	char *		pPeriod;

	pPeriod = StrPeriod (szFilename);

	if (pPeriod == NULL)
	{
		if (StrPeriod (szExtension) == NULL)
			strcat (szFilename, ".");
		strcat (szFilename, szExtension);
	}
	else
	{
		if (StrPeriod (szExtension) == NULL)
			pPeriod++;
		strcpy (pPeriod, szExtension);
	}
}


// ---------------------------------------------------------------------------
//    Function    -	GetExtension
//		Description	-	Returns file szFilename's extension in
//							parameter szExtension.
// ---------------------------------------------------------------------------
void GetExtension (char * szFilename, char * szExtension, int maxChars)
{
// Returns file extension, no period.

	char *		pPeriod;

	pPeriod = StrPeriod (szFilename);

	if (pPeriod == NULL)
		strcpy (szExtension, "");
	else
		strncpy (szExtension, pPeriod + 1, maxChars);

	// printf ("GetExtension: %s\n", szExtension);
}


// ---------------------------------------------------------------------------
//    Function    -	GetExtensionCode
//		Description	-	Finds code for file extension.
//		Returns		-	Code for file extension, or 0 if not found.
// ---------------------------------------------------------------------------
UBYTE GetExtensionCode (char * szFilename)
{
	int			i;
	char *		pExtension;

	pExtension = StrPeriod (szFilename);

	if (pExtension == NULL || strlen (pExtension) == 1)
	{
		return 0;								// no extension
	}
	else
	{
		pExtension++;							// skip the period
		for (i = 1; i < cMAX_EXTENSIONS; i++)
		{
			if (strcmpi (pExtension, gszExtensions[i]) == 0)
				return i;
		}
	}

	return 0;					// unknown extension
}


// ---------------------------------------------------------------------------
//    Function    -	RenameFileExtension
//		Description	-	Renames a file extension without changing the base name.
//		Returns		-  same as the library function rename: 0 if it worked,
//							nonzero if it didn't.
// ---------------------------------------------------------------------------
int RenameFileExtension (char * szFilename, char * szExtension)
{
	char			sznewFilename[_MAX_PATH];

	strcpy (sznewFilename, szFilename);
	ChangeExtension (sznewFilename, szExtension);
	return rename (szFilename, sznewFilename);
}


// ---------------------------------------------------------------------------
//    Function    -	main
//		Description	-	Parse command line, open res file, dispatch commands.
// ---------------------------------------------------------------------------
void main (int argc, char *argv[])
{
	int			resfile;
	int			iArg;
	char *		szFilename;
	RFU_OP		operation;
	char			szResfile[_MAX_PATH];
	char			szExtension[_MAX_EXT];

	if (argc < 3)
	{
		puts ("Usage: resutil resfile-name [[s nnnnn] [c] [n] [r] [u] [+|-|e sourcefile-name ]] |\n");
		puts (" [@ respfile-name] | [l] | [v]");
		puts ("   +  add file");
		// puts ("    - remove file");
		puts ("   c  compress resources");
		puts ("   e  extract file (does not remove it)");
		puts ("   hc use CRC hash (default)");
		puts ("   hi use ID hash");
		puts ("   l  list contents of resource file");
		puts ("   n  do not rotate PCX resources");
		puts ("   r  rotate PCX resources");
		puts ("   s  nnnnn max size of resource permitted");
		puts ("   u  do not compress resources");
		puts ("   v  verify resource file");
		puts ("   @  respfile run commands in respfile");
		exit (1);
	}

	gcResSlots = cRES_SLOTS_IN_BLOCK;
	gpaDirentry = (DIRENTRY_PTR) malloc (gcResSlots * sizeof (DIRENTRY));
	if (gpaDirentry == NULL)
	{
		puts ("Error! Could not allocate memory for directory");
		exit (1);
	}

	grfHeader.oDirectory = sizeof (RESFILE_HEADER);
	grfHeader.cResources = 0;
	grfHeader.versionResFile = RESUTIL_VERSION;

	strcpy (szResfile, argv[1]);
	AddExtension (szResfile, "res");
	GetExtension (szResfile, szExtension, _MAX_EXT-1);
	if (strcmpi (szExtension, "res") != 0)
	{
		int c;

		printf ("The extension %s is not standard.\n", szExtension);
		printf ("Are you sure you want to use the resource file %s? ", szResfile);
		c = toupper (getchar ());
		if (c != 'Y')
		{
			free (gpaDirentry);
			exit (1);
		}
	}

	if ((resfile = open (szResfile, MODE_MAKEFILE)) == fERROR)
	{
		printf ("Can't open resfile %s\n", szResfile);
		free (gpaDirentry);
		exit (1);
	}

	if (filelength (resfile) > 0)
	{
		LoadResfileHeader (resfile);
		LoadDirectory (resfile);
	}

	for (iArg = 2; iArg < argc; iArg++)
	{
		BOOL	fFilenameOp;

		fFilenameOp = FALSE;
		switch (toupper (argv[iArg][0]))
		{
			case '+':
				operation = RFU_ADD;
				fFilenameOp = TRUE;
				break;
			case '-':
				operation = RFU_REMOVE;
				fFilenameOp = TRUE;
				break;
			case 'C':
				gfCompress = TRUE;
				break;
			case 'E':
				operation = RFU_EXTRACT;
				fFilenameOp = TRUE;
				break;
			case 'H':
				if (toupper (argv[iArg][1]) == 'C')
					giHashType = HASH_CRC;
				else if (toupper (argv[iArg][1]) == 'I')
					giHashType = HASH_ID;
				else
					printf ("Invalid hash type requested\n");
				break;
			case 'L':
				operation = RFU_LIST;
				break;
			case 'N':
				gfRotate = FALSE;
				break;
			case 'R':
				gfRotate = TRUE;
				break;
			case 'S':
				if (strlen(argv[iArg]) > 1)
				{
					gMaxLen = atol (&argv[iArg][1]);
				}
				else
				{
					gMaxLen = atol (argv[iArg+1]);
					iArg++;
				}
				printf ("gMaxLen = %d\n", gMaxLen);
				break;
			case 'U':
				gfCompress = FALSE;
				break;
         case 'V':
				operation = RFU_VERIFY;
            break;
			case '@':
				operation = RFU_RESPONSEFILE;
				fFilenameOp = TRUE;
				break;
			default:
				printf ("Unknown command %s\n", argv[iArg]);
				close(resfile);
				free (gpaDirentry);
				exit (1);
		}

		if (fFilenameOp)
		{
			if (strlen(argv[iArg]) > 1)
			{
				szFilename = &argv[iArg][1];
			}
			else
			{
				szFilename = argv[iArg+1];
				iArg++;
			}
			printf ("szFilename = %s\n", szFilename);
		}
	}

	// printf ("Operation = %d\n", operation);

	// if (operation != RFU_LIST)
	// {
	// 	// Get filename; user may or may not have put a space after the command.
	// 	if (strlen(argv[2]) > 1)
	// 	{
	// 		szFilename = &argv[2][1];
	// 	}
	// 	else
	// 	{
	// 		szFilename = argv[3];
	// 	}
	// 	printf ("szFilename = %s\n", szFilename);
	// }


	switch (operation)
	{
		case RFU_ADD:
			AddFile (resfile, szFilename);
			break;
		// case RFU_REMOVE:
		// 	if (RemoveFile (resfile, szFilename, szResfile))
		// 		resfile = fERROR;
		// 	break;
		case RFU_EXTRACT:
			ExtractFile (resfile, szFilename);
			break;
		case RFU_LIST:
			ListContents (resfile, szResfile, FALSE);
			break;
		case RFU_RESPONSEFILE:
			ProcessResponseFile (resfile, szFilename, szResfile);
			break;
      case RFU_VERIFY:
			ListContents (resfile, szResfile, TRUE);
         break;
	}

	if (resfile != fERROR)
	{
		int fLength;

		fLength = filelength (resfile);
		close (resfile);
		if (fLength == 0)
		{
			printf ("File length = 0, deleting %s\n", szResfile);
			remove (szResfile);
		}
	}
	free (gpaDirentry);
	exit (0);
}


// ---------------------------------------------------------------------------
// 	HashCRC	- Uses polynomial division to hash a string into a word
//		Taken verbatim from BIRTHRT\RESMANAG.C.
// ---------------------------------------------------------------------------
ULONG HashCRC(register CSTRPTR szIn)
{
	CSTRPTR				sz;
	register ULONG		c;
	register ULONG		accumCRC = 0;
	USHORT				accumXOR = 0;

	// hashing JUST the filename and extension is better
	sz = strrchr(szIn, '\\');
	if (sz == NULL)
		sz = szIn;		// no path character (\)
	else
		sz++;				// next character after path char (\)

	do
	{
		c = *sz++;

		// make uppercase
		if (c >= 'a' && c <= 'z')
		{
			c -= ('a' - 'A');
		}

		// yes, I know this only uses the bottom byte.
		accumXOR = accumXOR ^ c;
		
		accumCRC = (accumCRC << 6) + ((c - ' ') & 63);

//		for (m=1<<21,k=0x1021<<5; m>(1<<15); m>>=1,k>>=1)
//			if (accumCRC & m)
//				accumCRC ^= k;

		// Unrolled for loop
		if (accumCRC & (1 << 21))
			accumCRC ^= 0x1021 << 5;

		if (accumCRC & (1 << 20))
			accumCRC ^= 0x1021 << 4;

		if (accumCRC & (1 << 19))
			accumCRC ^= 0x1021 << 3;

		if (accumCRC & (1 << 18))
			accumCRC ^= 0x1021 << 2;

		if (accumCRC & (1 << 17))
			accumCRC ^= 0x1021 << 1;

		if (accumCRC & (1 << 16))
			accumCRC ^= 0x1021;

	} while (c);

	return (BUILD_LONG(accumXOR,(USHORT)(accumCRC & 0xFFFF)));
}


// ---------------------------------------------------------------------------
// 	Function    - HashID
//    Description - Generate a unique number for each .AVD resource file.
//    				 Guess what?!? The ID is guaranteed  to be unique, so just
//    				 parse it out.
//    Returns     - a USHORT of the id.
//		Notes			4-24-97 Added for .WAV files.
// ---------------------------------------------------------------------------
ULONG HashID(CSTRPTR szFileName)
{
	ULONG	Result = 0;
	CHAR	c;
	CSTRPTR	sz = strrchr(szFileName, '\\');
	
	if (sz == NULL)
	{
		sz = szFileName;	// No path character (\)
	}
	else
	{
		sz++;				// Next character after path char. (\)
	}
	
	c = *sz;
	while (!isdigit(c) )
	{
		if (c == 0)
#if defined (_DEBUG)
			printf ("HashID ERROR! bad format for ID type file name %s.\n",
															szFileName);
			exit (1);
#else
			return 1000;		// Marlae Roesone.
#endif
		sz++;
		c = *sz;
	}
	
	while (isdigit(c) && c != '.')
	{
		Result = (10 * Result) + (c - '0');
		
		sz++;
		c = *sz;
	}
	
	return Result;
}


// ---------------------------------------------------------------------------
//    Function    -	Exists
//		Description	-	Check whether a file exists.
//		Returns		-	TRUE if file exists, FALSE if not
// ---------------------------------------------------------------------------
BOOL Exists( char *filename)
{
	return(!access(filename, 00));
}


// ---------------------------------------------------------------------------
//    Function    - GetNextLine
//    Description - Get the next line of the response file. Skip comments,
//						  i.e., '#' in column zero.
//                  Also skip leading white space (and thus blank lines).
//    Returns     - Either EOF or the number of characters retrieved.
// ---------------------------------------------------------------------------
ULONG GetNextLine(FILE *fp, char *cpNewLine, LONG len)
{
	ULONG result = 0;

	// printf ("At GetNextLine\n");
	do
	{
		LONG i;
		
		cpNewLine[0] = 0; // Initialize it incase the read fails.
		if( NULL == fgets(cpNewLine, len, fp))
		{
			result = EOF;
			break;
		}
		
		// Advance past the white space.
		for (i = 0; isspace(cpNewLine[i]) && cpNewLine[i] != 0; i++ )
		{
		}
		
		if (cpNewLine[i] == 0)
		{
			// End of line found.
			cpNewLine[0] = 0;
		}
		else
		if (i > 0) // Leading white space found.
		{
			char *cpShiftedLine;
			char *cpBeginText;
			
			cpShiftedLine = cpNewLine;
			cpBeginText = cpNewLine + i;
			// Cheap inplace string shift.
			do
			{
				*cpShiftedLine = *cpBeginText;
				cpBeginText++;
				cpShiftedLine++;
			}
			while(*cpBeginText);
			
			*cpShiftedLine = 0;
		}
	
	} while (cpNewLine[0] == '#' ||
	         cpNewLine[0] == 0 ||
	         cpNewLine[0] ==  '\n');
	
	if (result != EOF)
	{
		LONG LastCharIndex;
		
		result = strlen(cpNewLine);
		
		
		// Trim off the trailing <LF> <CR> and white space
		for (LastCharIndex = result - 1;
		     LastCharIndex > - 1 &&
		     (cpNewLine[LastCharIndex] == '\n' ||
		      cpNewLine[LastCharIndex] == '\r' ||
		      isspace(cpNewLine[LastCharIndex]));
			 LastCharIndex--, result--)
		{
			cpNewLine[LastCharIndex] = 0;
		}
	}
	
	return result;
}


// ---------------------------------------------------------------------------
static BOOL SaveFileNameTooBig (CSTRPTR szFilename)
{
// Call this function to save the name of a file that has been rejected
// because it is too big.  We are using it for WAV files currently.
// [d4-03-97 JPC] Use this function to create a batch file to do the copying.

	FILE *		fp;

	if (!gfInited)
	{
		gfInited = TRUE;
		fp = fopen (TOOBIG_FILE, "wt");
		if (fp != NULL)
		{
			fputs ("REM Batch file for copying large files to CD.\n", fp);
			fputs ("REM Parameter is the destination path.\n", fp);
			fputs ("REM Directory structure must exist on destination.\n\n", fp);
			fclose (fp);
		}
	}

	fp = fopen (TOOBIG_FILE, "at");
	if (fp != NULL)
	{
		char szFullPath[_MAX_PATH + 1];
		char * szNoDrive;
		char szBuffer[8 + 2 * _MAX_PATH];

		_fullpath (szFullPath, szFilename, _MAX_PATH);
		szNoDrive = strchr (szFullPath, ':');
		if (szNoDrive == NULL)
		{
			szNoDrive = szFullPath;
		}
		else
		{
			szNoDrive++;
		}
		sprintf (szBuffer, "copy %s %%1%s\n", szFullPath, szNoDrive);
		fwrite (szBuffer, 1, strlen (szBuffer), fp);
		fclose (fp);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


// ---------------------------------------------------------------------------
void LoadDirectory (int resfile)
{
	LONG			i;
	DIRENTRY_PTR	newPtr;

	gcResSlots = grfHeader.cResources;
	newPtr = (DIRENTRY_PTR) realloc (gpaDirentry, gcResSlots * sizeof (DIRENTRY));
	if (newPtr == NULL)
	{
		printf ("Error! not enough memory to expand directory\n");
		return;
	}
	gpaDirentry = newPtr;
	lseek (resfile, grfHeader.oDirectory, SEEK_SET);
	for (i = 0; i < grfHeader.cResources; i++)
	{
		read (resfile, &gpaDirentry[i], sizeof (gpaDirentry[i]));
	}
}


// ---------------------------------------------------------------------------
void SaveDirectory (int resfile)
{
	LONG			i;

	lseek (resfile, grfHeader.oDirectory, SEEK_SET);
	for (i = 0; i < grfHeader.cResources; i++)
	{
		write (resfile, &gpaDirentry[i], sizeof (gpaDirentry[i]));
	}
}


// ---------------------------------------------------------------------------
void LoadResfileHeader (int resfile)
{
	read (resfile, &grfHeader, sizeof (grfHeader));
	if (grfHeader.versionResFile != RESUTIL_VERSION)
	{
		printf ("Error! Wrong file version\n");
		exit (1);
	}
}


// ---------------------------------------------------------------------------
void SaveResfileHeader (int resfile)
{
	lseek (resfile, 0, SEEK_SET);
	write (resfile, &grfHeader, sizeof (grfHeader));
}


// ---------------------------------------------------------------------------
void AddDirectoryEntry (RESOURCE_HEADER * prh)
{
	size_t		size;

	if (grfHeader.cResources >= gcResSlots)
	{
		DIRENTRY_PTR newPtr;

		gcResSlots += cRES_SLOTS_IN_BLOCK;
		size = gcResSlots * sizeof (DIRENTRY);
		newPtr = (DIRENTRY_PTR) realloc (gpaDirentry, size);
		if (newPtr != NULL)
		{
			gpaDirentry = newPtr;
		}
		else
		{
			printf ("Error! not enough memory to expand directory\n");
			return;
		}
	}
	gpaDirentry[grfHeader.cResources].hashValue = prh->hashValue;
	gpaDirentry[grfHeader.cResources].fileExtension = prh->fileExtension;
	strncpy (gpaDirentry[grfHeader.cResources].szName, prh->szName, cMAX_RESNAME);
	gpaDirentry[grfHeader.cResources].resOffset = grfHeader.oDirectory;

	// Now update the resfile header.
	grfHeader.cResources++;
	grfHeader.oDirectory += prh->cbChunk;
}


// ---------------------------------------------------------------------------
static PTR DecompressPCX (PTR pInput, RESOURCE_HEADER * prh)
{
// This function converts a PCX file into a BITMAP resource that we can
// use directly in BIRTHRIGHT.
// If the global variable gfRotate is TRUE, the resource is rotated.

	PTR				pSrc, pDest, pSS, pDD;
	PTR				pOutput;
	USHORT *			pFinal;
	ULONG				cBytes, cbDecompressed, cbLargest, cbFinal;
	ULONG				cbSrcOffset, cbDestOffset;
	ULONG				l, ll, w;
	USHORT			j, k;
	UBYTE				c;
	ULONG				cbSafety = 2048L;		/* default to a safety buffer of 2k bytes */
	PCXHDR			PCX;

	pSrc = pInput;

	// At this point, pSrc points to an image in memory of the PCX file.
	// Get the PCX header.
	memcpy (&PCX.code, pSrc, sizeof (PCXHDR));
	PCX.w = PCX.w + (SHORT)1 - PCX.xo;			/* calc w and h */
	PCX.h = PCX.h + (SHORT)1 - PCX.yo;

	pSrc += 128L;								// skip to the PCX compressed data

	cBytes = prh->cbUncompressedData - 128L;

	/* cbDecompressed is the size of the decompressed graphic */
	cbDecompressed = (ULONG)PCX.cbLine * (ULONG)PCX.h;

	/* cbFinal is the size of the final graphic */
	cbFinal			= cbDecompressed + sizeof(BITMHDR);

	// We don't modify cbSafety here, because unlike LoadPCX, we don't
	// have one big buffer, we have two buffers.

	/* cbLargest is the size of the buffer to allocate for all operations */
	cbLargest		= MAX(cbFinal, cBytes) + cbSafety;

	/* cbSrcOffset is where the file is loaded */
	// Unlike LoadPCX, this points to a different buffer.
	cbSrcOffset		= 128L;					// size of PCX header + reserved space

	/* cbDestOffset is where the file is decompressed to */
	cbDestOffset = sizeof(BITMHDR);


	/* -------------------------------------------------------- */
	/* Allocate space for data */
	/* -------------------------------------------------------- */
	pOutput = malloc (cbLargest);

	if (pOutput == NULL)
		return NULL;

	memset (pOutput, 0, cbLargest);		// [d4-28-97 JPC] fix TITLE40.PCX bug

	pDest = pOutput + cbDestOffset;
	pFinal = (USHORT *) pOutput;


	/* -------------------------------------------------------- */
	/* Decompress the data from PCX-compressed format  */
	/* -------------------------------------------------------- */
	/* This run length decoding routine is unique to NOVA */
	if (gfRotate)
	{
		// Remember that the gfRotate flag is set by the "r"
		// command and remains set until we encounter the "n"
		// command in the response file.
		pSS = pSrc;
		ll = PCX.cbLine * PCX.h;
		w =  PCX.w * PCX.h;

		for (j=0; j < PCX.h; j++)
		{
			pDD = pDest + j;
			w = l = 0;
			do
			{
				c = *(pSS++);
				if ((c & 0xC0) == 0xC0)					/* test for run */
				{
					k = c & 0x3F;							/* run length */
					c = *(pSS++);							/* run byte */
					
					w += k;	// GWP optimization.
					while(k--)
					{
						pDD[l] = c;
						l += PCX.h;
						// GWP 	w++;
					}
				}
				else											/* unique byte */
				{
					pDD[l] = c;
					l += PCX.h;
					w++;
				}
			} while (l < ll);
			if (PCX.w < PCX.cbLine)
				pDD[l-PCX.h] = 0;
		}
		PCX.w = PCX.cbLine;
		l = PCX.w;				/* reverse the width and height */
		PCX.w = PCX.h;
		PCX.h = (SHORT)l;
	}

	/* This run length decoding routine is unique to PCX */
	else
	{
		pDD = pDest;
		pSS = pSrc;

		w = 0;
		for (l=0; l < cbDecompressed; )
		{
			c = *(pSS++);
			if ((c & 0xC0) == 0xC0)					/* test for run */
			{
				k = c & 0x3F;							/* run length */
				c = *(pSS++);							/* run byte */
				
				w += k;	// GWP optimization.
				l += k; // GWP optimization.
				memset(pDD, c, k);	// GWP optimization.
				pDD += k;	// GWP optimization.
				
				// GWP for (; k>0; k--)
				// GWP {
				// GWP 	*(pDD++) = c;
				// GWP 	l++;
				// GWP 	w++;
				// GWP }
			}
			else											/* unique byte */
			{
				*(pDD++) = c;
				l++;
				w++;
			}
			// [d4-28-97 JPC] Original code:
			// if (w == PCX.w)
			// 	*(pDD-1) = 0;
		}
		// [d4-29-97 JPC]
		// New code to remove FFs from the right edge of the picture.
		// (Apparently if a PCX has an odd width the PCX compression
		// rule is to stick one FF at the right edge to make the line
		// work out to an even number.)
		if (PCX.w < PCX.cbLine)			// same test as in rotated version, above
		{
			ULONG	h;
			for (h = 0, w = PCX.w; h < PCX.h; h++, w += PCX.cbLine)
				*(pDest + w) = 0;
		}
		// [d4-28-97 JPC] end of change.
		PCX.w = PCX.cbLine;
	}

	/* -------------------------------------------------------- */
	// The safety buffer can be removed after decompression.
	/* -------------------------------------------------------- */
	if (cbFinal != cbLargest)
	{
		PTR newPtr = (PTR) realloc (pOutput, cbFinal);
		if (newPtr != NULL)
		{
			pOutput = newPtr;
		}
		else
		{
			printf ("Could not realloc pOutput in DecompressPCX\n");
			free (pOutput);
			return NULL;
		}
	}

	prh->cbUncompressedData = cbFinal;

	// Get the pointer again, because memory may have moved.
	pFinal = (USHORT *) pOutput;
	
	pFinal[0] = PCX.w;
	pFinal[1] = PCX.h;
	pFinal[2] = UNITARY_SCALE;				// for resource building, assume no scale.
	pFinal[3] = 0;								// x center point
   pFinal[4] = TYPEBITM;               // type

	return pOutput;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

