/*  Eric Tu and Joshua Eckels
	CSSE332 - BareMetalOS
	Team 2-4
*/

/* Prototypes */
void printString(char*);
void readString(char*);
int printStringFirstLine(char*);
int len(char*);
int mod(int, int);
int div(int, int);
int readSector(char*, int);
void handleInterrupt21(int,int,int,int);

int main() {
	char line[80];
	char buffer[512];

	/* Main task 1 */
	printStringFirstLine("Hello World"); /* this uses raw memory commands; see function below */
	
	/* Main task 2 */
	makeInterrupt21();
	interrupt(0x21,0,"This uses printString()",0,0);
	interrupt(0x21,0,"\n",0,0);
	
	/* Main task 3 */
	interrupt(0x21,0,"Choose your fate: ",0,0);
	interrupt(0x21,1,line,0,0);

	/* Main task 4 */
	interrupt(0x21,0,line,0,0);

	/* Main task 5 */
	/*
	Be sure to run this on command line before running the floppya.img:
	dd if=message.txt of=floppya.img bs=512 count=1 seek=30 conv=notrunc
	*/
	interrupt(0x21,2,buffer, 30,0);
	interrupt(0x21,0,buffer,0,0);
	while(1);
}

void handleInterrupt21(int ax, int bx, int cx, int dx) {
	if (ax == 0) {
		printString(bx);
	} else if (ax == 1) {
		readString(bx);
	} else if (ax == 2) {
		readSector(bx, cx);
	} else {
		printString("Error occurred.\n");
	}
}

int readSector(char *buffer, int sector) {
	int ah;
	int al; /* num of sectors to read */
	int ch;
	int cl;
	int dh;
	int dl;
	int ax;
	int bx;
	int cx;
	int dx;

	ah = 2;
	al = 1;
	ch = div(sector, 36);
	cl = mod(sector, 18) + 1;
	dh = mod(div(sector,18),2);
	dl = 0;
	ax = ah*256 + al;
	bx = buffer;
	cx = ch*256 + cl;
	dx = dh*256 + dl;

	interrupt(0x13,ax, bx, cx, dx);
}

int mod(int a, int b)
{
  int temp;
  temp = a;
  while (temp >= b) {
    temp = temp-b;
  }
  return temp;
}

int div(int a, int b)
{
  int quotient;
  quotient = 0;
  while ((quotient + 1) * b <= a) {
    quotient++;
  }
  return quotient;
}

int printStringFirstLine(char* msg) {
	int addr = 0x8000;
	int seg = 0xB * 0x1000;
	int j = 0;
	int i;
	int color = 0x7;

	for (i = 0; i < len(msg); i++) {
		putInMemory(seg,addr + j++, msg[i]);
		putInMemory(seg,addr + j++, color);
	}
}

/* uses a cursor to print a msg */
void printString(char *msg) {
	int i;
	char ah = 0xe;
	for (i = 0; i < len(msg); i++) {
		char currChar = msg[i];
		int ax = ah* 256 + currChar;
		
		interrupt(0x10,ax,0,0,0);
	}
}

/*  Reads a line from the console using Interrupt 0x16. */
void readString(char *line)
{
  int i, lineLength, ax;
  char charRead, backSpace, enter;
  lineLength = 80;
  i = 0;
  ax = 0;
  backSpace = 0x8;
  enter = 0xd;
  charRead = interrupt(0x16, ax, 0, 0, 0);
  while (charRead != enter && i < lineLength-2) {
    if (charRead != backSpace) {
      interrupt(0x10, 0xe*256+charRead, 0, 0, 0);
      line[i] = charRead;
      i++;
    } else {
      i--;
      if (i >= 0) {
		interrupt(0x10, 0xe*256+charRead, 0, 0, 0);
		interrupt(0x10, 0xe*256+'\0', 0, 0, 0);
		interrupt(0x10, 0xe*256+backSpace, 0, 0, 0);
      }
      else {
	i = 0;
      }
    }
    charRead = interrupt(0x16, ax, 0, 0, 0);  
  }
  line[i] = 0xa;
  line[i+1] = 0x0;
  
  /* correctly prints a newline */
  printString("\r\n");

  return;
}

/* Returns the length of a string */
int len(char* msg) {
	int count = 0;
	int k;
	for (k = 0; msg[k] != '\0'; k++) {
		count++;
	}
	return count;
}
