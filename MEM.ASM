;±±±±±±±±±±±±±±±±±±±±±±±±±±±±± FILE HEADER ±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
;
;    file: mem.asm
; project: in house speed, watcom dos version
; NAV created: 6/11/97
; 
;
; made fast memcpy routine
;
;
;±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
.486p
.model flat 

jumps
local

.code
	public memcpy
	public _memcpy
;	public _light_ren
 ;	public light_ren

memcpy	proc  
	arg 	blocksA:ptr byte,dataA:ptr byte,Asize:dword	

	push 	ebp
	mov  	ebp,esp

	push 	ebx
	push 	esi
	push 	edi

	mov	ecx,Asize
	cmp	ecx,0 		;make sure size is above 0
	je	@@bye


	mov	edi,dword ptr [blocksA]
	mov	esi,dword ptr [dataA]

	cmp	ecx,5
	jb	@@looper2

	mov	ebx,ecx
	and	ebx,3
	shr	ecx,2

	mov	edx,4

	align 4

@@looper:
	mov	eax,[esi]
	add	edi,edx
	add	esi,4
	dec	ecx
	mov	[edi - 4],eax
	jnz	@@looper

	cmp	ebx,0
	je	@@bye

	mov	ecx,ebx

@@looper2:
	mov	al,[esi]
	inc	esi
	mov	[edi],al
	inc	edi
	dec	ecx
	jnz	@@looper2

@@bye:
	pop  	edi
	pop  	esi
	pop  	ebx
	pop  	ebp

	ret

memcpy 	endp

_memcpy	proc  
	arg 	blocksA:ptr byte,dataA:ptr byte,Asize:dword	

	push 	ebp
	mov  	ebp,esp

	push 	ebx
	push 	esi
	push 	edi

	mov	ecx,Asize
	cmp	ecx,0 		;make sure size is above 0
	je	@@bye1


	mov	edi,dword ptr [blocksA]
	mov	esi,dword ptr [dataA]

	cmp	ecx,5
	jb	@@looper4

	mov	ebx,ecx
	and	ebx,3
	shr	ecx,2

	mov	edx,4

@@looper3:
	mov	eax,[esi]
	add	edi,edx
	add	esi,4
	dec	ecx
	mov	[edi - 4],eax
	jnz	@@looper3

	cmp	ebx,0
	je	@@bye1

	mov	ecx,ebx

@@looper4:
	mov	al,[esi]
	inc	esi
	mov	[edi],al
	inc	edi
	dec	ecx
	jnz	@@looper4

@@bye1:
	pop  	edi
	pop  	esi
	pop  	ebx
	pop  	ebp

	ret

_memcpy 	endp

comment ?

_light_ren	proc ; char *sptr,char *tptr,long wrapmask,long tsy.lval,long src_inc,long sptr_inc,char *sptr_end
	arg dest:ptr byte,sour:ptr byte,masker:dword,index:dword,srcinc:dword,d_inc:dword,sptr_ending:ptr byte

	push	bp
	mov	bp,sp
	push	ebx esi edi

	mov	edi,dword ptr [dest]
	mov	esi,dword ptr [sour]
	mov	edx,srcinc
	mov	ecx,index
	mov	ebx,DWORD ptr [sptr_ending]
	
@@s_loop:
	and 	ecx,0ffff0000h
	mov	al,[esi+ecx]
	add	ecx,edx
	mov	[edi],al
	and	ecx,masker
	inc	edi
	mov	[edi],al
	add	edi,d_inc
	cmp	edi,ebx
	jl	@@s_loop

	pop	edi esi ebx
	pop	bp
	ret
_light_ren	endp

light_ren	proc ; char *sptr,char *tptr,long wrapmask,long tsy.lval,long src_inc,long sptr_inc,char *sptr_end
	arg dest:ptr byte,sour:ptr byte,masker:dword,index:dword,srcinc:dword,d_inc:dword,sptr_ending:ptr byte

	push	bp
	mov	bp,sp
	push	ebx esi edi

	mov	edi,dword ptr [dest]
	mov	esi,dword ptr [sour]
	mov	edx,srcinc
	mov	ecx,index
	mov	ebx,DWORD ptr [sptr_ending]
	
@@s_loop1:
	and 	ecx,0ffff0000h
	mov	al,[esi+ecx]
	add	ecx,edx
	mov	[edi],al
	and	ecx,masker
	inc	edi
	mov	[edi],al
	add	edi,d_inc
	cmp	edi,ebx
	jl	@@s_loop1

	pop	edi esi ebx
	pop	bp
	ret
light_ren	endp
	 ?
		end
