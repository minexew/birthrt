;; ננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננ
;;
;;    File:    lzssdcod.asm - LZSS de-compressing routine
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
;.DATA

;	pSource	dd ?
;	pDest		dd ?

.CODE
EVEN

LZSSDecode_	PROC	C USES  ECX EBX EDX ESI EDI, pSource:PTR BYTE, pDest:PTR BYTE, pEnd:PTR BYTE
	PUBLIC LZSSDecode_

	mov	esi, pSource	;  get source pointer
	mov	edi, pDest		;  get dest pointer

	cld						;  direction is forward

	xor	eax, eax			;  clear some registers
	xor	ebx, ebx
	xor	ecx, ecx

LZSSloop:
	lodsw						;  load the bit-flags
	mov	bx, ax			;  move bit-flags to bx
	stc						;  set the carry flag
	rcl	bx, 1				;  roll carry into lsb and test bit into carry
	jnc	DoRun				;  bit clear means a run

MoveUnique:
	movsb						;  bit set means unique byte copied verbatim
	shl	bx, 1				;  move next bit-flag into position
	jz		LZSSloop			;  bit-flags used up, get next bit-flag word
	jc		MoveUnique		;  bit set means unique byte copied verbatim

; bit clear means a run
DoRun:
	lodsw						;  get run token
	cmp	ax, 0				;  end token?
	jz		LZSSDone			;  if zero the all done

	mov	edx, esi			;  save the source pointer in edx

	mov	cl, ah			;  calc run length, ch has already been cleared
	shr	cx, 1				;  run is in top 6 bits
	shr	cx, 1				;
	and	cx, 3fh			;  6 bits of run length
	add	cx, 3				;  threshold of efficiency is run of 3

	and	ax, 03ffh		;  10 bits of back-address
	inc	ax					;  back-address goes back -2 to -1024
	mov	esi, edi			;  move dest index into source
	sub	esi, eax			;  si will always be larger than back-address

	shr	cx, 1				;  move lsb into carry
	rep	movsw				;  copy most of the run
	rcl	cx, 1				;  recover the lsb
	rep	movsb				;  possably move the odd byte

; bit is set so cleanup after the run
	mov	esi, edx			;  recover the source address from edx

NextBitFlag:
        cmp     edi,pEnd               ; [d3-06-97 JPC] prevent overflow
        ja     LZSSDone                ; [d3-06-97 JPC] prevent overflow
	shl	bx, 1				;  move next bit-flag into position
	jnc	DoRun				;  bit clear means a run
	jz		LZSSloop			;  bit-flags used up, get next bit-flag word
	jc		MoveUnique		;  bit was set so go move a unique byte

LZSSDone:
        mov eax, edi                   ; [d3-06-97 JPC]
	ret

LZSSDecode_	ENDP

END

;; ננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננננ
