	.386
ifdef VC
	.MODEL	FLAT,C
else
	.MODEL	FLAT,SYSCALL
endif

	.DATA

lastCompValL	dw	0
lastCompValR	dw	0
hundred dw 100

delta8_2	label	byte
	db	0,1,2,3,6,10,15,21

delta16_4	label	word
	dw	0,4,16,128,256,512,1024,2048

delta16_3	label	word
	dw	0,1,2,8,32,64,96,128,192,256,384,512,768,1024,2048,4096

delta16_2	label	word
	dw	0
	dw	8
	dw	16
	dw	32,48
	dw	64,80,96,112
	dw	128,144,160,176,192,208,224,240
	dw	256,272,288,304,320,336,352,368,384,400,416,432,448,464,480,496
	dw	512,520,528,536,544,552,560,568,576,584,592,600,608,616,624,632
	dw	    640,648,656,664,672,680,688,696,704,712,720,728,736,744,752,760
	dw	    768,776,784,792,800,808,816,824,832,840,848,856,864,872,880,888
	dw	    896,904,912,920,928,936,944,952,960,968,976,984,992,1000,1008,1016
	dw	1024,1088,1152,1216,1280,1344,1408,1472,1536,1600,1664,1728,1792,1856,1920,1984
	dw	2048,2304,2560,2816,3072,3328,3584,3840
	dw	4096,5120,6144,7168
	dw	8192,12288
	dw	16384


	.CODE
; eax, edx do not have to be saved

Merge proc public uses es edi esi ecx,\
	outb:near ptr, inb:near ptr, count:dword, bits:byte, check:byte

	push	ds
	pop	es
	mov	edi, outb
	mov	esi, inb
	mov	ecx, count
	cmp	ecx, 0
	jle	mergeOut
	cmp	bits, 8
	jle	merge8
	shr	ecx, 1
	cmp	check, 0
	je	merge16nc
	xor	edx, edx
	xor	eax, eax
merge16c:
	mov	dx, es:[edi]
	lodsw
	sar	ax, 1
	sar	dx, 1
	add	ax, dx
	stosw
	loop	merge16c
	ret
merge16nc:
	lodsw
	add	ax, es:[edi]
	jno	merge16ok
	js	merge16p
	mov	ax, 8000h
	jmp	short merge16ok
merge16p:
	mov	ax, 7fffh
merge16ok:
	stosw
	loop	merge16nc
	ret
merge8:
	cmp	check, 0
	je	merge8nc
merge8c:
	mov	dx, 8080h
	lodsb
	sub	dl, al
	sub	dh, es:[edi]
	add	dl, dh
	sar	dl, 1
	mov	al, 80h
	sub	al, dl
	stosb
	loop	merge8c
	ret
merge8nc:
	mov	dx, 8080h
	lodsb
	sub	dl, al
	sub	dh, es:[edi]
	add	dl, dh
	jno	merge8ok
	js	merge8p
	mov	dl, 81h
	jmp	short merge8ok
merge8p:
	mov	dl, 80h
merge8ok:
	mov	al, 80h
	sub	al, dl
	stosb
	loop	merge8nc
mergeOut:
	ret

Merge endp

Distort proc public uses es edi ecx,\
	outb:near ptr, count:dword, bits:byte, distort:byte

	push	ds
	pop	es
	mov	edi, outb
	mov	ecx, count
	cmp	ecx, 0
	jle	distortOut
	mov	dl, distort
	cmp	bits, 8
	jle	distort8
	shr	ecx, 1
	shl	dx, 8
distort16:
	mov	ax, es:[edi]
	and	ax, dx
	stosw
	loop	distort16
	ret
distort8:
	mov	al, es:[edi]
	and	al, dl
	stosb
	loop	distort8
distortOut:
	ret

Distort endp

Volume proc public uses es edi ecx,\
	outb:near ptr, count:dword, bits:byte, vol:byte, filter:byte 

	push	ds
	pop	es
	mov	edi, outb
	mov	ecx, count
	cmp	bits, 8
	jle	vol8
	shr	ecx, 1
	cmp	vol, 0
	je	vol16_1
	cmp	vol, 1
	je	vol16_2
	cmp	vol, 2
	je	vol16_3
	cmp	vol, 3
	je	vol16_4
	cmp	vol, 4
	je	vol16_5
	cmp	vol, 5
	je	vol16_6
	cmp	vol, 6
	je	vol16_7
	cmp	vol, 7
	je	vol16_8
	cmp	vol, 8
	je	vol16_9
	cmp	vol, 9
	je	vol16_10
	cmp	vol, 10
	je	vol16_11
	cmp	vol, 11
	je	vol16_12
	cmp	vol, 12
	je	vol16_13
	cmp	vol, 13
	je	vol16_14
	cmp	vol, 14
	je	vol16_15
	cmp	vol, 15
	je	vol16_16
	cmp	vol, 16
	je	vol16_17
	cmp	vol, 17
	je	vol16_18
	cmp	vol, 18
	je	vol16_19
	cmp	vol, 19
	je	vol16_20
	cmp	vol, 20
	je	vol16_21
	cmp	vol, 21
	je	vol16_22
	cmp	vol, 22
	je	vol16_23
	cmp	vol, 23
	je	vol16_24
	cmp	vol, 24
	je	vol16_25
	cmp	vol, 25
	je	vol16_26
	cmp	vol, 26
	je	vol16_27
	cmp	vol, 27
	je	vol16_28
	cmp	vol, 28
	je	vol16_29
	cmp	vol, 29
	je	vol16_30
	cmp	vol, 30
	jne	vol16_31
	ret
