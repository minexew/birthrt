/* ========================================================================
   Copyright (c) 1990,1996	Synergistic Software
   All Rights Reserved
   ========================================================================
   Filename: scene.cpp  -
   Author:   Gary Powell
   
   ========================================================================
   
   Contains the following general functions:

   
   ======================================================================== */

/* ------------------------------------------------------------------------
   Includes
   ------------------------------------------------------------------------ */
#include <ctype.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"
#include "engine.h"
#include "machine.h"

#include "fileutil.h"
#include "scncamr.hxx"

/* ------------------------------------------------------------------------
   Notes
   ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------
   Defines and Compile Flags
   ------------------------------------------------------------------------ */
   
/* ------------------------------------------------------------------------
   Macros
   ------------------------------------------------------------------------ */
   
/* ------------------------------------------------------------------------
   Prototypes
   ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------
   Global Variables
   ------------------------------------------------------------------------ */
	
/* ========================================================================
   Function    - mfReadCameraData
   Description - Read the Camera section of the scene file.
   Returns     - 
   ======================================================================== */
LONG CAMERA_SETUP_DATA::mfReadCameraData(FILE *fp)
{
	char cpBuffer[40];
	LONG Result = GetNextLine(fp, cpBuffer, sizeof(cpBuffer));

	if (Result != EOF)
	{
		LONG i;
		const LONG count = atol(cpBuffer);
		
	    for (i = 0; i < count; i++)
	    {
			char cpKeyword[20];
			LONG Value;
			
			Result = GetNextLine(fp, cpBuffer, sizeof(cpBuffer));
			
			if (Result != EOF && 2 == sscanf(cpBuffer,"%s %ld", cpKeyword, &Value))
			{
				if (strcmp(cpKeyword, "Accelleration") == 0)
				{
					mfSetCameraAccelleration(Value);
				}
			}
		}
	}
	
	return Result;
}
