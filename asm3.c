#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

FILE* input;
FILE* output;

int  ch;
#define MAXLINE 255
#define MAXNAME 16

char  line[MAXLINE+1];
int   linenum;
int   linepos;

int   undef;

int   pass;
int   stop;
int   loccnt;

int   error;
int   errbeg=0;


void Error(const char msg[])
{
    static int lastline=0;
    if (pass==2)
      {
       if (lastline != linenum)
           fprintf(stderr, "Error: (L%d:C%d) %s\n", linenum, errbeg, line);
       fprintf(stderr, "Error: (L%d:C%d) %*s", linenum, errbeg, errbeg, "^");
       fprintf(stderr, "%s\n", msg);
       lastline= linenum;
       error= 1;
      }
}

void Missing(const char msg[])
{
    char str[MAXLINE+1];
    sprintf(str, "Missing %s", msg);
    Error(str);
}

void InvalidAM(void)
{
    Error("Invalid addressing mode");
}

int readline(void)
{
    int p= 0;
    int c;

    linepos= 0;
    linenum++;
    while ( (c=fgetc(input)) != EOF && c != '\n' )
      {
       if (p<MAXLINE)
          line[p]= c;
       p++;
      }
    line[p]= '\0';
    ch= ' ';
    if (pass==2)
       fprintf(output,"; %s\n", line);
//    fprintf(stderr, "linea: %d: %s\n", linenum, line);
    return p==0 && c==EOF;
}


int readch(void)
{
    if (line[linepos]=='\0')
       {
        linepos= 0;
        return ch='\n';
       }

    return ch= line[linepos++];
}


typedef struct {
            char  name[MAXNAME+1];
            int   type;
            int   val;
        } Token;

enum { nil, reg, instr, byteop, wordop, endop, label, comment,
       regA, regB, regBP, regSP,
       addsym, subsym, mulsym, divsym, colonsym, sharpsym,
       lbrsym, rbrsym, equsym, number, commasym, lpasym, rpasym, stringsym };

const char *TokenName[] = { "nil", "reg", "instr", "byteop", "wordop", "endop", 
                     "label", "comment", "regA", "regB", "regBP", "regSP",
                     "addsym", "subsym", "mulsym", "divsym", "colonsym", 
                     "sharpsym", "lbrsym", "rbrsym", "equsym", "number", 
                     "commasym", "lpasym", "rpasym", "stringsym" };


Token tok = { "", nil };

Token* SetTok(const char str[], int type, int val)
{
    strcpy(tok.name, str);
    tok.type= type;
    tok.val= val;
/*
    if (pass==2)
           fprintf(stderr, "DEBUG: Token: %s  type=%s val=%04x\n",
                           str, TokenName[type], val);
*/
}
char chr[1024];

typedef struct {
            char  name[MAXNAME+1];
            int   type;
            int   val;
            int   def;
        } Label;

Label Labels[1024];
int   NumLabels= 0;
int   Reserved;
int   idx;

int NewLabel(const char str[], int type, int val, int def)
{
    strcpy(Labels[NumLabels].name, str);
    Labels[NumLabels].type= type;
    Labels[NumLabels].val=  val;
    Labels[NumLabels].def= def;
    NumLabels++;
}

int SetLabel(int idx, int val, int def)
{
    Labels[idx].val= val;
    Labels[idx].def = def;
}


int hex(int c)
{
    if (isdigit(c))
        return c-'0';
    c= tolower(c);
    if ('a'<=c && c<='f')
        return c-'a'+10;
    return -1;
}

int readnumber(int radix)
{
  int num= 0;
  for (;;)
    {
     int r= hex(ch);
     if (0<=r && r<radix)
        num= radix * num + hex(ch);
     else
        return num;
     readch();
    }
}


