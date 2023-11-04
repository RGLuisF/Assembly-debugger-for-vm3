@n=-6
@k=-8
@r: word 0

@main:
	psh bp
	lbp sp	
	
	irq 3
	sta [@r] 
	irq 3
	psh a
	lda [@r]
	psh a

	lda #0
	sta [@r]
	clr a
	
	jsr @cb
	pop 4
	
	lda [@r]
	irq 1
	clr a
	jmp @end
		
@cb:
	psh bp
	lbp sp		; marco de la pila
	
	lda [bp+@k]
	psh a
	lda [bp+@n]
	cmp
	jgt @end

	lda [bp+@n]
	psh a
	lda [bp+@k]
	cmp
	jeq @else	; k=n?

	lda [bp+@k]
	psh a
	lda #0
	cmp	
	jeq @else	; k=0?
	
	lda [bp+@k]
	psh a
	lda #1
	sub
	psh a		; k--
	
	lda [bp+@n]
	psh a
	lda #1
	sub
	psh a
	clr a		; n--
	
	jsr @cb
	pop 4
	
	lda [bp+@k]
	psh a
	lda [bp+@n]
	psh a
	lda #1
	sub
	psh a
	clr a		; n--
	
	jsr @cb
	pop 4
	
	lsp bp
	pop bp
	rtn
	
@else:
	lda #1
	psh a
	lda [@r]
	add
	sta [@r]
	clr a
	
	lsp bp
	pop bp
	rtn

@end: 
	lsp bp
	pop bp
	rtn

@pila:
	end @main, @pila
