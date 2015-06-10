/* SEXP implementation code sexp-output.c
 * Ron Rivest
 * 5/5/1997
 */

#include <stdio.h>
#include <malloc.h>
#include "sexp.h"

static char *hexDigits = "0123456789ABCDEF";

static char *base64Digits = 
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/***********************/
/* SEXP Output Streams */
/***********************/

/* putChar(os,c)
 * Puts the character c out on the output stream os.
 * Keeps track of the "column" the next output char will go to.
 */
void putChar(os,c)
sexpOutputStream *os;
int c;
{
  putc(c,os->outputFile);
  os->column++;
}

/* varPutChar(os,c)
 * putChar with variable sized output bytes considered.
 */
void varPutChar(os,c)
sexpOutputStream *os;
int c;      /* this is always an eight-bit byte being output */
{
  c &= 0xFF;
  os->bits = (os->bits << 8) | c;
  os->nBits += 8;
  while (os->nBits >= os->byteSize)
    { 
      if ((os->byteSize==6 || os->byteSize==4 
	   || c == '}' || c == '{' || c == '#' || c == '|')
	  && os->maxcolumn > 0 
	  && os->column >= os->maxcolumn)
	os->newLine(os,os->mode);
      if (os->byteSize == 4)
	os->putChar(os,
		    hexDigits[(os->bits >> (os->nBits-4)) & 0x0F]);
      else if (os->byteSize == 6)
	os->putChar(os,
		    base64Digits[(os->bits >> (os->nBits-6)) & 0x3F]);
      else if (os->byteSize == 8)
	os->putChar(os,os->bits & 0xFF);
      os->nBits -= os->byteSize;
      os->base64Count++;
    }
}

/* changeOutputByteSize(os,newByteSize,mode)
 * Change os->byteSize to newByteSize
 * record mode in output stream for automatic line breaks
 */
void changeOutputByteSize(os,newByteSize,mode)
sexpOutputStream *os;
int newByteSize;
int mode;
{ 
  if (newByteSize != 4 && newByteSize !=6 && newByteSize !=8)
    ErrorMessage(ERROR,"Illegal output base %d.",newByteSize,0);
  if (newByteSize !=8 && os->byteSize != 8)
    ErrorMessage(ERROR,"Illegal change of output byte size from %d to %d.",
		 os->byteSize,newByteSize);
  os->byteSize = newByteSize;
  os->nBits = 0;
  os->bits = 0;
  os->base64Count = 0;
  os->mode = mode;
}

/* flushOutput(os)
 * flush out any remaining bits 
 */
void flushOutput(os)
sexpOutputStream *os;
{ if (os->nBits > 0)
    { 
      if (os->byteSize == 4)
	os->putChar(os,hexDigits[(os->bits << (4 - os->nBits)) & 0x0F]);
      else if (os->byteSize == 6)
	os->putChar(os,base64Digits[(os->bits << (6 - os->nBits)) & 0x3F]);
      else if (os->byteSize == 8)
	os->putChar(os,os->bits & 0xFF);
      os->nBits = 0;
      os->base64Count++;
    }
  if (os->byteSize == 6) /* and add switch here */
    while ((os->base64Count & 3) != 0) 
      { if (os->maxcolumn > 0 && os->column >= os->maxcolumn)
	  os->newLine(os,os->mode);
	os->putChar(os,'=');
	os->base64Count++;
      }
}

/* newLine(os,mode)
 * Outputs a newline symbol to the output stream os.
 * For ADVANCED mode, also outputs indentation as one blank per 
 * indentation level (but never indents more than half of maxcolumn).
 * Resets column for next output character.
 */
void newLine(os,mode)
sexpOutputStream *os;
int mode;
{ int i;
  if (mode == ADVANCED || mode == BASE64)
    { os->putChar(os,'\n');
      os->column = 0;
    }
  if (mode == ADVANCED)
    for (i=0;i<os->indent&&(4*i)<os->maxcolumn;i++) 
      os->putChar(os,' ');
}