void ReadToken(void)
{
    while (isspace(ch))
        if (ch=='\n')
            {
             SetTok(";", comment, 0);
             return;
            }
        else
           readch();

    errbeg= linepos;
    if (ch==';')
        {
         while (readch()!='\n')
            ;
         SetTok(";", comment, 0);
         return;
        }

    if (isalpha(ch)|| ch=='_' || ch=='@')
        {
         int k= 0;
         char str[MAXNAME+1];

         do {
             if (k<MAXNAME)
                str[k++]= ch;
             readch();
         } while (isalpha(ch) || isdigit(ch) || ch=='_' || ch=='@');
         str[k]='\0';

         for (k=0; k<Reserved; ++k)
            {
             if (strcasecmp(str, Labels[k].name)==0)
                {
                 idx= k;
                 SetTok(Labels[idx].name, Labels[idx].type, Labels[idx].val);
				 fprintf( stderr,  "var: %s val: %d\n", str, Labels[idx].val);
                 return;
                }
            }

         for (; k<NumLabels; ++k)
            {
             if (strcmp(str, Labels[k].name)==0)
                {
                 idx= k;
                 SetTok(Labels[idx].name, Labels[idx].type, Labels[idx].val);
                 return;
                }
            }

         idx= NumLabels;
         NewLabel(str, label, linenum, 0);
         SetTok(Labels[idx].name, Labels[idx].type, Labels[idx].val);
         return;
        }

    if (ch=='0')
       {
        readch();
        if (ch=='x' || ch=='X')
           {
            readch();
            SetTok("NUM", number, readnumber(16));
           }
        else
           SetTok("NUM", number, readnumber(8));
        return;
       }

    if (isdigit(ch))
       {
        SetTok("NUM", number, readnumber(10));
        return;
       }

    if (ch=='\'')
      {
       int v;
       readch();
 
       if (ch=='\\')
          {
           readch();

           if ('0'<=ch && ch<='7')
              v=readnumber(8);
           else
              {
               switch (ch) {
                  case '\\' :  v= '\\';  break;
                  case 'n'  :  v= '\n';  break;
                  case 't'  :  v= '\t';  break;
                  case 'b'  :  v= '\b';  break;
                  default   :  v= ch;    break;
                 } 
               readch();
              }
          }
       else
          {
           v= ch;   
           readch();
          }

       if (ch=='\'')
          readch();
       else
          Error("missing '");

       SetTok("NUM", number, v);
       return;
      }

    if (ch=='"')
       {
        int n= 0;
        int v= 0;

        readch();
        while (ch!='"')
          {
           if (ch=='\n')
              break;
           if (ch=='\\')
               if ('0' <= readch() && ch <= '7' )
                  v= readnumber(8);
               else
                  {
                   switch (ch) {
                     case '\\' :  v= '\\';  break;
                     case 'n'  :  v= '\n';  break;
                     case 't'  :  v= '\t';  break;
                     case 'b'  :  v= '\b';  break;
                     default   :  v= ch;    break;
                    }
                  readch();
                 }
           else   
               {
                v= ch;
                readch();
               }
           chr[n++]= v; 
          }
        chr[n++]= 0; 
        SetTok("", stringsym, 0); 
        if (ch=='"')
            readch();
        return;
       }

    switch (ch) {
       case '+' : SetTok("+", addsym, 0);  readch();  return;
       case '-' : SetTok("-", subsym, 0);  readch();  return;
       case '*' : SetTok("*", mulsym, 0);  readch();  return;
       case '/' : SetTok("/", divsym, 0);  readch();  return;
       case '[' : SetTok("[", lbrsym, 0);  readch();  return;
       case ']' : SetTok("]", rbrsym, 0);  readch();  return;
       case '(' : SetTok("(", lpasym, 0);  readch();  return;
       case ')' : SetTok(")", rpasym, 0);  readch();  return;
       case '=' : SetTok("=", equsym, 0);  readch();  return;
       case ',' : SetTok(",", commasym, 0);  readch();  return;
       case '#' : SetTok(",", sharpsym, 0);  readch();  return;
       case ':' : SetTok(":", colonsym, 0);  readch();  return;
       default  :  SetTok("", nil, 0);
      }

    Error("Unknown symbol");
}


int Code[MAXLINE];
int CodeSize;


void GenCod(int x)
{
   if (pass==2 && error==0)
      fprintf(output, " 0x%04x 0x%02x\n", loccnt, x&0xFF);
   loccnt++;
}

void GenStart(int st, int sp)
{
   if (pass==2)
      fprintf(output, "< 0x%04x 0x%04x\n", st, sp);
}

void ReportUndef(void)
{
   int k;
   int t= 1;
   for (k=Reserved; k<NumLabels; ++k)
      if (Labels[k].def==0) 
        {
         if (t==1)
            {
             fprintf(stderr, "There are undefined labels\n"
                             "Line  Name\n");
             t= 0;
            }
         fprintf(stderr, "%4d %s\n", Labels[k].val, Labels[k].name);
        }
}

int Expr(void);

