@n: word 0
 
 main:
 	psh bp
 	lbp sp
 	lda #0
 	sta [@n]
 ciclo:
 	lda [@n]
 	psh a
 	lda #1
 	add
 	sta [@n]
 	irq 1
 	lda [@n]
 	psh a
 	lda #10
 	sub
 	jlt ciclo
 lsp bp
 pop bp
 rtn
 
 pila:
 	 end main, pila