/* newSexpOutputStream()
 * Creates and initializes new sexpOutputStream object.
 */
sexpOutputStream *newSexpOutputStream()
{
  sexpOutputStream *os;
  os = (sexpOutputStream *) sexpAlloc(sizeof(sexpOutputStream));
  os->column = 0;
  os->maxcolumn = DEFAULTLINELENGTH;
  os->indent = 0;
  os->putChar = putChar;
  os->newLine = newLine;
  os->byteSize = 8;
  os->bits = 0;
  os->nBits = 0;
  os->outputFile = stdout;
  os->mode = CANONICAL;
  return(os);
}

/*******************/
/* OUTPUT ROUTINES */
/*******************/

/* printDecimal(os,n)
 * print out n in decimal to output stream os
 */
void printDecimal(os,n)
sexpOutputStream *os;
long int n;
{ char buffer[50];
  int i;
  sprintf(buffer,"%ld",n);
  for (i=0;buffer[i]!=0;i++)
    varPutChar(os,buffer[i]);
}

/********************/
/* CANONICAL OUTPUT */
/********************/

/* canonicalPrintVerbatimSimpleString(os,ss)
 * Print out simple string ss on output stream os as verbatim string.
 */
void canonicalPrintVerbatimSimpleString(os,ss)
sexpOutputStream *os;
sexpSimpleString *ss;
{ 
  long int len;
  long int i;
  octet *c;
  len = simpleStringLength(ss);
  c = simpleStringString(ss);
  if (c == NULL) 
    ErrorMessage(ERROR,"Can't print NULL string verbatim",0,0);
  /* print out len: */
  printDecimal(os,len);
  varPutChar(os,':');
  /* print characters in fragment */
  for (i=0;i<len;i++) varPutChar(os,(int)*c++);
}

/* canonicalPrintString(os,s)
 * Prints out sexp string s onto output stream os
 */
void canonicalPrintString(os,s)
sexpOutputStream *os;
sexpString *s;
{ sexpSimpleString *ph, *ss;
  ph = sexpStringPresentationHint(s);
  if (ph != NULL) 
    { varPutChar(os,'[');
      canonicalPrintVerbatimSimpleString(os,ph);
      varPutChar(os,']');
    }
  ss = sexpStringString(s);
  if (ss == NULL)
    ErrorMessage(ERROR,"NULL string can't be printed.",0,0);
  canonicalPrintVerbatimSimpleString(os,ss);
}

/* canonicalPrintList(os,list)
 * Prints out the list "list" onto output stream os
 */
void canonicalPrintList(os,list)
sexpOutputStream *os;
sexpList *list;
{ sexpIter *iter;
  sexpObject *object;
  varPutChar(os,'(');
  iter = sexpListIter(list);
  while (iter != NULL)
    { object = sexpIterObject(iter);
      if (object != NULL) 
	  canonicalPrintObject(os,object);
      iter = sexpIterNext(iter);
    }
  varPutChar(os,')');
}

/* canonicalPrintObject(os,object)
 * Prints out object on output stream os
 * Note that this uses the common "type" field of lists and strings.
 */
void canonicalPrintObject(os,object)
sexpOutputStream *os;
sexpObject *object;
{
  if (isObjectString(object))
    canonicalPrintString(os,(sexpString *)object);
  else if (isObjectList(object))
    canonicalPrintList(os,(sexpList *)object);
  else ErrorMessage(ERROR,"NULL object can't be printed.",0,0);
}

/* *************/
/* BASE64 MODE */
/* *************/
/* Same as canonical, except all characters get put out as base 64 ones */

void base64PrintWholeObject(os,object)
sexpOutputStream *os;
sexpObject *object;
{
  changeOutputByteSize(os,8,BASE64);
  varPutChar(os,'{');
  changeOutputByteSize(os,6,BASE64);
  canonicalPrintObject(os,object);
  flushOutput(os);
  changeOutputByteSize(os,8,BASE64);
  varPutChar(os,'}');
}

/*****************/
/* ADVANCED MODE */
/*****************/

