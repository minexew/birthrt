;; ננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננ
;;
;;    File:    lzssencd.asm - LZSS compressing routine
;;
;;    History:
;;    10/16/95		MikeB	Ported to 32 bit pure assembly
;;
;; ננננננ  (c) 1990-6  Synergistic Software  ננננננננננננננננננננננננננננננננננ
;;
;;	Theory of LZSS compression:
;;	Scan through source bytes looking for matches with previous data.
;;	If the match length is less than 3 then output a byte verbatim,
;;	otherwise output a run. When data remaining is zero, output a run
;;	with a zero token.
;;
;; ננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננ

.386P
.MODEL SMALL, C
.DATA

;	pData			dd ?
;	pOutput		dd ?
;	mylength		dd ?

	pDest			dd	?
	MatchStart	dd ?
	index			dd ?
	bfcount		dw ?
	bitflags		dw ?
;	idots			dw ?

	buffer		db	100 dup(0)

;	spin			db	'/'
;					db	'-'
;					db	'\'
;					db	'|'

.CODE
EVEN

LZSSEncode_	PROC	C USES	ESI EDI EBX ECX EDX, pData:PTR BYTE, pOutput:PTR BYTE, mylength:DWORD
	PUBLIC LZSSEncode_

; we are register based, save the incoming data
;	mov	pData, eax
;	mov	pOutput, edx
;	mov	mylength, ebx

;	mov	word ptr idots, 0

	cld						;  direction is forward
	mov	word ptr index, 0;  init some variables
	mov	word ptr bitflags, 0;
	mov	word ptr bfcount, 16;
	mov	esi, pData	;  get pointer to current position

	mov	pDest, esi;  save dest into pDest

EncodeLoop:
; check length for possibility of a run. if less than 3 then no run
	mov	eax, mylength	;  get length
	cmp	eax, 3			;  test it
	jae	FindMatch		;
	jmp	OutVerbatim		;  run is too short

;	Search through a block of data for the largest string which matches a
;	given string. edi points to the block of data (up to 1023 bytes).
;	esi points to the string to be matched. The block of data appears in
;	memory immediately before the string to be matched.

FindMatch:
; set the rep counter
	mov	ecx, esi			;
	sub	ecx, edi			;  set rep counter to si-di-1
	cmp	ecx, 2			;  first two bytes must be verbatim
	jb		OutVerbatim		;
	dec	ecx				;  byte before current is # zero for checking
	cmp	ecx, 1023		;  max check of 1023 bytes
	jb		NotAbove			;
	mov	ecx, 1023		;  set to 1023 bytes
NotAbove:
	mov	edx, pData		;  start edx equal to esi, which is set next

FindMatchLoop:
; find match for first byte from current position
	mov	esi, pData		;  load si from pointer to current position
	lodsb						;  load first byte from current pos into al
	repne	scasb				;  repeat while !=, compare al to [edi]
	jne	FindMatchDone	;  not equal if no match was found
	mov	eax, edi			;  save copy of di
	mov	ebx, ecx			;  save start of area to test match in bx

; find match for subsequent bytes
	mov	ecx, 66			;  max match of 66
	repe	cmpsb				;  repeat while =, compare [ds:si] to [es:di]
	cmp	esi, edx			;  is new size longer than old size
	jb		NotGreater		;
	mov	edx, esi			;  put var to calc run size in dx
	mov	MatchStart, ebx	;  store var to calc start of match

NotGreater:
	mov	edi, eax			;  recover di
	mov	ecx, ebx			;  recover cx
	jcxz	FindMatchDone	;  if cx is zero then we're done
	jmp	FindMatchLoop	;  if cx is not zero then continue checking

FindMatchDone:
	mov	esi, pData; 	;  restore pointer to current position
	sub	edx, esi			;  difference is length of match +1
	cmp	edx, 4			;  threshold of efficiency 3 +1
	jb		OutVerbatim		;  run is too short
	dec	edx				;  remove the +1

; reduce run to remaining length
;	mov	cx, word ptr mylength[2];  load high word of length
;	or		cx, cx			;  test it
;	jnz	Plenty			;  length >= 64k
;	mov	ax, word ptr mylength;  load low word of length
;	cmp	ax, dx			;  test it
;	ja		Plenty			;  length is greater then run
;	mov	edx, eax			;  use up whats left <--BUG! [d3-07-97 JPC]

