*= 0x100
@x: word 0
@r: word 0
@p: word 0
@c: word 0
@t: word 0
@ro: word 0

@main:
	psh bp
	lbp sp
	irq 3
	sta [@x]

	lda #0
	sta [@p]
	sta [@c]
	sta [@r]
	sta [@ro]
	
	psh a
	lda [@x]
	cmp
	jne @noescero

	lda [@c]
	dec 
	sta [@c]
	psh a
	lda #0
	cmp 
	 
	jgt @raiz
	
	jmp @fin

@noescero:
	lda [@p]
	psh a
	lda #2
	shl
	psh a
	lda [@x]
	psh a
	lda #3
	and
	or
	sta [@p]

	lda [@c]
	inc
	sta [@c]

	lda [@x]
	psh a
	lda #2
	shr
	sta [@x]

	psh a
	lda #0
	cmp
	jne @noescero

@raiz:
	lda [@ro]
	psh a
	lda #1
	shl
	sta [@ro]

	lda [@r]
	psh a
	lda #2
	shl
	psh a
	lda [@p]
	psh a
	lda #3
	and
	or
	sta [@r]

	lda [@p]
	psh a
	lda #2
	shr
	sta [@p]

	lda [@ro]
	psh a
	lda #1
	shl
	psh a
	lda #1
	or
	sta [@t]

	psh a
	lda [@r]
	cmp
	jle @tre

	lda [@c]
	dec 
	sta [@c]
	psh a
	lda #0
	cmp
	jgt @raiz

	jmp @fin
@tre:
	lda [@r]
	psh a
	lda [@t]
	sub
	sta [@r]

	lda [@ro]
	psh a
	lda #1
	or
	sta [@ro]

	lda [@c]
	dec 
	sta [@c]
	psh a
	lda #0
	cmp
	jgt @raiz
@fin:
	lda [@ro]
	irq 1
	lsp bp
	pop bp
	rtn
@pila:
	end @main, @pila
