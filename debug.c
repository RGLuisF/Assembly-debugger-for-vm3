#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>

typedef int       INT;

#define MaxMem  65535
#define Byte16  65535

int    un, go, db;				//  un: desensamble
								//  go: ejecucion
								//  db: depuracion
int    sp, pc, bp, aa, bb;		//  registros
								//  sp:  stack pointer
								//  bp:  base pointer
								//  pc:  program counter
								//  aa:  acumulador a
								//  bb:  acumulador b [indice]
int    C, Z, N, O;				//  banderas
								//  C:   carry
								//	Z:   zero
								//	N:   negative
								//  O:   overflow
	
unsigned char  mem[MaxMem+1];	//  memoria
unsigned char  dis[32];			//  desensamble de una instruccion

WINDOW *cpu, *memory, *info, *vm, *fsel;
int vmy,vmx;
int init,nv,pl,df;
int bpts[10];

void Error(char* mesg)
{
   fprintf(stderr, "Error [0x%04x]:  %s\n", pc, mesg);
//   exit(1);
}

int SetN(INT x)					//   Analizar negativo
{
   return N= (x & 0x8000) ? 1 : 0;
}

int SetZ(INT x)					//	Analizar cero
{
   return Z= (x & 0xFFFF)==0 ? 1 : 0;
}

int SetO(INT x)					// Analizar overflow
{
   return O= x ? 1 : 0;
}

int SetC(INT x)					// Analizar acarreo
{
   return C= x & 0x10000 ? 1 : 0;
}

int ReadByte(int addr)			// lee de la memoria un byte.
{
   return mem[addr & MaxMem];
}

void WriteByte(int addr, int x)	// escribir un byte en la memoria
{
   mem[addr & MaxMem]= x;
}

int ReadInt(int addr)			// leer un entero
{
   addr&= MaxMem;
   return ReadByte(addr) | (ReadByte(addr+1)<<8);
}

void WriteInt(int addr, int x)	// escribir un entero
{
   WriteByte(addr, x);
   WriteByte(addr+1, x>>8);
}

void Push(int x)				// meter a la pila un byte
{
   WriteByte(sp,x);
   sp= (sp+1) & MaxMem;
}

void PushV(int v)				// meter v cero en la pila.
{
   int k;
   for (k=0; k<v; ++k)
      Push(0);
}

int  Pop(void)					// sacar un byte de la pila
{
   sp= (sp-1) & MaxMem;
   return ReadByte(sp);
}

void PopV(int v)				// sacar v bytes de la pila
{
   int k;
   for (k=0; k<v; ++k)
      Pop();
}


void iPush(int x)				// meter un entero a la pila
{
   Push(x&255);
   Push(x>>8);
}

int iPop(void)					// sacar un entero de la pila
{
   int hi= Pop();
   int lo= Pop();
   return (hi<<8) + lo;
}

void NOP(void)					// instruccion NOP
{
   pc++;
   sprintf(dis,"NOP");
}

void BAD(void)					// codigos desconocidos
{
   //char str[128];
   //sprintf(str, "Invalid instruction code 0x%02x", mem[pc]);
   //Error(str);
   go= 0;
   sprintf(dis,"BAD");
}

INT neg(INT x)					// Analizar si x es negativo
{
  return x & 0x8000 ? 1 : 0;
}

INT pos(INT x)					// Analizar si x es positivo o cero
{
  return x & 0x8000 ? 0 : 1;
}

void ADD(void)					// Addition
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT r= a+b;

      SetZ(r);
      SetN(r);
      SetC(r);
      SetO( neg(a) && neg(b) && pos(r)  || pos(a) && pos(b) && neg(r) );
      aa= r & 0xffff;
     }
   sprintf(dis,"ADD");
   pc++;
}

void SUB(void)					//  Substraction
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT r= a-b;

      SetZ(r);
      SetN(r);
      SetC(r);
      SetO(neg(a) && pos(b) && pos(r)  || pos(a) && neg(b) && neg(r));
      aa= r & 0xffff;
     }
   sprintf(dis,"SUB");
   pc++;
}

void MUL(void)					// multiplication
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT r= a*b;

      SetZ(r);
      SetN(r);
      aa= r & 0xffff;
     }
   sprintf(dis,"MUL");
   pc++;
}

void DIV(void)					// division
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT r= 0;

      if (b==0)
         SetO(1);
      else
         {
          r= a/b;
          SetO(0);
         }
      SetZ(r);
      SetN(r);
      aa= r & 0xffff;
     }
   sprintf(dis,"DIV");
   pc++;
}

void MOD(void)					//  remainder or modulus
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT r= 0;

      if (b==0)
         SetO(1);
      else
         {
          r= a%b;
          SetO(0);
         }
      SetZ(r);
      SetN(r);
      aa= r & 0xffff;
     }
   sprintf(dis,"MOD");
   pc++;
}

void CMP(void)					// compare
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT r= a-b;

      SetZ(r);
      SetN(r);
      SetC(r);
      SetO(neg(a) && pos(b) && pos(r)  || pos(a) && neg(b) && neg(r));
     }
   sprintf(dis,"CMP");
   pc++;
}

void SHL(void)					//  shift left
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT c= a<<b;

      SetZ(c);
      SetN(c);
      aa= c & 0xFFFF;
     }
   sprintf(dis,"SHL");
   pc++;
}

void SHR(void)					// shift right
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT c= a>>b;

      SetZ(c);
      SetN(c);
      aa= c & 0xFFFF;
     }
   sprintf(dis,"SHR");
   pc++;
}

void NEG(void)					//  Negative (cambio de signo)
{
   if (go)
     {
      int r= -aa;
      SetZ(r);
      SetN(r);
      aa= r & 0xFFFF;
     }
   sprintf(dis,"NEG");
   pc++;
}

void INC(void)					// increment
{
   if (go)
     {
      INT r= aa+1;

      SetZ(r);
      SetN(r);
      SetC(r);
      SetO( pos(aa) && neg(r) );
      aa= r & 0xFFFF;
     }
   sprintf(dis,"INC");
   pc++;
}

void DEC(void)					// decrement
{
   if (go)
     {
      INT r= aa-1;

      SetZ(r);
      SetN(r);
      SetC(r);
      SetO( neg(aa) && pos(r) );
      aa= r & 0xFFFF;
     }
   sprintf(dis,"DEC");
   pc++;
}

void XCH(void)					// exchange  accumulador-stk
{
  if (go)
     {
      INT r= aa;
      aa= iPop();
      iPush(r);
     }
  sprintf(dis, "XCH");
  pc++;
}


void AND(void)					// bitwise	and
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT r= a & b;

      SetZ(r);
      SetN(r);
      aa= r;
     }
   sprintf(dis,"AND");
   pc++;
}

void OR(void)						// bitwise or
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT r= a | b;

      SetZ(r);
      SetN(r);
      aa= r;
     }
   sprintf(dis,"OR ");
   pc++;
}

void XOR(void)						// bitwise xor
{
   if (go)
     {
      INT a= iPop();
      INT b= aa;
      INT r= a ^ b;

      SetZ(r);
      SetN(r);
      aa= r;
     }
   sprintf(dis,"XOR");
   pc++;
}

void NOT(void)					// bitwise negation
{
   if (go)
     {
      INT r= ~aa;

      SetZ(r);
      SetN(r);
      aa= r & 0xFFFF;
     }
   sprintf(dis,"NOT");
   pc++;
}

void TST(void)
{
   if (go)
     {
      INT r= aa ? 1 : 0;
      SetZ(r);
      SetN(r);
      aa= r;
     }
   sprintf(dis,"TST");
   pc++;
}

void JEQ(void)					// jumpt if equal
{
   INT  dest= ReadInt(pc+1);
   if (go)
      pc= Z ? dest : pc+3;
   else
      pc+= 3;
   sprintf(dis,"JEQ 0x%04x", dest);
}

void JNE(void)					// jump if equal
{
   INT  dest= ReadInt(pc+1);
   if (go)
      pc= !Z ? dest : pc+3;
   else
      pc+= 3;
   sprintf(dis,"JNE 0x%04x", dest);
}

void JLE(void)					// jump if less or equal
{
   INT  dest= ReadInt(pc+1);
   if (go)
      pc= Z || (N^O) ? dest : pc+3;
   else
      pc+= 3;
   sprintf(dis,"JLE 0x%04x", dest);
}

void JGT(void)					// jump if greater
{
   INT  dest= ReadInt(pc+1);
   if (go)
      pc= !(Z || (N^O)) ? dest : pc+3;
   else
      pc+= 3;
   sprintf(dis,"JGT 0x%04x", dest);
}

void JGE(void)					// jump if greater or equal
{
   INT  dest= ReadInt(pc+1);
   if (go)
      pc= Z || !(N^O) ? dest : pc+3;
   else
      pc+= 3;
   sprintf(dis,"JGE 0x%04x", dest);
}

void JLT(void)					// jump if less than
{
   INT  dest= ReadInt(pc+1);
   if (go)
      pc= !Z && (N^O) ? dest : pc+3;
   else
      pc+= 3;
   sprintf(dis,"JLT 0x%04x", dest);
}

void JMP(void)
{
   INT  dest= ReadInt(pc+1);
   if (go)
      pc = dest;
   else if(pc==0xfffd) pc=dest;
   else
      pc+= 3;
   sprintf(dis,"JMP 0x%04x", dest);
}

void JMPb(void)					// direct jump
{
   if (go)
      pc= bb;
   else
      pc++;
   sprintf(dis,"JMP [B]");
}

void JSR(void)
{
   INT  dest= ReadInt(pc+1);
   if (go)
      {
       iPush(pc+3);
       pc=dest;
      }
   else if(pc==0xfff7) pc=dest;
   else
      pc+= 3;
   sprintf(dis,"JSR 0x%04x", dest);
}

void JSRb(void)					// indirect jump indirecto, register B
{
   INT dest= bb;
   if (go)
      {
       iPush(pc+1);
       pc= dest;
      }
   if(df) pc=dest;
   else
      pc++;
   sprintf(dis,"JSR [B]");
}

void RTN(void)					// Return
{
   if (go)
      pc= iPop();
   else if(df){
      pc = iPop();
      iPush(pc);
    }
   else
      pc++;
   sprintf(dis,"RTN");
}

void IRQ(void)					// servicios
{   
   INT r= ReadInt(pc+1);
   sprintf(dis,"IRQ 0x%04x", r); 
   pc+= 3;
   if (go)
      {
       INT   x, y;
       INT   t, n, k;
       int my,mx;
       getmaxyx(vm,my,mx);
       switch(r)
         {
          case 0:   
                    go = 0;               /* stop */
                    break;

          case 1:                        /* print integer */
                    x = (0x8000 & aa) ? aa|0xFFFF0000 : aa & 0x0000FFFF;        
                    if(vmx==0){
                    wmove(vm,vmy,0);
                    wclrtoeol(vm);
                    mvwprintw(vm,vmy,0,"vm>");
                    vmx=4;
                    }
                    mvwprintw(vm,vmy,vmx,"%d",x);
                    vmx += floor(log10(abs(x))) + 1; 
                    break;

          case 2:                        /* print string */
                    for(x= bb; mem[x]; x++){
                     //  mvwprintw(vm,vmy,vmx,"%c",mem[x]);
                    }
                    break;

          case 3:   echo(); /* read integer */
                    if(vmy!=0){ vmy++;
                    vmx=0;}
                    wmove(vm,my,0);
                    wclrtoeol(vm);
                    mvwprintw(vm,vmy,0,"vm<");
                    mvwscanw(vm,vmy,4,"%d", &x);
                    aa= x & 0xFFFF;
                    noecho();
                    mvwprintw(vm,vmy,4,"%d",x);
                    if(vmy<my-1) vmy++;
                    else vmy=0;
                    vmx=0;
                    break;

          case 5:   if(vmx==0){
                    wmove(vm,vmy,0);
                    wclrtoeol(vm);
                    mvwprintw(vm,vmy,0,"vm>");
                    vmx=4;
                    }
                    mvwprintw(vm,vmy,vmx,"%c", aa & 0xFF ); /*  print char */
                    vmx++;
                    if((aa == '\n') || (0xFF == '\n')){
                     if(vmy<my-1) vmy++;
                     else vmy=0;
                    vmx=0;
                    }
                    break;

          case 4:   printf("%s", (char*)&mem[bb]);
                    break;
                                        /*  switch search   */
          case 7:   n= iPop();          /*  number of cases */
                    t= iPop();          /*  table           */
                    x= iPop();          /*  expression      */
                    for (k=0; k<n; k++)
                       if ( x==ReadInt(t+4*k) )/* case value */
                          break;
                    pc= ReadInt(t+4*k+2); 
         }
      }
    wrefresh(vm);
}

void CLRa(void)						//	Clear accumulator
{
   if (go)
       aa= 0;
   sprintf(dis,"CLR A");
   pc++;
}

void CLRb(void)						// clear b
{
   if (go)
       bb= 0;
   sprintf(dis,"CLR B");
   pc++;
}

void LDAi(void)						// immediate load accumulator
{
   int v= ReadInt(pc+1);
   if (go)
      aa= v;
   sprintf(dis, "LDA #0x%04x",v);
   pc+= 3;
}

void LDAd(void)						// direct load accumulator
{
   int d= ReadInt(pc+1);
   if (go)
      aa= ReadInt(d);
   sprintf(dis, "LDA [0x%04x]",d);
   pc+= 3;
}

void LDAb(void)					// indirect B load accumulator
{
   if (go)
      aa= ReadInt(bb);
   sprintf(dis, "LDA [B]");
   pc++;
}

void LDAbp(void)				// stack load accumulator
{
   int v= ReadInt(pc+1);
   if (go)
      aa= ReadInt(bp+v);
   sprintf(dis, "LDA [BP+0x%04x]",v);
   pc+= 3;
}


void STAd(void)					// store accumulator
{
   int d= ReadInt(pc+1);
   if (go)
      WriteInt(d, aa);
   sprintf(dis, "STA [0x%04x]",d);
   pc+= 3;
}

void STAb(void)					// indirect store accumulator
{
   if (go)
      WriteInt(bb, aa);
   sprintf(dis, "STA [B]");
   pc++;
}

void STAbp(void)				// store accumulator to stk
{
   int v= ReadInt(pc+1);
   if (go)
      WriteInt(bp+v, aa);
   sprintf(dis, "STA [BP+0x%04x]",v);
   pc+= 3;
}


void PSHa(void)					// Push a
{
   if (go)
      iPush(aa);
   sprintf(dis,"PSH A");
   pc++;
}

void PSHb(void)					// Push b
{
   if (go)
      iPush(bb);
   sprintf(dis,"PSH B");
   pc++;
}

void PSHbp(void)				// Push bp
{
   if (go)
      iPush(bp);
   sprintf(dis,"PSH BP");
   pc++;
}

void PSHv(void)					// Push n bytes.
{
   int v= ReadInt(pc+1);
   if (go)
      PushV(v);
   sprintf(dis,"PSH %d", v);
   pc+= 3;
}

void POPa(void)					// Pop accumulator a
{
   if (go)
     {
      aa= iPop();
      SetZ(aa);
      SetN(aa);
     }
   sprintf(dis,"POP A");
   pc++;
}

void POPb(void)					// Pop register b
{
   if (go)
     {
      bb= iPop();
      SetZ(bb);
      SetN(bb);
     }
   sprintf(dis,"POP B");
   pc++;
}


void POPbp(void)				// Pop register bp
{
   if (go)
      bp= iPop();
   sprintf(dis,"POP BP");
   pc++;
}


void POPv(void)					// Pop n bytes
{
   int v= ReadInt(pc+1);
   if (go)
      PopV(v);
   sprintf(dis,"POP %d", v);
   pc+= 3;
}

void LBPi(void)
{
   int v= ReadInt(pc+1);
   if (go)
      bp= v; 
   sprintf(dis,"LBP #0x%04x", v);
   pc+= 3;
}

void LBPb(void)					// bp <-  b
{
   if (go)
      bp= bb;
   sprintf(dis,"LBP B");
   pc++;
}

void LBPsp(void)				// bp <- sp
{
   if (go)
      bp= sp;
   pc++;
   sprintf(dis,"LBP SP");
}

void LSPi(void)					// sp <- int
{
   int v= ReadInt(pc+1);
   if (go)
      sp= v; 
   pc+=3;
   sprintf(dis,"LSP #0x%04x", v);
}

void LSPb(void)					// sp <- B
{
   if (go)
      sp= bb;
   pc++;
   sprintf(dis,"LSP B");
}

void LSPbp(void)				// sp <- bp
{
   if (go)
      sp= bp;
   pc++;
   sprintf(dis,"LSP BP");
}

void LEAd(void)					// B <- dir
{
   int r = ReadInt(pc+1);
   if (go)
      bb= r;
   pc+= 3;
   sprintf(dis,"LEA 0x%04x", r);
}

void LEAbp(void)				// B <- BP + desp
{
   int r = ReadInt(pc+1);
   if (go)
      bb= (bp+r) & 0xFFFF;
   pc+= 3;
   sprintf(dis,"LEA BP+0x%04x", r);
}

void LEAibp(void)				// B <- Mem[BP+bp];     B= *p;
{
   int r = ReadInt(pc+1);
   if (go)
      bb=  ReadInt((bp+r) & 0xFFFF);
   pc+= 3;
   sprintf(dis,"LEA [BP+0x%04x]", r);
}

void ADBa(void)					//	B<-  B+A
{
   if (go)
      bb= (bb+aa) & 0xffff;
   pc++;
   sprintf(dis, "ADB A");
}


void (*inst[])(void) = {
/*         00    01    02    03    04    05    06    07     */
/*         08    09    0a    0b    0c    0d    0e    0f     */

/* 00 */  NOP,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,
          BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,

/* 10 */  ADD,  SUB,  MUL,  DIV,  MOD,  CMP,  SHL,  SHR,
          NEG,  INC,  DEC,  XCH,  BAD,  BAD,  BAD,  BAD, 

/* 20 */  AND,   OR,  XOR,  NOT,  TST,  BAD,  BAD,  BAD, 
          BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 

/* 30 */  JEQ,  JNE,  JLE,  JGT,  JGE,  JLT,  BAD,  BAD, 
          JMP,  JMPb, JSR,  JSRb, RTN,  IRQ,  BAD,  BAD, 

/* 40 */  CLRa, CLRb, LDAi, LDAd, LDAb, LDAbp, BAD, BAD, 
          BAD,  BAD,  BAD,  STAd, STAb, STAbp, BAD, BAD, 

/* 50 */  PSHa,  PSHb, PSHbp,  PSHv,  BAD,  BAD, BAD,  BAD, 
          BAD,   BAD,    BAD,   BAD,  BAD,  BAD, BAD,  BAD, 

/* 60 */  POPa,  POPb, POPbp,  POPv,  BAD,  BAD, BAD,  BAD, 
          BAD,   BAD,    BAD,   BAD,  BAD,  BAD, BAD,  BAD, 

/* 70 */  LBPb,  LBPsp,  LBPi,  BAD,  BAD,  BAD,  BAD,  BAD, 
          LSPb,  LSPbp,  LSPi,  BAD,  BAD,  BAD,  BAD,  BAD, 

/* 80 */  LEAd,  LEAbp,  LEAibp, BAD,  BAD,  BAD,  BAD,  BAD, 
          ADBa,    BAD,  BAD,   BAD,  BAD,  BAD,  BAD,  BAD, 

/* 90 */  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 
          BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 

/* a0 */  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 
          BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 

/* b0 */  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 
          BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 

/* c0 */  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,
          BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,

/* d0 */  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,
    	  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,

/* e0 */  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,
          BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,

/* f0 */  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,
          BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD

};


int readaddr(const char* line, int* a, int da)
{
   int n;
   const char* p;
 
   *a= da;
   for (p=line; *p!=0 && isspace(*p); p++)
        ;

   if (*p=='-' || *p=='\0')
      return 1;

   n= sscanf(p, "%x", a);
   return 1;
}

int status(int d)
{
   printf(" pc=%04x  bp=%04x  a=%04x  b=%04x  sp= %04x"
          " Flags: %c%c%c%c",
          pc, bp, aa, bb, sp,  
          N?'N':' ',Z?'Z':' ',O?'O':' ',C?'C':' ');

   if (d)
       printf("%s", dis);

   printf("\n");
}

WINDOW* createwin(int h, int w, int y0, int x0)
{
  WINDOW *win;
  win = newwin(h,w,y0,x0);
  wrefresh(win);
  return win;
}

void deletewin(WINDOW *window)
{
   werase(window);
   werase(window);
   delwin(window);
}

void printborders(WINDOW* win,int y0,int x0){
  int my,mx;
  getmaxyx(win,my,mx); 
  
  mvwvline(win,y0,x0,0,my);
  mvwvline(win,y0,x0+mx-1,0,my);
  mvwvline(win,y0,26,0,2*my/3);
  mvwvline(win,my/3,mx/3,2*my/3,2*mx/3);
  mvwvline(win,2*my/3,31,0,mx/3);
  mvwhline(win,y0,x0,0,mx);
  mvwhline(win,my-1,x0,0,mx);
  mvwhline(win,2*my/3,x0,0,mx);
 
  mvwaddch(win,y0,x0,ACS_ULCORNER);
  mvwaddch(win,y0,mx-1,ACS_URCORNER);
  mvwaddch(win,my-1,0,ACS_LLCORNER);
  mvwaddch(win,my-1,mx-1,ACS_LRCORNER);
  mvwaddch(win,2*my/3,0,ACS_LTEE);
  mvwaddch(win,2*my/3,26,ACS_BTEE);
  mvwaddch(win,2*my/3,mx-1,ACS_RTEE);
  mvwaddch(win,y0,26,ACS_TTEE);
  mvwaddch(win,2*my/3,31,ACS_TTEE);
  mvwaddch(win,my-1,31,ACS_BTEE);

  mvwprintw(win,y0,1,"CPU");
  mvwprintw(win,2*my/3,1,"MEMORY DUMP");
  mvwprintw(win,2*my/3,32,"INFO");
  mvwprintw(win,y0,27,"VM");

  wrefresh(win);
}

void printcpu(WINDOW* win)
{
  int k,h,w,p,bpf;
  getmaxyx(win,h,w);
  p = pc;
  if(pl){
  df=1;
  for (k=0; k<h;k++)
  {
    int oldpc = pc;
    inst[mem[pc]]();
    pc = pc % (MaxMem+1);
    bpf=0;
    for (int i=0;i<10;i++){
      if(bpts[i]==oldpc){
        bpf=1;
        mvwaddch(win,k,0,ACS_DIAMOND);
      } 
    }
    if(oldpc == p){
    mvwaddch(win,k,0,ACS_RARROW);
    mvwprintw(win,k,1+bpf," %04x %.*s",oldpc, w-8, dis);
    }
    else
    mvwprintw(win,k,0+bpf," %04x %.*s", oldpc, w-6, dis);
    }
  pc = p;
  df=0;
  }
  else mvwprintw(win,0,0,"No program loaded");
  wrefresh(win); 
}

void printmem(WINDOW* win)
{
  int k,p,my,mx,i=-1;
  getmaxyx(win,my,mx);
  p = pc;
  for (k=0; k<my*8; k++)
  { 
    if (k%8==0){
    i++;
    mvwprintw(win,i,0," %04x", pc);
    }
    mvwprintw(win,i,5+3*(k%8)," %02x", mem[pc++]);
  }
  pc = p;
  wrefresh(win);
}

void printinfo(WINDOW* win)
{
  mvwprintw(win,0,0," a  %04x", aa);
  mvwprintw(win,1,0," b  %04x", bb);
  mvwprintw(win,2,0," pc %04x", pc);
  mvwprintw(win,3,0," sp %04x", sp);
  mvwprintw(win,4,0," bp %04x", bp);
  mvwprintw(win,6,0," Flags");
  mvwprintw(win,7,0," %c %c %c %c",Z?'Z':' ', N?'N':' ', C?'C':' ', O?'O':' ');
  wrefresh(win);
}

void printbar(WINDOW* win,int c){
  int i,l=0;
  char *op[] = {
    "File",
    "Run",
    "Step",
    "Breakpoints",
    "Help",
    "Exit",
  }; 
  for(i = 0; i < 6; i++){
    if(i == c){
      wattron(win, A_REVERSE);
      mvwprintw(win, 0, l,"%s",op[i]);
      wattroff(win, A_REVERSE);
    }
    else mvwprintw(win, 0, l,"%s",op[i]); 
    l = l + 1+ strlen(op[i]);
  }
  wrefresh(win);
}