/* TOKEN */

/* canPrintAsToken(ss)
 * Returns TRUE if simple string ss can be printed as a token.
 * Doesn't begin with a digit, and all characters are tokenchars.
 */
int canPrintAsToken(os,ss)
sexpOutputStream *os;
sexpSimpleString *ss;
{
  int i;
  octet *c;
  long int len;
  len = simpleStringLength(ss);
  c = simpleStringString(ss);
  if (len <= 0) return(FALSE);
  if (isDecDigit((int)*c)) return(FALSE);
  if (os->maxcolumn > 0 && os->column + len >= os->maxcolumn)
    return(FALSE);
  for (i=0;i<len;i++)
    if (!isTokenChar((int)(*c++))) return(FALSE);
  return(TRUE);
}

/* advancedPrintTokenSimpleString(os,ss)
 * Prints out simple string ss as a token (assumes that this is OK).
 * May run over max-column, but there is no fragmentation allowed...
 */
void advancedPrintTokenSimpleString(os,ss)
sexpOutputStream *os;
sexpSimpleString *ss;
{ int i;
  long int len;
  octet *c;
  len = simpleStringLength(ss);
  if (os->maxcolumn>0 && os->column > (os->maxcolumn - len))
    os->newLine(os,ADVANCED);
  c = simpleStringString(ss);
  for (i=0;i<len;i++) 
    os->putChar(os,(int)(*c++));
}

/* advancedLengthSimpleStringToken(ss)
 * Returns length for printing simple string ss as a token 
 */
int advancedLengthSimpleStringToken(ss)
sexpSimpleString *ss;
{ return(simpleStringLength(ss)); }


/* VERBATIM */ 

/* advancedPrintVerbatimSimpleString(os,ss)
 * Print out simple string ss on output stream os as verbatim string.
 * Again, can't fragment string, so max-column is just a suggestion...
 */
void advancedPrintVerbatimSimpleString(os,ss)
sexpOutputStream *os;
sexpSimpleString *ss;
{ 
  long int len = simpleStringLength(ss);
  long int i;
  octet *c;
  c = simpleStringString(ss);
  if (c == NULL) 
    ErrorMessage(ERROR,"Can't print NULL string verbatim",0,0);
  if (os->maxcolumn>0 && os->column > (os->maxcolumn - len))
    os->newLine(os,ADVANCED);
  printDecimal(os,len);
  os->putChar(os,':');
  for (i=0;i<len;i++) os->putChar(os,(int)*c++);
}

/* advancedLengthSimpleStringVerbatim(ss)
 * Returns length for printing simple string ss in verbatim mode
 */
int advancedLengthSimpleStringVerbatim(ss)
sexpSimpleString *ss;
{ long int len = simpleStringLength(ss);
  int i = 1;
  while (len > 9L) { i++; len = len / 10; }
  return(i+1+len);
}

/* BASE64 */

/* advancedPrintBase64SimpleString(os,ss)
 * Prints out simple string ss as a base64 value.
 */
void advancedPrintBase64SimpleString(os,ss)
sexpOutputStream *os;
sexpSimpleString *ss;
{
  long int i,len;
  octet *c = simpleStringString(ss);
  len = simpleStringLength(ss);
  if (c == NULL) 
    ErrorMessage(ERROR,"Can't print NULL string base 64",0,0);
  varPutChar(os,'|');
  changeOutputByteSize(os,6,ADVANCED);
  for (i=0;i<len;i++) 
    varPutChar(os,(int)(*c++));
  flushOutput(os);
  changeOutputByteSize(os,8,ADVANCED);
  varPutChar(os,'|');
}

/* HEXADECIMAL */

/* advancedPrintHexSimpleString(os,ss)
 * Prints out simple string ss as a hexadecimal value.
 */