; [d3-07-97 JPC] Fix bug: we were moving eax into edx without having
; cleared the upper 16 bits of eax, causing edx to be far too big.
; All we really need to do is check whether edx is more than the
; remaining bytes in the source.
   cmp   edx, mylength
   jb    Plenty
   mov   edx, mylength

Plenty:
; convert the match run found into a token
	mov	ecx, edx			;  move copy of run length into cx
	sub	ecx, 3			;  len of zero is actually 3
	shl	ecx, 1			;  put match length into top 6 bits
	shl	ecx, 1			;
	mov	eax, MatchStart;
	inc	eax				;  actually one more than saved value
	or		ah, cl			;  length into most significant 6 bits of ax

StoreToken:
; put token (ax) into buffer[index]
	mov	ebx, offset buffer[0]		;  load effective address of buffer into bx
	mov	ecx, index		;  load the index
	add	ebx, ecx			;  add index to effective address
	mov	word ptr [ebx], ax	;  store in buffer[index]
	add	ecx, 2				;  add two to index
	mov	index, ecx		;  save the index
	shl	word ptr bitflags, 1;  shift the bitflag count, 0=token
	dec	word ptr bfcount;  dec the bitflag count
	jz		OutputBuffer	;  buffer is full so store in output data area
	jmp	AdjustPointers	;


OutVerbatim:
; put byte (al) into buffer[index]
	mov	al, [esi]		;  load the current byte
	mov	ebx, offset buffer[0]		;  load effective address of buffer into bx
	add	ebx, index		;  add index to effective address
	mov	byte ptr [ebx], al	;  store in buffer[index]
	inc	index	;  move index forward one
	mov	edx, 1			;  set fake run counter to one
	stc						;  set carry
	rcl	word ptr bitflags, 1;  roll a 1 into bitflags
	dec	word ptr bfcount;  dec the bitflag count
	jnz	AdjustPointers	;  buffer not full so skip

OutputBuffer:
; don't touch dx in this routine
;	mov	esi, 0b8000000h				;  do spinner
;	mov	ebx, offset spin			;
;	mov	cx, idots		;
;	add	ebx, ecx			;
;	mov	al, byte ptr [ebx];
;	mov	byte ptr [esi], al;
;	inc	cx					;
;	and	cx, 3				;
;	mov	word ptr idots, cx;

	mov	esi, offset buffer	;  load effective address of buffer into si
	mov	edi, pOutput	;  set pOutput as destination
	mov	ax, bitflags	;  load the bitflags
	stosw						;  store the bitflags
	mov	ecx, index		;  move index into rep count
	rep	movsb				;  copy buffer to output

	mov	pOutput, edi	;  save the new output pointer

	mov	dword ptr index, 0		;  reset to 0
	mov	word ptr bitflags, 0		;  reset to 0
	mov	word ptr bfcount, 16		;  reset to 16


AdjustPointers:
; move si by the size of the run, adjust di when si moves beyond 1024
	mov	esi, pData		;  restore pointer to current position
	mov	edi, pDest		;  restore back-address pointer
	add	esi, edx			;  add run size to si
	cmp	esi, 1024		;  is si greater than 1024?
	jb		NoChangeDI		;  if less than 1024 then don't change di
	mov	eax, esi			;
	sub	eax, 1024		;  check if difference between si and di..
	cmp	eax, edi			;  ..is greater than 1024
	jb		NoChangeDI		;  if less than 1024 then don't change di
	mov	edi, eax			;  otherwise set di to si-1024

NoChangeDI:
	mov	pData, esi
	mov	pDest, edi

	sub	dword ptr mylength, edx;  subtract run from length
	jc		EncodeDone		;
	mov	eax, dword ptr mylength;	 test for zero length, this indicates done
	jz		NearlyDone		;
	jmp	EncodeLoop		;

NearlyDone:
	xor	eax, eax			;  make zero token
	mov	edx, 1			;  sutract one more

	mov	cx, word ptr bfcount;
	dec	cx					;
	shl	word ptr bitflags, cl;  shift the bitflag word
	mov	word ptr bfcount, 1;
	jmp	StoreToken		;

EncodeDone:

	mov	eax, pOutput
	ret

LZSSEncode_	ENDP

END

;; ננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננ
