@n: word 0
@r: word 0

@main: 
	psh bp
	lbp sp
	irq 3
	sta [@n]
	sta [@r]
	
@fac:
	jmp @0
    
    @1: pop 2
        jmp @6

    @2: jsr @f
    	jmp @1

    @3: lda [@n]
    	psh a
	jmp @2

    @0: jmp @3

    @f:	lda [@n]
    	psh a
    	lda #1
    	cmp
    	jeq @5
    	
    	lda [@r]
    	psh a
    	lda [@n]
    	dec
    	sta [@n]
    	mul
    	sta [@r]
    	
    	jsr @f
    @5: rtn
    @6: lda [@r]
    	irq 1
    	clr a
    	pop bp
    	lbp sp    	
	rtn
@pila:
	end @main,@pila
