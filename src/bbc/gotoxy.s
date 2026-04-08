	.include "oslib/os.inc"

	.export		gotoxy
	.export		_gotoxy
	.import		popa

gotoxy:
_gotoxy:
	pha
	lda	#31
	jsr	OSWRCH
	jsr	popa
	jsr	OSWRCH
	pla
	jmp	OSWRCH
