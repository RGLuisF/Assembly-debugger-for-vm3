@a: word 0
@b: word 0

@main:
	psh bp
	lbp sp

	irq 3
	sta [@a]
	irq 3
	sta [@b]
	clr a
@euc:
	lda [@b]
	psh a
	lda #0
	cmp
	jeq @end
	
	lda [@b]
	psh a
	lda [@a]
	psh a
	lda [@b]
	mod
	sta [@b]
	xch
	sta [@a]
	clr a
	jmp @euc

@end:   
	lda [@a]
	irq 1
	clr a

	lsp bp
	pop bp
	rtn

@pila:
	end @main,@pila