int Factor(void)
{
    int f= 0;
    switch (tok.type) {
      case number:
         f= tok.val;
         ReadToken();
         return f;

      case label:
         if (Labels[idx].def == 0)
            {
             Error("Undefined name");
             undef= 1;
             f= 0;
            }
         else
            f= Labels[idx].val;
         ReadToken();
         return f;

      case mulsym:
         f= loccnt;
         ReadToken();
         return f;

      case lpasym:
         ReadToken();
         f= Expr();
         if (tok.type==rpasym)
             ReadToken();
         else
             Missing("')'");
         return f;

      default:
         Missing("<factor>");
         return 0;
    }
}

int Term(void)
{
    int t= Factor();
    int f;
    for (;;)
       switch (tok.type) {
         case mulsym:
            ReadToken();
            t*= Factor();
            break; 
         case divsym:
            ReadToken();
            f= Factor();
            if (f==0)
                Error("Division by 0");
            else
               t/= f;
            break; 
         default:
           return t;
       }
}


int Expr(void)
{
    int e= 0;

    switch(tok.type) {
       case addsym:
           ReadToken();
           e= Term();
           break;
       case subsym:
           ReadToken();
           e= -Term(); 
           break; 
       default:
           e= Term();
      }

    for (;;)
       switch(tok.type) {
          case addsym:
              ReadToken();
              e+= Term();
              break;
          case subsym:
              ReadToken();
              e-= Term(); 
              break; 
          default:
              return e;
         }

}

int Expression(void)
{
   undef= 0;
   return Expr();
}

void Addressing( int ins )
{
   enum { NIL, INHERENT, IMMEDIATE,
          REGA, REGB, REGSP, REGBP,
          REGIB, REGIBP, DIRECT, INDIRECT, REGBPV };

   int mode= NIL;
   int size= 1;
   int p;

   ReadToken();
   switch(tok.type) {
     case comment:
       mode= INHERENT;
       break;

     case sharpsym:                  /* modo inmediato */
       mode= IMMEDIATE;
       size= 3;
       ReadToken();
       p= Expression();
       break;

     case lbrsym:
       ReadToken();
       if (tok.type==regB)           /* modo [b]       */
         {
          ReadToken();
          if (tok.type==rbrsym)
             {
              mode=REGIB;
              ReadToken();
             }
          else
             Missing("']'");
         }
       else
       if (tok.type==regBP)         /* modo [bp+offset] */
         {
          ReadToken();
          if (tok.type==addsym || tok.type==subsym)
             {
              mode= REGIBP;
              size= 3;
              p= Expression();
              if (tok.type==rbrsym)
                 ReadToken();
              else
                 Missing("']'");
             }
          else
             Missing("'+' or '-'");
         }
       else
         {
          mode= INDIRECT;
          p= Expression();     /* [address]  */
          size= 3;
          if (tok.type==rbrsym)
             ReadToken();
          else
             Missing("']'");
         }
       break;

   case regBP:             /* BP */
      mode= REGBP;
      ReadToken();
      if (tok.type==addsym || tok.type==subsym)
         {
          p=Expression();
          mode= REGBPV;
         }
      break;

   case regSP:            /* SP */
      mode= REGSP;
      ReadToken();
      break;

   case regA:
      mode= REGA;
      ReadToken();
      break;

   case regB:             /* B */
      mode= REGB;
      ReadToken();
      break;

   default:                      /* modo directo */
      mode= DIRECT;
      size= 3;
      p= Expression();
	  fprintf(stderr, "irq p=%d\n", p);
      break;
   }


/* ===== segunda fase =======================*/
   switch(ins) {
      case 0x30:                      /* jeq */
      case 0x31:                      /* jne */
      case 0x32:                      /* jle */
      case 0x33:                      /* jgt */
      case 0x34:                      /* jge */
      case 0x35:                      /* jlt */
         if (mode != DIRECT)
            InvalidAM();
         else
            {
             GenCod(ins);
             GenCod(p & 0xFF);
             GenCod(p >> 8);
            }
         break;

      case 0x38:                      /* jmp */
      case 0x3a:                      /* jsr */
         if (mode == DIRECT)
            {
             GenCod(ins);
             GenCod(p & 0xFF);
             GenCod(p >> 8);
            }
         else
         if (mode == REGIB)
            GenCod(ins + 1);
         else
            InvalidAM();
         break;

      case 0x3d:                     /* irq */
         if (mode == DIRECT)
            {
             GenCod(ins);
             GenCod(p & 0xFF);
             GenCod(p >> 8);
            }
          else
             InvalidAM();
          break;


      case 0x40:
         switch (mode) {
             case REGA:  GenCod(ins);
                         break;
             case REGB:  GenCod(ins+1);
                         break;
             default:    InvalidAM();
                         break;
         }
         break;

      case 0x50:                      /* psh */
      case 0x60:                      /* pop */
         switch (mode)
           {
            case REGA:
                  GenCod(ins);
                  break;

            case REGB:
                  GenCod(ins+1);
                  break;

            case REGBP:
                  GenCod(ins+2);
                  break;

            case DIRECT:
                  GenCod(ins+3);
                  GenCod(p & 0xFF);
                  GenCod(p >> 8);
                  break;
            default:
                  InvalidAM();
                  break;
           }
         break;

      case 0x42:                      /* lda */
         switch (mode) {
           case IMMEDIATE:
             GenCod(ins);
             GenCod(p & 0xFF);
             GenCod(p >> 8);
             break;

           case INDIRECT:
             GenCod(ins+1);
             GenCod(p & 0xFF);
             GenCod(p >> 8);
             break;

           case REGIB:
             GenCod(ins+2);
             break;

           case REGIBP:
             GenCod(ins+3);
             GenCod(p & 0xFF);
             GenCod(p >> 8);
             break;

           default:
            InvalidAM();
            break;
          }
          break;

      case 0x4a:                      /* sta */
         switch (mode) {
           case INDIRECT:
             GenCod(ins+1);
             GenCod(p & 0xFF);
             GenCod(p >> 8);
             break;

           case REGIB:
             GenCod(ins+2);
             break;

           case REGIBP:
             GenCod(ins+3);
             GenCod(p & 0xFF);
             GenCod(p >> 8);
             break;

           default:
             InvalidAM();
          }
         break;

      case 0x78:                      /* lsp */
         switch (mode) {
           case REGB:
             GenCod(ins);
             break;

         case REGBP:
            GenCod(ins+1);
            break;

         case IMMEDIATE:
            GenCod(ins+2);
            GenCod(p & 0xFF);
            GenCod(p >> 8);
            break;
             
         default:
            InvalidAM();
        }
      break;

      case 0x70:                      /* lbp */
         switch (mode) {
           case REGB:
            GenCod(ins);
            break;

         case REGSP:
            GenCod(ins+1);
            break;
 
         case IMMEDIATE:
            GenCod(ins+2);
            GenCod(p & 0xFF);
            GenCod(p >> 8);
            break;
             
         default:
            InvalidAM();
        }
      break;

      case 0x80:                      /* lea */
       switch (mode) {
         case DIRECT:
           GenCod(ins);
           GenCod(p & 0xFF);
           GenCod(p >> 8);
           break;

         case REGBPV:
           GenCod(ins+1);
           GenCod(p & 0xFF);
           GenCod(p >> 8);
           break;

         case REGIBP:
           GenCod(ins+2);
           GenCod(p & 0xFF);
           GenCod(p >> 8);
           break;

        default:
          InvalidAM();
           break;
       }
       break;

      default:
        if (mode==INHERENT)
           GenCod(ins);
        else
           InvalidAM();
     }
}