vol16_31: ;31x/32 = (1x+2x+4x+8x+16x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_31
	ret
vol16_30: ;30x/32 = (1x+2x+4x+8x)/16
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 4
	stosw
	loop	vol16_30
	ret
vol16_29: ;29x/32 = (1x+4x+8x+16x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 2
	add	edx, eax 
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_29
	ret
vol16_28: ;28x/32 = (1x+2x+4x)/8
	movsx	eax, word ptr es:[edi]
	mov	edx, eax 
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 3
	stosw
	loop	vol16_28
	ret
vol16_27: ;27x/32 = (1x+2x+8x+16x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 1
	add	edx, eax 
	shl	eax, 2
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_27
	ret
vol16_26: ;26x/32 = (1x+4x+8x)/16
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 2
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 4
	stosw
	loop	vol16_26
	ret
vol16_25: ;25x/32 = (1x+8x+16x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 3
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_25
	ret
vol16_24: ;24x/32 = (1x+2x)/4
	movsx	eax, word ptr es:[edi]
	mov	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 2
	stosw
	loop	vol16_24
	ret
vol16_23: ;23x/32 = (1x+2x+4x+16x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	edx, eax 
	shl	eax, 2
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_23
	ret
vol16_22: ;22x/32 = (1x+2x+8x)/16
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 1
	add	edx, eax 
	shl	eax, 2
	add	eax, edx 
	shr	eax, 4
	stosw
	loop	vol16_22
	ret
vol16_21: ;21x/32 = (1x+4x+16x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 2
	add	edx, eax 
	shl	eax, 2
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_21
	ret
vol16_20: ;20x/32 = (1x+4x)/8
	movsx	eax, word ptr es:[edi]
	mov	edx, eax 
	shl	eax, 2
	add	eax, edx 
	shr	eax, 3
	stosw
	loop	vol16_20
	ret
vol16_19: ;19x/32 = (1x+2x+16x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 1
	add	edx, eax 
	shl	eax, 3
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_19
	ret
vol16_18: ;18x/32 = (1x+8x)/16
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 3
	add	eax, edx 
	shr	eax, 4
	stosw
	loop	vol16_18
	ret
vol16_17: ;17x/32 = (1x+16x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 4
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_17
	ret
vol16_16: ;16x/32 = (1x)/2
	sar	word ptr es:[edi], 1
	add	edi, 2
	loop	vol16_16
	ret
vol16_15: ;15x/32 = (1x+2x+4x+8x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_15
	ret
vol16_14: ;14x/32 = (1x+2x+4x)/16
	movsx	eax, word ptr es:[edi]
	mov	edx, eax 
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 4
	stosw
	loop	vol16_14
	ret
vol16_13: ;13x/32 = (1x+4x+8x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 2
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_13
	ret
vol16_12: ;12x/32 = (1x+2x)/8
	movsx	eax, word ptr es:[edi]
	mov	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 3
	stosw
	loop	vol16_12
	ret
vol16_11: ;11x/32 = (1x+2x+8x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 1
	add	edx, eax 
	shl	eax, 2
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_11
	ret
vol16_10: ;10x/32 = (1x+4x)/16
	movsx	eax, word ptr es:[edi]
	mov	edx, eax 
	shl	eax, 2
	add	eax, edx 
	shr	eax, 4
	stosw
	loop	vol16_10
	ret
vol16_9: ;9x/32 = (1x+8x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 3
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_9 
	ret
vol16_8: ;8x/32 = (1x)/4
	sar	word ptr es:[edi], 2
	add	edi, 2
	loop	vol16_8
	ret
vol16_7: ;7x/32 = (1x+2x+4x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_7 
	ret
vol16_6: ;6x/32 = (1x+2x)/16
	movsx	eax, word ptr es:[edi]
	mov	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shr	eax, 4
	stosw
	loop	vol16_6
	ret
vol16_5: ;5x/32 = (1x+4x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 2
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_5 
	ret
vol16_4: ;4x/32 = (1x)/8
	sar	word ptr es:[edi], 3
	add	edi, 2
	loop	vol16_4
	ret
vol16_3: ;3x/32 = (1x+2x)/32
	movsx	eax, word ptr es:[edi]
	mov	edx, eax
	shl	eax, 1
	add	eax, edx 
	shr	eax, 5
	stosw
	loop	vol16_3 
	ret
vol16_2: ;2x/32 = (1x)/16
	sar	word ptr es:[edi], 4
	add	edi, 2
	loop	vol16_2
	ret
vol16_1: ;1x/32 = (1x)/32
	sar	word ptr es:[edi], 5
	add	edi, 2
	loop	vol16_1
	ret
vol8:
	cmp	vol, 0
	je	vol8_1
	cmp	vol, 1
	je	vol8_2
	cmp	vol, 2
	je	vol8_3
	cmp	vol, 3
	je	vol8_4
	cmp	vol, 4
	je	vol8_5
	cmp	vol, 5
	je	vol8_6
	cmp	vol, 6
	je	vol8_7
	cmp	vol, 7
	je	vol8_8
	cmp	vol, 8
	je	vol8_9
	cmp	vol, 9
	je	vol8_10
	cmp	vol, 10
	je	vol8_11
	cmp	vol, 11
	je	vol8_12
	cmp	vol, 12
	je	vol8_13
	cmp	vol, 13
	je	vol8_14
	cmp	vol, 14
	je	vol8_15
	cmp	vol, 15
	je	vol8_16
	cmp	vol, 16
	je	vol8_17
	cmp	vol, 17
	je	vol8_18
	cmp	vol, 18
	je	vol8_19
	cmp	vol, 19
	je	vol8_20
	cmp	vol, 20
	je	vol8_21
	cmp	vol, 21
	je	vol8_22
	cmp	vol, 22
	je	vol8_23
	cmp	vol, 23
	je	vol8_24
	cmp	vol, 24
	je	vol8_25
	cmp	vol, 25
	je	vol8_26
	cmp	vol, 26
	je	vol8_27
	cmp	vol, 27
	je	vol8_28
	cmp	vol, 28
	je	vol8_29
	cmp	vol, 29
	je	vol8_30
	cmp	vol, 30
	je	vol8_31
	ret
vol8_31: ;31x/32 = (1x+2x+4x+8x+16x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 1
	add	dx, ax 
	shl	ax, 1
	add	dx, ax 
	shl	ax, 1
	add	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_31
	ret
vol8_30: ;30x/32 = (1x+2x+4x+8x)/16
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 1
	add	dx, ax 
	shl	ax, 1
	add	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 4
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_30
	ret
vol8_29: ;29x/32 = (1x+4x+8x+16x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 2
	add	dx, ax 
	shl	ax, 1
	add	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_29
	ret
vol8_28: ;28x/32 = (1x+2x+4x)/8
	mov	al, filter
	sub	al, es:[edi]
	movsx	eax, al
	mov	edx, eax 
	shl	eax, 1
	add	edx, eax 
	shl	eax, 1
	add	eax, edx 
	shl	ax, 5
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_28
	ret
vol8_27: ;27x/32 = (1x+2x+8x+16x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 1
	add	dx, ax 
	shl	ax, 2
	add	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_27
	ret
vol8_26: ;26x/32 = (1x+4x+8x)/16
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 2
	add	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 4
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_26
	ret
vol8_25: ;25x/32 = (1x+8x+16x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 3
	add	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_25
	ret
vol8_24: ;24x/32 = (1x+2x)/4
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 6
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_24
	ret
vol8_23: ;23x/32 = (1x+2x+4x+16x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 1
	add	dx, ax 
	shl	ax, 1
	add	dx, ax 
	shl	ax, 2
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_23
	ret
vol8_22: ;22x/32 = (1x+2x+8x)/16
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 1
	add	dx, ax 
	shl	ax, 2
	add	ax, dx 
	shl	ax, 4
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_22
	ret
vol8_21: ;21x/32 = (1x+4x+16x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 2
	add	dx, ax 
	shl	ax, 2
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_21
	ret
vol8_20: ;20x/32 = (1x+4x)/8
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax 
	shl	ax, 2
	add	ax, dx 
	shl	ax, 5
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_20
	ret
vol8_19: ;19x/32 = (1x+2x+16x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 1
	add	dx, ax 
	shl	ax, 3
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_19
	ret
vol8_18: ;18x/32 = (1x+8x)/16
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 3
	add	ax, dx 
	shl	ax, 4
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_18
	ret
vol8_17: ;17x/32 = (1x+16x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 4
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_17
	ret
vol8_16: ;16x/32 = (1x)/2
	mov	al, filter
	mov	ah, al
	sub	ah, es:[edi]
	sar	ah, 1
	sub	al, ah
	stosb
	loop	vol8_16
	ret
vol8_15: ;15x/32 = (1x+2x+4x+8x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 1
	add	dx, ax 
	shl	ax, 1
	add	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_15
	ret
vol8_14: ;14x/32 = (1x+2x+4x)/16
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax 
	shl	ax, 1
	add	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 4
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_14
	ret
vol8_13: ;13x/32 = (1x+4x+8x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 2
	add	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_13
	ret
vol8_12: ;12x/32 = (1x+2x)/8
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 5
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_12
	ret
vol8_11: ;11x/32 = (1x+2x+8x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 1
	add	dx, ax 
	shl	ax, 2
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_11
	ret
vol8_10: ;10x/32 = (1x+4x)/16
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax 
	shl	ax, 2
	add	ax, dx 
	shl	ax, 4
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_10
	ret
vol8_9: ;9x/32 = (1x+8x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 3
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_9 
	ret
vol8_8: ;8x/32 = (1x)/4
	mov	al, filter
	mov	ah, al
	sub	ah, es:[edi]
	sar	ah, 2
	sub	al, ah
	stosb
	loop	vol8_8
	ret
vol8_7: ;7x/32 = (1x+2x+4x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 1
	add	ax, dx 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_7 
	ret
vol8_6: ;6x/32 = (1x+2x)/16
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax 
	shl	ax, 1
	add	ax, dx 
	shl	ax, 4
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_6
	ret
vol8_5: ;5x/32 = (1x+4x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 2
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_5 
	ret
vol8_4: ;4x/32 = (1x)/8
	mov	al, filter
	mov	ah, al
	sub	ah, es:[edi]
	sar	ah, 3
	sub	al, ah
	stosb
	loop	vol8_4
	ret
vol8_3: ;3x/32 = (1x+2x)/32
	mov	al, filter
	sub	al, es:[edi]
	movsx	ax, al
	mov	dx, ax
	shl	ax, 1
	add	ax, dx 
	shl	ax, 3
	mov	al, filter
	sub	al, ah
	stosb
	loop	vol8_3 
	ret
vol8_2: ;2x/32 = (1x)/16
	mov	al, filter
	mov	ah, al
	sub	ah, es:[edi]
	sar	ah, 4
	sub	al, ah
	stosb
	loop	vol8_2
	ret
vol8_1: ;1x/32 = (1x)/32
	mov	al, filter
	mov	ah, al
	sub	ah, es:[edi]
	sar	ah, 5
	sub	al, ah
	stosb
	loop	vol8_1
	ret

Volume endp

Silence proc public uses es edi ecx,\
	outb:near ptr, count:dword, bits:byte

	push	ds
	pop	es
	mov	edi, outb
	mov	ecx, count
	cmp	bits, 8
	jle	silence8
	shr	ecx, 1
	mov	ax, 0
	rep	stosw
	ret
silence8:
	mov	ax, 80h
	rep	stosb
	ret

Silence endp

Decomp proc public uses es edi esi ebx ecx\
	buff:near ptr, count:dword, factor:dword, bits:dword, channels:dword,\
	compValL:dword, compValR:dword

	push	ds
	pop	es
	mov	ecx, count
	cmp	ecx, 0
	jle	decompOut
	mov	esi, buff
	mov	edi, esi
	mov	eax, factor
	mul	ecx
	add	esi, eax
	mov	edx, compValL
	xor	ebx, ebx
	cmp	bits, 16
	je	decomp16
	mov	dh, dl
	cmp	channels, 1
	je	decomp8M
	shr	ecx, 1
	rol	edx, 16
	add	edx, compValR
	mov	dh, dl
decomp8L:
	rol	edx, 16
	lodsw
	mov	dl, al
	shr	al, 4
	and	al, 0fh
	cmp	al, 8
	jl	d8L1
	and	al, 7
	mov	bl, al
	mov	al, delta8_2[ebx]
	sub	dh, al
	jmp	d8L2
d8L1:
	mov	bl, al
	mov	al, delta8_2[ebx]
	add	dh, al
d8L2:
	mov	al, dh
	stosb
	mov	al, ah
	rol	edx, 16
	mov	dl, al
	shr	al, 4
	and	al, 0fh
	cmp	al, 8
	jl	d8R1
	and	al, 7
	mov	bl, al
	mov	al, delta8_2[ebx]
	sub	dh, al
	jmp	d8R2
decomp8L_jmp:
	jmp	decomp8L
d8R1:
	mov	bl, al
	mov	al, delta8_2[ebx]
	add	dh, al
d8R2:
	mov	al, dh
	stosb
	rol	edx, 16
	mov	al, dl
	and	al, 0fh
	cmp	al, 8
	jl	d8L3
	and	al, 7
	mov	bl, al
	mov	al, delta8_2[ebx]
	sub	dh, al
	jmp	d8L4
d8L3:
	mov	bl, al
	mov	al, delta8_2[ebx]
	add	dh, al
d8L4:
	mov	al, dh
	stosb
	rol	edx, 16
	mov	al, dl
	and	al, 0fh
	cmp	al, 8
	jl	d8R3
	and	al, 7
	mov	bl, al
	mov	al, delta8_2[ebx]
	sub	dh, al
	jmp	d8R4
d8R3:
	mov	bl, al
	mov	al, delta8_2[ebx]
	add	dh, al
d8R4:
	mov	al, dh
	stosb
	loop	decomp8L_jmp
	xor	ah, ah
	mov	lastCompValR, ax
	rol	edx, 16
	mov	al, dh
	mov	lastCompValL, ax
decompOut:
	ret
decomp8M:
	lodsb
	mov	dl, al
	shr	al, 4
	and	al, 0fh
	cmp	al, 8
	jl	d8M1
	and	al, 7
	mov	bl, al
	mov	al, delta8_2[ebx]
	sub	dh, al
	jmp	d8M2
d8M1:
	mov	bl, al
	mov	al, delta8_2[ebx]
	add	dh, al
d8M2:
	mov	al, dh
	stosb

	mov	al, dl
	and	al, 0fh
	cmp	al, 8
	jl	d8M3
	and	al, 7
	mov	bl, al
	mov	al, delta8_2[ebx]
	sub	dh, al
	jmp	d8M4
d8M3:
	mov	bl, al
	mov	al, delta8_2[ebx]
	add	dh, al
d8M4:
	mov	al, dh
	stosb
	loop	decomp8M
	xor	ah, ah
	mov	lastCompValL, ax
	ret

decomp16:
	cmp	factor, 1
	je	decomp16_2
	cmp	factor, 3
	je	decomp16_4
	cmp	channels, 1
	je	decomp16_3M
	shr	ecx, 1
decomp16_3L:
	loop	decomp16_3L
decomp16_3R:
	loop	decomp16_3R
	ret
decomp16_3M:
	loop	decomp16_3M
	ret
decomp16_2:
	cmp	channels, 1
	je	decomp16_2M
	shr	ecx, 1
	rol	edx, 16
	add	edx, compValR
decomp16_2L:
	rol	edx, 16
	lodsw
	mov	bl, al
	shl	bl, 1
	jc	d16_2L1
	add	dx, delta16_2[ebx]
	jmp	d16_2L2
d16_2L1:
	sub	dx, delta16_2[ebx]
d16_2L2:
	mov	bl, ah
	mov	ax, dx
	stosw
	rol	edx, 16
	shl	bl, 1
	jc	d16_2R1
	add	dx, delta16_2[ebx]
	jmp	d16_2R2
d16_2R1:
	sub	dx, delta16_2[ebx]
d16_2R2:
	mov	ax, dx
	stosw
	loop	decomp16_2L
	mov	lastCompValR, dx
	rol	edx, 16
	mov	lastCompValL, dx
	ret
decomp16_2M:
	lodsb
	mov	bl, al
	shl	bl, 1
	jc	d16_2M1
	add	dx, delta16_2[ebx]
	jmp	d16_2M2
d16_2M1:
	sub	dx, delta16_2[ebx]
d16_2M2:
	mov	ax, dx
	stosw
	loop	decomp16_2M
	mov	lastCompValL, dx
	ret
decomp16_4:
	cmp	channels, 1
	je	decomp16_4M
	shr	ecx, 1
	rol	edx, 16
	add	edx, compValR
decomp16_4L:
	rol	edx, 16
	lodsw
	push	eax
	mov	bl, al
	shr	bl, 4
	and	bl, 0fh
	shl	bl, 1
	cmp	bl, 16
	jl	d16_4L1
	and	bl, 0eh
	sub	dx, delta16_4[ebx]
	jmp	d16_4L2
d16_4L1:
	add	dx, delta16_4[ebx]
d16_4L2:
	mov	bl, ah
	mov	ax, dx
	stosw
	rol	edx, 16
	shr	bl, 4
	and	bl, 0fh
	shl	bl, 1
	cmp	bl, 16
	jl	d16_4R1
	and	bl, 0eh
	sub	dx, delta16_4[ebx]
	jmp	d16_4R2
decomp16_4L_jmp:
	jmp	decomp16_4L
d16_4R1:
	add	dx, delta16_4[ebx]
d16_4R2:
	mov	ax, dx
	stosw
	rol	edx, 16
	pop	eax
	mov	bl, al
	and	bl, 0fh
	shl	bl, 1
	cmp	bl, 16
	jl	d16_4L3
	and	bl, 0eh
	sub	dx, delta16_4[ebx]
	jmp	d16_4L4
d16_4L3:
	add	dx, delta16_4[ebx]
d16_4L4:
	mov	bl, ah
	mov	ax, dx
	stosw
	rol	edx, 16
	and	bl, 0fh
	shl	bl, 1
	cmp	bl, 16
	jl	d16_4R3
	and	bl, 0eh
	sub	dx, delta16_4[ebx]
	jmp	d16_4R4
d16_4R3:
	add	dx, delta16_4[ebx]
d16_4R4:
	mov	ax, dx
	stosw
	loop	decomp16_4L_jmp
	mov	lastCompValR, dx
	rol	edx, 16
	mov	lastCompValL, dx
	ret
decomp16_4M:
	lodsb
	push	eax
	shr	al, 4
	and	al, 0fh
	mov	bl, al
	shl	bl, 1
	cmp	bl, 16
	jl	d16_4M1
	and	bl, 0eh
	sub	dx, delta16_4[ebx]
	jmp	d16_4M2
d16_4M1:
	add	dx, delta16_4[ebx]
d16_4M2:
	mov	ax, dx
	stosw
	pop	eax
	and	al, 0fh
	mov	bl, al
	shl	bl, 1
	cmp	bl, 16
	jl	d16_4M3
	and	bl, 0eh
	sub	dx, delta16_4[ebx]
	jmp	d16_4M4
d16_4M3:
	add	dx, delta16_4[ebx]
d16_4M4:
	mov	ax, dx
	stosw
	loop	decomp16_4M
	mov	lastCompValL, dx
	ret

Decomp endp

DecompGet proc public channel:dword

	cmp	channel, 0
	jne	dg1
	mov	ax, lastCompValL
	ret
dg1:
	mov	ax, lastCompValR
	ret

DecompGet endp

ConvBits proc public uses es edi esi ecx,\
	buff:near ptr, count:dword, factor:dword

	mov	ecx, count
	cmp	ecx, 0
	jle	convBitsX
	push	ds
	pop	es
	mov	esi, buff
	mov	edi, esi
	cmp	factor, 0
	jl	convBitsDown
	add	esi, ecx
	sub	esi, 1
	add	edi, ecx
	add	edi, ecx
	sub	edi, 2
	pushf
	std
convBitsUp:
	lodsb
	mov	ah, 7fh
	sub	ah, al
	mov	al, 0
	sbb 	al, 0
	stosw
	loop	convBitsUp
	popf
	ret
convBitsDown:
	lodsw
	mov	al, 80h
	sub	al, ah
	stosb
	loop	convBitsDown
convBitsX:
	ret

ConvBits endp

Pan proc public uses es edi esi ebx ecx,\
	buff:near ptr, count:dword, pan:dword

	mov	ecx, count
	cmp	ecx, 0
	jle	panX
	push	ds
	pop	es
	mov	esi, buff
	mov	edi, esi
	mov	ebx, pan
	mov	dx, 100
	sub	dx, bx
	rol	ebx, 16
	mov	bx, dx
	shr	ecx, 1
pan16:
	lodsw
	rol	ebx, 16
	imul	bx
	idiv	hundred
	stosw
	lodsw
	rol	ebx, 16
	imul	bx
	idiv	hundred
	stosw
	loop	pan16
panX:
	ret

Pan endp

ConvChannels proc public uses es edi esi ecx,\
	buff:near ptr, count:dword, bits:dword, factor:dword

	mov	ecx, count
	cmp	ecx, 0
	jle	convChanX
	push	ds
	pop	es
	mov	esi, buff
	mov	edi, esi
	cmp	factor, 0
	jl	convChanDown
;convChanUp:
	pushf
	std
	add	esi, ecx
	mov	eax, ecx
	mul	factor
	add	edi, eax
	sub	esi, 1
	sub	edi, 1
	cmp	bits, 8
	je	convChanUp_8
	sub	esi, 1
	sub	edi, 1
	shr	ecx, 1
convChanUp_16:
	lodsw
	stosw
	stosw
	loop	convChanUp_16
	popf
convChanX:
	ret
convChanUp_8:
	lodsb
	stosb
	stosb
	loop	convChanUp_8
	popf
	ret
convChanDown:
	cmp	bits, 8
	je	convChanDown_8
	shr	ecx, 1
convChanDown_16:
	lodsw
	add	ax, es:[esi]
	jno	mergeok
	js	mergep
	mov	ax, 8000h
	add	esi, 2
	jmp	short convChanDown_16
mergep:
	mov	ax, 7fffh
mergeok:
	stosw
	add	esi, 2
	loop    convChanDown_16
	ret
convChanDown_8:
	mov	dx, 8080h
	lodsb
	sub	dl, al
	lodsb
	sub	dh, al
	add	dl, dh
	sar	dl, 1
	mov	al, 80h
	sub	al, dl
	stosb
	loop	convChanDown_8
	ret

ConvChannels endp

ConvRate proc public uses es edi esi ecx,\
	buff:near ptr, count:dword, bits:dword, chans:dword, factor:dword

	mov	ecx, count
	cmp	ecx, 0
	jle	convRateX
	push	ds
	pop	es
	mov	esi, buff
	mov	edi, esi
	cmp	factor, 0
	jl	convRateDown
;convRateUp:
	pushf
	std
	add	esi, ecx
	mov	eax, ecx
	mul	factor
	add	edi, eax
	cmp	bits, 8
	je	convRateUp_8
;convRateUp_16:
	cmp	chans, 2
	jne	convRateUp_16m
	shr	ecx, 2
	sub	esi, 4
	sub	edi, 4
;convRateUp_16s:
	cmp	factor, 2
	je	convRateUp2_16s
	cmp	factor, 4
	je	convRateUp4_16s
; 5.5K -> 44K @ 16bit stereo
convRateUp8_16s:
	lodsd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	loop	convRateUp8_16s
	popf
convRateX:
	ret
; 5.5K -> 22K or 11K -> 44K @ 16bit stereo
convRateUp4_16s:
	lodsd
	stosd
	stosd
	stosd
	stosd
	loop	convRateUp4_16s
	popf
	ret
; 5.5K -> 11K or 11K -> 22K or 22K -> 44K @ 16bit stereo
convRateUp2_16s:
	lodsd
	stosd
	stosd
	loop	convRateUp2_16s
	popf
	ret
convRateUp_8:
	cmp	chans, 2
	jne	convRateUp_8m
; @ 16bit mono or 8bit stereo
;convRateUp_8s:
convRateUp_16m:
	shr	ecx, 1
	sub	esi, 2
	sub	edi, 2
	cmp	factor, 2
	je	convRateUp2_16m
	cmp	factor, 4
	je	convRateUp4_16m
; 5.5K -> 44K @ 16bit mono or 8bit stereo
;convRateUp8_8s:
convRateUp8_16m:
	lodsw
	stosw
	stosw
	stosw
	stosw
	stosw
	stosw
	stosw
	stosw
	loop	convRateUp8_16m
	popf
	ret
; 5.5K -> 22K or 11K -> 44K @ 16bit stereo or 8bit stereo
;convRateUp4_8s:
convRateUp4_16m:
	lodsw
	stosw
	stosw
	stosw
	stosw
	loop	convRateUp4_16m
	popf
	ret
; 5.5K -> 11K or 11K -> 22K or 22K -> 44K @ 16bit stereo or 8bit stereo
convRateUp2_8s:
convRateUp2_16m:
	lodsw
	stosw
	stosw
	loop	convRateUp2_16m
	popf
	ret
; @ 8bit mono
convRateUp_8m:
	sub	esi, 1
	sub	edi, 1
	cmp	factor, 2
	je	convRateUp2_8m
	cmp	factor, 4
	je	convRateUp4_8m
; 5.5K -> 44K @ 8bit mono
convRateUp8_8m:
	lodsb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	loop	convRateUp8_8m
	popf
	ret
; 5.5K -> 22K or 11K -> 44K @ 8bit mono
convRateUp4_8m:
	lodsb
	stosb
	stosb
	stosb
	stosb
	loop	convRateUp4_8m
	popf
	ret
; 5.5K -> 11K or 11K -> 22K or 22K -> 44K @ 8bit mono
convRateUp2_8m:
	lodsb
	stosb
	stosb
	loop	convRateUp2_8m
	popf
	ret
convRateDown:
	neg	factor
	cmp	bits, 8
	je	convRateDown_8
;convRateDown16
	cmp	chans, 2
	jne	convRateDown_16m
	mov	edx, 4
	shr	ecx, 2
	cmp	factor, 2
	je	convRateDown_16s
	add	edx, 8
	cmp	factor, 4
	je	convRateDown_16s
	add	edx, 16
convRateDown_16s:
	lodsd
	stosd
	add	esi, edx
	loop	convRateDown_16s
	ret
convRateDown_8:
	cmp	chans, 2
	jne	convRateDown_8m
convRateDown_16m:
	mov	edx, 2
	shr	ecx, 1
	cmp	factor, 2
	je	convRateDown_16mt
	add	edx, 4
	cmp	factor, 4
	je	convRateDown_16mt
	add	edx, 8
;convRateDown_8s
convRateDown_16mt:
	lodsw
	stosw
	add	esi, edx
	loop	convRateDown_16mt
	ret
convRateDown_8m:
	mov	edx, 1
	cmp	factor, 2
	je	convRateDown_8mt
	add	edx, 2
	cmp	factor, 4
	je	convRateDown_8mt
	add	edx, 4
convRateDown_8mt:
	lodsb
	stosb
	add	esi, edx
	loop	convRateDown_8mt
	ret

ConvRate endp

CopyEOS proc public uses es edi esi ecx\
	outb:near ptr, inb:near ptr, count:dword, bits:byte

	push	ds
	pop	es
	mov	edi, outb
	mov	esi, inb
	mov	ecx, count
	cmp	ecx, 0
	jle	copyeosOut
	cmp	bits, 8
	je	copyeos8
copyeos16:
	lodsw
	stosw
	add	edi, 2
	loop	copyeos16
	ret
copyeos8:
	lodsb
	stosb
	add	edi, 1
	loop	copyeos8
copyeosOut:
	ret

CopyEOS endp

;DupeChannel proc public uses es edi esi ecx\
;	inb:near ptr, count:dword, channel:dword, bits:byte
;
;	push	ds
;	pop	es
;	mov	esi, inb
;	mov	edi, esi
;	mov	ecx, count
;	cmp	ecx, 0
;	jle	dupeOut
;	xor	eax, eax
;	mov	al, bits
;	shr	eax, 3
;	.if channel == 0
;		add	esi, eax
;	.else
;		add	edi, eax
;	.endif
;	cmp	bits, 8
;	je	dupe8
;dupe16:
;	lodsw
;	stosw
;	add	esi, 2
;	add	edi, 2
;	loop	dupe16
;	ret
;dupe8:
;	lodsb
;	stosb
;	add	esi, 1
;	add	edi, 1
;	loop	dupe8
;dupeOut:
;	ret
;
;DupeChannel endp

DupeChannel proc public uses es edi esi ecx\
	inb:near ptr, count:dword, channel:dword, bits:byte

	push	ds
	pop	es
	mov	esi, inb
	mov	edi, esi
	mov	ecx, count
	cmp	ecx, 0
	jle	avgOut
	xor	eax, eax
	mov	al, bits
	shr	eax, 3
	.if channel == 0
		add	esi, eax
		cmp	bits, 8
		je	avg8_even
		lodsw
		sub	esi, 2
		movsx	edx, ax
		jmp	short avg16
avg8_even:
		lodsb
		sub	esi, 1
		movsx	edx, al
		and	edx, 0ffh
		jmp	short avg8
	.else
		add	edi, eax
		cmp	bits, 8
		je	avg8_odd
		lodsw
		add	esi, 2
		dec	ecx
		jz	avg16_1
		movsx	edx, ax
		jmp	short avg16
avg8_odd:
		lodsb
		add	esi, 1
		dec	ecx
		jz	avg8_1
		movsx	edx, al
		and	edx, 0ffh
		jmp	short avg8
	.endif
avg16:
	lodsw
	movsx	eax, ax
	add	edx, eax
	xchg	eax, edx
	shr	eax, 1
	stosw
	add	esi, 2
	add	edi, 2
	loop	avg16
avg16_1:
	.if channel != 0
		sub	esi, 4
		lodsw
		stosw
	.endif
	ret
avg8:
	lodsb
	add	edx, eax
	xchg	eax, edx
	shr	eax, 1
	stosb
	add	esi, 1
	add	edi, 1
	loop	avg8
avg8_1:
	.if channel != 0
		sub	esi, 2
		lodsb
		stosb
	.endif
avgOut:
	ret

DupeChannel endp

MonitorNoise proc public uses esi ecx\
	inb:near ptr, count:dword, bits:byte

	mov	esi, inb
	mov	ecx, count
	cmp	ecx, 0
	jle	noiOut
	cmp	bits, 8
	je	noi8
	shr	ecx, 1
noi16:
	lodsw
	cmp	ah, 5
	jg	noiOut
	cmp	ah, -5
	jl	noiOut
	loop	noi16
	xor	eax, eax
	ret
noi8:
	lodsb
	cmp	al, 7ah
	jb	noiOut
	cmp	al, 85h
	ja	noiOut
	loop	noi8
	xor	eax, eax
	ret
noiOut:
	mov	eax, 1
	ret

MonitorNoise endp

MemCopy proc public uses es edi esi ecx,\
	outb:near ptr, inb:near ptr, count:dword

	push	ds
	pop	es
	mov	esi, inb
	mov	edi, outb
	mov	ecx, count
	rep movsb
	ret

MemCopy endp

	end

