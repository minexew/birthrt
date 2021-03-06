//-----------------------------------------------------------------------------
//
//        Copyright 1995/1996 by NW Synergistic Software, Inc.
//
//        SndUtil.c - Provides interface for playing wave files.
//
//------------------------------------------------------------------------------

#if defined (__cplusplus)
extern "C" {
#endif

int WaveOpenFile( char          *pszFileName,
                  HMMIO         *phmmioIn,
                  WAVEFORMATEX  **ppwfxInfo,
                  MMCKINFO      *pckInRIFF );

int WaveReadFile( HMMIO       hmmioIn,
                  UINT        cbRead,
                  BYTE        *pbDest,
                  MMCKINFO    *pckIn,
                  UINT        *cbActualRead );

int WaveStartDataRead( HMMIO       *phmmioIn,
                       MMCKINFO    *pckIn,
                       MMCKINFO    *pckInRIFF );


#if defined (__cplusplus)
}

// wave.h
#endif