void deletebar(){
  move(0,0);
  clrtoeol();
}

void menu(WINDOW* win, int my, int mx){
   int gp=go;
   go = 0;
   cpu = createwin(2*my/3-2,25,2,1);
   memory = createwin(my/3-2,30,2*my/3+1,1);
   info = createwin(my/3-2,mx/3-2,2*my/3+1,32);
   
  if(pc==0xfff7){
    wclear(vm);
    vmy=0;
    vmx=0;
   }
   //printborders(win,1,0); 
   printcpu(cpu);
   printmem(memory);
   printinfo(info);
   gp?(go=1):(go=0);
}

void deletemenu(void){ 
   deletewin(cpu);
   deletewin(memory);
   deletewin(info);
}

void readfile(WINDOW* win){
  FILE *code;
  DIR *d;
  char nombre[256];
  int my,mx,c,q=0,ch=0,i=0;
  struct dirent *dir;
  
  memset(nombre,'\0',sizeof(char)*128);
  
  wclear(win);
  getmaxyx(win,my,mx);
  printbar(win,0);
  mvwvline(win,1,0,0,my);
  mvwvline(win,1,mx-1,0,my);
  mvwhline(win,1,1,0,mx);
  mvwhline(win,my-1,1,0,mx);
  mvwaddch(win,1,0,ACS_ULCORNER);
  mvwaddch(win,1,mx-1,ACS_URCORNER);
  mvwaddch(win,my-1,0,ACS_LLCORNER);
  mvwaddch(win,my-1,mx-1,ACS_LRCORNER);
  mvwprintw(win,1,1,"%s",realpath(".",NULL));

  fsel = createwin(my-3,mx-2,2,1);
  do{
    refresh();
    mvwprintw(fsel,0,0,"Select a File:");
    d = opendir(".");
    if (d){
      while ((dir = readdir(d)) != NULL && i<my-3) 
      {
        if(strcmp(dir->d_name,".") && strcmp(dir->d_name,"..") && (strstr(dir->d_name,".e"))!=NULL){
        if(ch==i){
        wattron(fsel,A_REVERSE);
        mvwprintw(fsel,i+1,1,"%.*s\n",mx/2, dir->d_name);
        wattroff(fsel,A_REVERSE);
        sprintf(nombre,"%s",dir->d_name);
        }
        else mvwprintw(fsel,i+1,1,"%.*s\n",mx/2,dir->d_name);
        wrefresh(fsel);
        i++;
        }
      }
    }
    closedir(d);
    i=0;
    if(nombre[1]=='\0'){
      mvwprintw(fsel,0,0,"No compatible files found in:");
      //mvwprintw(fsel,1,0,"%s",realpath(".",NULL));
      mvwprintw(fsel,1,0,"Press ESC to go back");
      wrefresh(fsel);
      if(getch()==27) q=1;
    }
    else{
    switch(getch()){
    case KEY_UP: if(0<ch) ch--;
                 break;
    case KEY_DOWN: ch++;
                   break;
    case 27:memset(nombre,'\0',sizeof(char)*128);
            q=1;
            break;
    case 10:if(strstr(nombre,".e")!=NULL) q=1;
            else{
              wattron(fsel,A_REVERSE);
              mvwprintw(fsel,0,0,"Invalid File Format");
              wattroff(fsel,A_REVERSE);
              wrefresh(fsel);
              sleep(1);
              move(2,2);
              clrtoeol();
            }
            break;
    }
    }
  }while(!q);
  
  if(nombre[1]!='\0'){
  memset(mem,'\0',sizeof(unsigned char)*(MaxMem+1));
  init= 0;
  sp= 0;
  bp= 0;
  SetN(0);
  SetZ(0);
  SetO(0);
  SetC(0);
  code = fopen(nombre,"rt");
  while ((c=fgetc(code)) != EOF)
  {
    int   add, val;
    switch (c)
    {
      case '<': fscanf(code,"%x %x", &init, &sp);
                break;
      case ' ': fscanf(code,"%x %x", &add, &val);
                mem[add]= val;
                
                break;
      case ';': break;
    }
    while ((c=fgetc(code))!=EOF && c!='\n')
      ;
  }
  fclose(code);
  pl = 1;
  }
  pc = MaxMem-8;
  mem[pc+0]= 0x3a;     // JSR
  mem[pc+1]= init%256; // main
  mem[pc+2]= init/256;
  mem[pc+3]= 0x3d;     // IRQ 0
  mem[pc+4]= 0;
  mem[pc+5]= 0;
  mem[pc+6]= 0x38;     // JMP
  mem[pc+7]= pc%256;	
  mem[pc+8]= pc/256;
  
  memset(bpts,-1,sizeof(int)*10);
  deletewin(fsel);
}

