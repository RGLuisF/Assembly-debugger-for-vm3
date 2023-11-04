@n: word 0
@k: word 0
@r: word 0
@x: word 0

@main:
	psh bp
	lbp sp
	lda #1
	sta [@r]
	clr a
	irq 3
	sta [@n]
	psh a
	irq 3
	sta [@k]
	cmp
	jgt @coef
	irq 0

@coef:  
	lda [@k]
	psh a
	lda [@x]
	cmp
	jeq @end

	lda [@r]
	psh a
	lda [@n]
	psh a
	lda [@x]
	sub
	mul
	psh a
	lda #1
	psh a
	lda [@x]
	add
	div
	sta [@r]
	lda [@x]
	inc
	sta[@x]
	jmp @coef
	
     @end:
     	lda [@r]
	irq 1
	clr a
	lsp bp
	pop bp
	rtn
     	
@pila:
     end @main,@pila