void advancedPrintHexSimpleString(os,ss)
sexpOutputStream *os;
sexpSimpleString *ss;
{
  long int i,len;
  octet *c = simpleStringString(ss);
  len = simpleStringLength(ss);
  if (c == NULL) 
    ErrorMessage(ERROR,"Can't print NULL string hexadecimal",0,0);
  os->putChar(os,'#');
  changeOutputByteSize(os,4,ADVANCED);
  for (i=0;i<len;i++) 
    varPutChar(os,(int)(*c++));
  flushOutput(os);
  changeOutputByteSize(os,8,ADVANCED);
  os->putChar(os,'#');
}

/* advancedLengthSimpleStringHexadecimal(ss)
 * Returns length for printing simple string ss in hexadecimal mode
 */
int advancedLengthSimpleStringHexadecimal(ss)
sexpSimpleString *ss;
{ long int len = simpleStringLength(ss);
  return(1+2*len+1);
}

/* QUOTED STRING */

/* canPrintAsQuotedString(ss)
 * Returns TRUE if simple string ss can be printed as a quoted string.
 * Must have only tokenchars and blanks.
 */
int canPrintAsQuotedString(ss)
sexpSimpleString *ss;
{
  long int i, len;
  octet *c = simpleStringString(ss);
  len = simpleStringLength(ss);
  if (len < 0) return(FALSE);
  for (i=0;i<len;i++,c++)
    if (!isTokenChar((int)(*c)) && *c != ' ') 
      return(FALSE);
  return(TRUE);
}

/* advancedPrintQuotedStringSimpleString(os,ss)
 * Prints out simple string ss as a quoted string 
 * This code assumes that all characters are tokenchars and blanks,
 *  so no escape sequences need to be generated.
 * May run over max-column, but there is no fragmentation allowed...
 */
void advancedPrintQuotedStringSimpleString(os,ss)
sexpOutputStream *os;
sexpSimpleString *ss;
{ long int i;
  long int len = simpleStringLength(ss);
  octet *c = simpleStringString(ss);
  os->putChar(os,'\"');
  for (i=0;i<len;i++) 
    { if ( os->maxcolumn>0 && os->column >= os->maxcolumn-2 )
	{
	  os->putChar(os,'\\');
	  os->putChar(os,'\n');
	  os->column = 0;
	}
      os->putChar(os,*c++);
    }
  os->putChar(os,'\"');
}

/* advancedLengthSimpleStringQuotedString(ss)
 * Returns length for printing simple string ss in quoted-string mode
 */
int advancedLengthSimpleStringQuotedString(ss)
sexpSimpleString *ss;
{ long int len = simpleStringLength(ss);
  return(1+len+1);
}

/* SIMPLE STRING */

/* advancedPrintSimpleString(os,ss)
 * Prints out simple string ss onto output stream ss
 */
void advancedPrintSimpleString(os,ss)
sexpOutputStream *os;
sexpSimpleString *ss;
{ long int len = simpleStringLength(ss);
  if (canPrintAsToken(os,ss)) 
    advancedPrintTokenSimpleString(os,ss);
  else if (canPrintAsQuotedString(ss))
    advancedPrintQuotedStringSimpleString(os,ss);
  else if (len <= 4 && os->byteSize == 8) 
    advancedPrintHexSimpleString(os,ss);
  else if (os->byteSize == 8)
    advancedPrintBase64SimpleString(os,ss);
  else 
    ErrorMessage(ERROR,"Can't print advanced mode with restricted output character set.",0,0);
}

/* advancedPrintString(os,s)
 * Prints out sexp string s onto output stream os
 */
void advancedPrintString(os,s)
sexpOutputStream *os;
sexpString *s;
{
  sexpSimpleString *ph = sexpStringPresentationHint(s);
  sexpSimpleString *ss = sexpStringString(s);
  if (ph != NULL) 
    { os->putChar(os,'[');
      advancedPrintSimpleString(os,ph);
      os->putChar(os,']');
    }
  if (ss == NULL)
    ErrorMessage(ERROR,"NULL string can't be printed.",0,0);
  advancedPrintSimpleString(os,ss);
}

/* advancedLengthSimpleStringBase64(ss)
 * Returns length for printing simple string ss as a base64 string
 */