void createbreakpoints(WINDOW* win){
  int p,bpf,ch=0,q=0;
  int mx,my,t=0,idx=0;
  int s[1024];
  
  getmaxyx(win,my,mx);
  p = pc;
  pc = MaxMem-8;
  df=1;
  for(int k=0;k<1024;k++){
    s[k] = pc;
    inst[mem[pc]]();
    pc = pc%(MaxMem+1);
  }
  df=0;
  ch = s[0];
  do{
    werase(win);
    for (int k=t; k<my+t;k++)
    {
    pc=s[k];
    inst[mem[pc]]();
    bpf=0;
    for (int i=0;i<10;i++){
      if(bpts[i]==s[k]){
        bpf=1;
        mvwaddch(win,k-t,0,ACS_DIAMOND);
      } 
    }
    if(s[k] == p && s[k] == ch){
      wattron(win,A_REVERSE);
      mvwaddch(win,k-t,1+bpf,ACS_RARROW);
      mvwprintw(win,k-t,2+bpf," %04x %.*s",s[k], mx-8, dis);
      wattroff(win,A_REVERSE);
    }else if(s[k] == ch){
      wattron(win,A_REVERSE);
      mvwprintw(win,k-t,2+bpf,"%04x %.*s",s[k], mx-8, dis);
      wattroff(win,A_REVERSE); 
    }else if(s[k] == p){
      mvwaddch(win,k-t,0+bpf,ACS_RARROW);
      mvwprintw(win,k-t,1+bpf," %04x %.*s",s[k], mx-8, dis);
    }else mvwprintw(win,k-t,0+bpf," %04x %.*s",s[k], mx-6, dis);
    }
    wrefresh(win);
    switch(getch()){
      case KEY_UP: if(0<idx) idx--;
                   break;
      case KEY_DOWN:if(idx<1024) idx++; 
                    break;
      case 27: q=1;
               break;
      case 10: int e=0;
               for(int j=0;j<sizeof(bpts);j++){
                if(bpts[j] == ch) {
                  bpts[j] = -1;
                  e=1;
                }
               }
               if(e==1) break;
               for(int j=0;j<sizeof(bpts);j++){
                if(bpts[j]==-1){
                  bpts[j] = ch;
                  break;
                }
               }
               break;
    }
    if(my+t-1<idx && my+t<1023) t++;
    if(idx<t && 0<=t) t--;
    ch = s[idx];
  }while(!q);
  pc = p;
}

void checkbreakpoint(){
 for(int i=0;i<10;i++){
    if(pc == bpts[i]){
      go = 0;
      break;
    }
  }
}

void help(WINDOW *win){
  int my,mx,t=0,q=0;
  char *p[]={
"                    .,-:;//;:=,\n",
"                . :H@@@MM@M#H/.,+%;,\n",
"             ,/X+ +M@@M@MM%=,-%HMMM@X/,\n",
"           -+@MM; $M@@MH+-,;XMMMM@MMMM@+-\n",
"          ;@M@@M- XM@X;. -+XXXXXHHH@M@M#@/.\n",
"        ,%MM@@MH ,@%=             .---=-=:=,.\n",
"        =@#@@@MX.,                -%HX$$%%%:;\n",
"       =-./@M@M$                   .;@MMMM@MM:\n",
"       X@/ -$MM/                    . +MM@@@M$\n",
"      ,@M@H: :@:                    . =X#@@@@-\n",
"      ,@@@MMX, .                    /H- ;@M@M=\n",
"      .H@@@@M@+,                    %MM+..%#$.\n",
"       /MMMM@MMH/.                  XM@MH; =;\n",
"        /%+%$XHH@$=              , .H@@@@MX,\n",
"         .=--------.           -%H.,@@@@@MX,\n",
"         .%MM@@@HHHXX$$$%+- .:$MMX =M@@MM%.\n",
"           =XMMM@MM@MM#H;,-+HMM@M+ /MMMX=\n",
"             =%@M@M#@$-.=$@MM@@@M; %M%=\n",
"               ,:+$+-,/H#MMMMMMM@= =,\n",
"                     =++%%%%+/:-.\n",
"\n",
"help.txt   For debug version 0.1.  Last change: 2023 July 17\n",
"\n",
"   Virtual Machine 3 by Eduardo Viruena Silva\n"
"            DEBUGGER by Luis Romero\n"
"\n",
"            DEBUGGER - help file\n",
"\n",
"              Select:  Use the ENTER key.\n",
"         Move around:  Use the cursor keys.\n",
"    Close any window:  Use the ESC key.\n",
"        Exit program:  Select the Exit button\n",
"\n",
"   Loading a program:  Select the File button and choose a\n",
"                       program to load. Only files compiled\n",
"                       to machine language with the given asm3\n",
"                       program with extension .e can be loaded.\n",
"\n",
" Compiling with asm3:  To compile a program written in assembly\n",
"                       for this machine it is necessary to use\n",
"                       the given asm3 program given to you.\n",
"                       Use the following command to compile\n",
"                       your .asm program:\n",
"                       ./asm3 program.asm program.e\n",
"\n",
"   Running a program:  Once a program is loaded you can run\n",
"                       it all at once or step by step, by\n",
"                       selecting the corresponding buttons.\n",
"\n",
"Creating breakpoints:  Once a program is loaded, select the\n",
"                       Breakpoints button and then select where\n",
"                       you want to create a breakpoint.\n"
};

  wclear(win);
  getmaxyx(win,my,mx);
  printbar(win,4);
  mvwvline(win,1,0,0,my);
  mvwvline(win,1,mx-1,0,my);
  mvwhline(win,1,1,0,mx);
  mvwhline(win,my-1,1,0,mx);
  mvwaddch(win,1,0,ACS_ULCORNER);
  mvwaddch(win,1,mx-1,ACS_URCORNER);
  mvwaddch(win,my-1,0,ACS_LLCORNER);
  mvwaddch(win,my-1,mx-1,ACS_LRCORNER);
  mvwprintw(win,1,1,"Help");
  
  fsel = createwin(my-3,mx-2,2,1);
  getmaxyx(fsel,my,mx);
 do{
    refresh();
    werase(fsel);
    for(int i=t;i<my+t-2;i++) wprintw(fsel,"%s",p[i]);
    wrefresh(fsel);
    switch(getch()){
      case KEY_UP:if(0<t) t--;
                  break;
      case KEY_DOWN:if(t<52-my) t++;
                    break;
      case KEY_RESIZE:  deletewin(fsel);
                        wclear(win);
                        getmaxyx(win,my,mx);
                        printbar(win,4);
                        mvwvline(win,1,0,0,my);
                        mvwvline(win,1,mx-1,0,my);
                        mvwhline(win,1,1,0,mx);
                        mvwhline(win,my-1,1,0,mx);
                        mvwaddch(win,1,0,ACS_ULCORNER);
                        mvwaddch(win,1,mx-1,ACS_URCORNER);
                        mvwaddch(win,my-1,0,ACS_LLCORNER);
                        mvwaddch(win,my-1,mx-1,ACS_LRCORNER);
                        mvwprintw(win,1,1,"Help");
  
                        fsel = createwin(my-3,mx-2,2,1);
                        getmaxyx(fsel,my,mx);
                        break;
      case 27: q=1;
                break;
    }
    wmove(fsel,0,0);
  }while(!q);
  deletewin(fsel);
}