void Line(void)
{
    int expr;
    int lab;

    error= 0;
    ReadToken();

    if (tok.type==label)
        {
         lab= idx;
         ReadToken();
         if (tok.type==colonsym)
            {
             SetLabel(lab, loccnt, linenum);
             ReadToken();
            }
         else
         if (tok.type==equsym)
            {
             ReadToken();
             SetLabel(lab, Expression(), linenum);
            }
         else
            Error("Bad label definition");
        }

    switch (tok.type)
      {
       case mulsym:
         ReadToken();
         if (tok.type==equsym)
            {
             ReadToken();
             loccnt= Expression();
            }
         else
            Missing("'='" );
         break;

       case byteop:
           ReadToken();
           if (tok.type==stringsym)
              {
               int p= 0;
               do {
                  GenCod(chr[p]);
               } while (chr[p++]);
               ReadToken();
              }
           else
              GenCod(Expression());

           while (tok.type==commasym)
              {
               ReadToken();
               if (tok.type==stringsym)
                  {
                   int p=0;
                   do
                      GenCod(chr[p]);
                   while(chr[p++]);
                   ReadToken();
                  }
               else
                  GenCod(Expression());
              }
           break;

       case wordop:
           ReadToken();
           expr= Expression();
           GenCod(expr);
           GenCod(expr >> 8);
           while (tok.type==commasym)
              {
               ReadToken();
               expr= Expression();
               GenCod(expr);
               GenCod(expr >> 8);
              }
           break;

       case endop:
           ReadToken();
           if (tok.type==label)
               {
                int st= tok.val;
                int sp;
                ReadToken();
                if (tok.type==commasym)
                   {
                    ReadToken();
                    if (tok.type==label)
                       {
                        sp= tok.val;
                        ReadToken();
                       }
                    else
                       sp= loccnt;
                   }
                else
                   sp= loccnt;
                GenStart(st, sp);
                stop= 1;
               }
           break;

       case instr:
          Addressing(Labels[idx].val);
          break;

       case comment:
          break;

       default:
          Missing("instruction or pseudo operation");
          return; 
    }

    if (tok.type!=comment)
        Error("Comment expected");
}



