 @x: word 0
 @p: word 0
 
 @main:
 	psh bp
 	lbp sp	; marco de la pila
 
 	irq 3		
 	sta [@x]	; cin >> x;
 
 	lea @p
 	psh b
 	lda #2
 	pop b
 	sta [b]		; p= 2;
 
 @1:				; while (
 	lea @x
 	lda [b]		; 	x	
 	psh a
 	
 	lda #1		; 	>
 	cmp			;   1
 
 	clr a
 	jle @2
 	inc
 @2:
 	tst
 	jeq @3 		;  ) {
 
 @4:	lea @x		;  while (x%p==0) {
 	lda [b]
 	psh a
 
 	lea @p	
 	lda [b]
 
 	mod
 	psh a
 
 	lda #0
 	cmp
 	clr a
 	jne @5
 	inc
 
 @5:	tst
 	jeq @6
 
 	lea @p		; cout << p << ' ';
 	lda [b]
 	irq 1
 	lda #' '
 	irq 5
 
 	lea @x		; x= x/p;
 	psh b
 	lea @x
 	lda [b]
 	psh a
 
 	lea @p
 	lda [b]
 
 	div
 
 	pop b
 	sta [b]
 	jmp @4		; }
 
 @6:	
 	lea @p		;  ++p;
 	lda [b]
 	inc
 	sta [b]
 	jmp @1		; }
 	
 @3:
 	lda #'\n' 	; cout << endl;
 	irq 5
 
 	pop bp
 	lbp sp		; se destruye el marco
 	rtn
 
 @pila:
 	end @main, @pila
 