int advancedLengthSimpleStringBase64(ss)
sexpSimpleString *ss;
{ return(2+4*((simpleStringLength(ss)+2)/3)); }

/* advancedLengthSimpleString(os,ss)
 * Returns length of printed image of s
 */
int advancedLengthSimpleString(os,ss)
sexpOutputStream *os;
sexpSimpleString *ss;
{ long int len = simpleStringLength(ss);
  if (canPrintAsToken(os,ss)) 
    return(advancedLengthSimpleStringToken(ss));
  else if (canPrintAsQuotedString(ss))
    return(advancedLengthSimpleStringQuotedString(ss));
  else if (len <= 4 && os->byteSize == 8) 
    return(advancedLengthSimpleStringHexadecimal(ss));
  else if (os->byteSize == 8)
    return(advancedLengthSimpleStringBase64(ss));
  else 
    return(0);  /* an error condition */
}

/* advancedLengthString(os,s)
 * Returns length of printed image of string s
 */
int advancedLengthString(os,s)
sexpOutputStream *os;
sexpString *s;
{ int len = 0;
  sexpSimpleString *ph = sexpStringPresentationHint(s);
  sexpSimpleString *ss = sexpStringString(s);
  if (ph != NULL) 
    len += 2+advancedLengthSimpleString(os,ph);
  if (ss != NULL)
    len += advancedLengthSimpleString(os,ss);
  return(len);
}

/* advancedLengthList(os,list)
 * Returns length of printed image of list given as iterator
 */
int advancedLengthList(os,list)
sexpOutputStream *os;
sexpList *list;
{ int len = 1;                       /* for left paren */
  sexpIter *iter;
  sexpObject *object;
  iter = sexpListIter(list);
  while (iter != NULL)
    { object = sexpIterObject(iter);
      if (object != NULL)
	{ if (isObjectString(object))
	    len += advancedLengthString(os,((sexpString *)object));
	  /* else */ 
	  if (isObjectList(object))
	    len += advancedLengthList(os,((sexpList *)object));
	  len++;                     /* for space after item */
	}
      iter = sexpIterNext(iter);
    }
  return(len+1);  /* for final paren */
}

/* advancedPrintList(os,list)
 * Prints out the list "list" onto output stream os.
 * Uses print-length to determine length of the image.  If it all fits
 * on the current line, then it is printed that way.  Otherwise, it is
 * written out in "vertical" mode, with items of the list starting in
 * the same column on successive lines.
 */
void advancedPrintList(os,list)
sexpOutputStream *os;
sexpList *list;
{ int vertical = FALSE;
  int firstelement = TRUE;
  sexpIter *iter;
  sexpObject *object;
  os->putChar(os,'(');
  os->indent++;
  if (advancedLengthList(os,list) > os->maxcolumn - os->column)
    vertical = TRUE;
  iter = sexpListIter(list);
  while (iter != NULL)
    { object = sexpIterObject(iter);
      if (object != NULL) 
	{ if (!firstelement)
	    { if (vertical) os->newLine(os,ADVANCED);
	      else          os->putChar(os,' ');
	    }
	  advancedPrintObject(os,object);
	}
      iter = sexpIterNext(iter);
      firstelement = FALSE;
    }
  if (os->maxcolumn>0 && os->column>os->maxcolumn-2)
    os->newLine(os,ADVANCED);
  os->indent--;
  os->putChar(os,')');
}

/* advancedPrintObject(os,object)
 * Prints out object on output stream os 
 */
void advancedPrintObject(os,object)
sexpOutputStream *os;
sexpObject *object;
{
  if (os->maxcolumn>0 && os->column>os->maxcolumn-4)
    os->newLine(os,ADVANCED);
  if (isObjectString(object))
    advancedPrintString(os,(sexpString *)object);
  else if (isObjectList(object))
    advancedPrintList(os,(sexpList *)object);
  else 
    ErrorMessage(ERROR,"NULL object can't be printed.",0,0);
}