void Pass(int n)
{
    pass= n;
    linenum= 0;
    loccnt= 0;
    stop= 0;
    while (readline()==0 && stop==0)
       Line();
}


int main(int narg, char* args[])
{
   if (narg > 1)
      if (strcmp(args[1],"-")==0)
         input= stdin;
      else
         {
          input= fopen(args[1],"rt");
          if (input==NULL)
             {
              fprintf(stderr, "Cannot open input file %s\n", args[1]);
              exit(1);
             }
         }
   else
     input= stdin;

   if (narg > 2)
      if (strcmp(args[2],"-")==0)
         output= stdout;
      else
         {
          output= fopen(args[2],"wt");
          if (output==NULL)
             {
              fprintf(stderr, "Cannot open output file %s\n", args[2]);
              exit(1);
             }
         }
   else
     output= stdout;

   NumLabels= 0;
   NewLabel( "byte", byteop, 0, 0 );
   NewLabel( "word", wordop, 0, 0 );
   NewLabel( "end",  endop,  0, 0 );

   NewLabel( "a",   regA, 0, 0 );
   NewLabel( "b",   regB, 0, 0 );
   NewLabel( "sp",  regSP, 0, 0 );
   NewLabel( "bp",  regBP, 0, 0 );

   NewLabel( "nop", instr, 0x00, 0 );
   NewLabel( "add", instr, 0x10, 0 );
   NewLabel( "sub", instr, 0x11, 0 );
   NewLabel( "mul", instr, 0x12, 0 );
   NewLabel( "div", instr, 0x13, 0 );
   NewLabel( "mod", instr, 0x14, 0 );
   NewLabel( "cmp", instr, 0x15, 0 );
   NewLabel( "shl", instr, 0x16, 0 );
   NewLabel( "shr", instr, 0x17, 0 );

   NewLabel( "neg", instr, 0x18, 0 );
   NewLabel( "inc", instr, 0x19, 0 );
   NewLabel( "dec", instr, 0x1a, 0 );
   NewLabel( "xch", instr, 0x1b, 0);

   NewLabel( "and", instr, 0x20, 0 );
   NewLabel( "or",  instr, 0x21, 0 );
   NewLabel( "xor", instr, 0x22, 0 );
   NewLabel( "not", instr, 0x23, 0 );
   NewLabel( "tst", instr, 0x24, 0 );

   NewLabel( "jeq", instr, 0x30, 0 );
   NewLabel( "jne", instr, 0x31, 0 );
   NewLabel( "jle", instr, 0x32, 0 );
   NewLabel( "jgt", instr, 0x33, 0 );
   NewLabel( "jge", instr, 0x34, 0 );
   NewLabel( "jlt", instr, 0x35, 0 );

   NewLabel( "jmp", instr, 0x38, 0 );
   NewLabel( "jsr", instr, 0x3a, 0 );

   NewLabel( "rtn", instr, 0x3c, 0 );
   NewLabel( "irq", instr, 0x3d, 0 );

   NewLabel( "clr", instr, 0x40, 0 );
   NewLabel( "lda", instr, 0x42, 0 );
   NewLabel( "sta", instr, 0x4a, 0 );

   NewLabel( "psh", instr, 0x50, 0 );

   NewLabel( "pop", instr, 0x60, 0 );

   NewLabel( "lbp", instr, 0x70, 0 );
   NewLabel( "lsp", instr, 0x78, 0 );

   NewLabel( "lea", instr, 0x80, 0 );
   NewLabel( "adb", instr, 0x88, 0 );

   NewLabel( "vm_hlt", label, 0x00, 1);
   NewLabel( "pr_int", label, 0x01, 1);
   NewLabel( "pr_str", label, 0x02, 1);
   NewLabel( "rd_int", label, 0x03, 1);
   NewLabel( "pr_chr", label, 0x05, 1);
   NewLabel( "switch", label, 0x07, 1);

   Reserved= NumLabels;

   Pass(1);
   rewind(input);
   Pass(2);
   ReportUndef();
   fclose(input);
   fclose(output);

   return 0;
}
