@n: word 0
@k: word 0
@r: word 0

@main:
	psh bp
	lbp sp
	lda #0
	sta [@r]
	clr a
	irq 3
	sta[bp+@n]
	psh a
	irq 3
	sta[bp+@k]
	cmp
	jgt @coefbin
	irq 0

@coefbin:
	jmp @0

   @1: pop 2
   	jmp @end

    @2: jsr @coef
   	jmp @1

    @3: lda [bp+@k]
    	psh a
	jmp @2

    @4: lda [bp+@n]
    	psh a
	jmp @3

    @0: jmp @4

    @coef:
    	lda [bp+@n]
	psh a
	lda [bp+@k]
	cmp
	jeq @one

	lda [bp+@k]
	psh a
	lda #0
	cmp
	jeq @one

	lda [bp+@n]
	dec
	sta [bp+@n]
	jsr @coef

	lda [bp+@k]
	dec
	sta [bp+@k]
	jsr @coef

	rtn

   @one:
 	lda #1
	psh a
	lda [@r]
    	add
	sta [@r]
	clr a
	rtn

@end:
    	lda [@r]
    	irq 1
    	clr a
	pop bp
	lbp sp

@pila:
       end @main,@pila
