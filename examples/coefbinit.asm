@n: word 0
@k: word 0
@r: word 0
@x: word 0

@main:
	psh bp		;marco de la pila
	lbp sp	
	lda #1
	sta [@r]
	clr a
	irq 3
	sta [@n]	 ;leer n  
	psh a   
	irq 3
	sta [@k]	 ;leer k
	cmp		 ;n mayor que k?
	
	lda #0
	sta [@x]	; modificacion	
	clr a
	
	jgt @coef
	sta [@r]
	jmp @end
	
@coef:  
	lda [@k]
	psh a
	lda [@x]	 ;x menor que k?
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
	lda [@x]
	psh a
	lda #1
	add
	div
	sta [@r]
	
	lda [@x]
	inc
	sta [@x]
	clr a
	jmp @coef        ;x++ y repetir
@end:
     	lda [@r]
	irq 1
	clr a
	pop bp
	lbp sp
	rtn
     	
@pila:
     end @main,@pila
