// ===========================================================================
// 	LZSS.H
// 	Prototypes for assembly-language LZSS encode and decode.
// ===========================================================================

#if !defined (_LZSS_)
#define _LZSS_

PTR _LZSSEncode_ (PTR, PTR, ULONG);
// void _LZSSDecode_ (PTR, PTR);
PTR _LZSSDecode_ (PTR, PTR, PTR);

#endif

