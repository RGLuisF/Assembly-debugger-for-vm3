 ; Luis Fernando Romero Garcia
 
 @n: word 0		
 @k: word 0
 @i: word 0
 
 @main:
 	psh bp
 	lbp sp		; marco de la pila
 
 	lda #0x1234
 	sta [@i]
 
 	irq 3
 	sta [@n]	; leer n
 	
 	lda #1
 	psh a
 
 @ps:	
 	lda #2
 	mul
 	psh a
 
 	lda [@n]
 	dec
 	sta [@n]
 	
 	lda [@n]
 	psh a
 	lda #0
 	cmp
 	jgt @ps
 	xch
 	dec
 	sta [@n]	; calcular 2^n
 	sta [@k]
 
 @2:
 	lda #'{'
 	irq 5
 
 @tobin:
 	lda [@n]
 	psh a
 	lda #2
 	mod	  	; n mod 2
 	psh a
 	
 	lda #0
 	cmp
 	jeq @1
 	lda [@i]
 	irq 1		; si el bit es 1, escribir i
 
 @1:
 	lda [@i]
 	inc
 	sta [@i]
 	
 	lda [@n]
 	psh a
 	lda #2
 	div
 	sta [@n]  	; n/2
 
 	psh a
 	lda #0
 	cmp	
 	jgt @tobin	; repite si n distinto de cero
 
 	lda #'}'
 	irq 5
 
 	lda #'\n'
 	irq 5		; salto de linea
 
 	lda #1
 	sta [@i]	; resetear i
 
 	lda [@k]
 	dec
 	psh a
 	sta [@n]
 	sta [@k]
 	lda #0
 	cmp
 	jgt @2	; repetir de 2^n hasta 1
 	
 	lda #'{'
 	irq 5
 	lda #'}'
 	irq 5
 	clr a		; conjunto vacio
 	
 @end:
 	lsp bp
 	pop bp
 	rtn
 
 @pila:
 	end @main,@pila
 