int main(int narg, char* args[] )
{   
   FILE* code;
   int   quit;
   int   addr0;
   int   c, k, n;
   int   mx, my;
   int   ch=0;
   char* p;
   char  line[1024];

   init= 0;
   sp= 0;
   bp= 0;
   pl= 0;
   df= 0;
   SetN(0);
   SetZ(0);
   SetO(0);
   SetC(0);
   quit= 0;
   
   initscr();

   if(has_colors() == FALSE){
    printf("La terminal no soporta color");
    endwin();
   }   
  
   cbreak();
   noecho();
   keypad(stdscr,TRUE);
   set_escdelay(0);
   curs_set(0);

   use_default_colors();
   start_color();
   refresh();
   
   getmaxyx(stdscr,my,mx);
   printbar(stdscr,ch);
   printborders(stdscr,1,0);
   menu(stdscr,my,mx);
   vm = createwin(2*my/3-2,mx-28,2,27);
   do{ 
    switch(getch()){ 
    case KEY_RIGHT: if(ch==5) ch=-1;
                    if(ch<5) ch++;
                    deletebar();
                    printbar(stdscr,ch);
                    break;
    case KEY_LEFT:  if(ch==0) ch=6;
                    if(0<ch) ch--;
                    deletebar();
                    printbar(stdscr,ch);
                    break;
    case KEY_RESIZE:getmaxyx(stdscr,my,mx);
                    deletemenu();
                    deletebar();
                    deletewin(vm);
                    erase();
                    printbar(stdscr,ch);
                    printborders(stdscr,1,0);
                    menu(stdscr,my,mx);
                    vm = createwin(2*my/3-2,mx-28,2,27);
                    break;
    case 10:       switch(ch){ 
                      case 0: readfile(stdscr);
                              clear();
                              getmaxyx(stdscr,my,mx);
                              printbar(stdscr,0);
                              printborders(stdscr,1,0);
                              menu(stdscr,my,mx);
                              break;
                      case 1: if(pl){
                              go = 1;
                              while(go){
                              inst[mem[pc]]();
                              pc = pc%(MaxMem+1);
                              deletemenu();
                              menu(stdscr,my,mx);
                              checkbreakpoint();
                              usleep(5000);
                              }
                              }
                              break;
                      case 2: if(pl){
                              go = 1;
                              inst[mem[pc]]();
                              pc = pc%(MaxMem+1);
                              deletemenu();
                              menu(stdscr,my,mx);
                              go = 0;
                              }
                              break;
                      case 3: if(pl){
                              createbreakpoints(cpu);
                              deletemenu();
                              menu(stdscr,my,mx);
                              }
                              break;
                      case 4: help(stdscr);
                              clear();
                              getmaxyx(stdscr,my,mx);
                              printbar(stdscr,4);
                              printborders(stdscr,1,0);
                              menu(stdscr,my,mx);
                              break;
                      case 5: quit=1;
                              break;
                      default:break;
                   }              
    }
   }while(!quit);
   deletebar();
   deletemenu();
   deletewin(vm);
   endwin();
   return 0;
}
