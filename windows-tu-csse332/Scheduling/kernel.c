/*  Eric Tu, Joshua Eckels, Jaxon Hoffman
	CSSE332 - BareMetalOS
	Team 2-4
*/
#define MAX_FILE_SIZE 13312
#define MAX_PROCESS 8
/* Prototypes */
void printString(char*);
void readString(char*);
int printStringFirstLine(char*);
int len(char*);
int mod(int, int);
int div(int, int);
int readSector(char*, int);
int writeSector(char*, int);
void handleInterrupt21(int,int,int,int);
void readFile(char*, char*);
void writeFile(char*, char*, int);
void printNumber(short);
void executeProgram(char*);
void terminateProgram(void);
void handleTimerInterrupt(int, int);
void killProcess(int);
void wait(char*);

struct process {
	int isActive;
	int sp;
	int waitingOn;
	int isWaiting;
};

struct process processTable[MAX_PROCESS];
int currentProcess;

int main() {
	char buffer[MAX_FILE_SIZE];  /* this is the maximum size of a file */
	int i;
	for(i = 0; i < MAX_PROCESS; i++){
		processTable[i].isActive = 0;
		processTable[i].sp = 0xff00;
		processTable[i].waitingOn = -1;
		processTable[i].isWaiting = 0;
	}
	currentProcess = -1;

	makeInterrupt21();
	interrupt(0x21, 4, "shell\0", 0, 0);
	/*interrupt(0x21, 4, "phello\0",0,0);
	interrupt(0x21, 4, "tstpr2\0", 0, 0);
	*/
	makeTimerInterrupt();
	while(1);
}

void handleTimerInterrupt(int segment, int sp){
	int i;
	char buf[3];
	if (currentProcess == -1) {
		currentProcess++;
		returnFromTimer(0x2000,processTable[0].sp);
	}

/* Debugging stuff -- ignore */
/*
	printNumber(processTable[0].isActive);
	buf[0] = ' ';
	buf[1] = '\0';
	printString(buf);
	printNumber(processTable[1].isActive);
	printString(buf);
	printNumber(processTable[2].isActive);
	buf[0] = '\n';
	buf[1] = '\r';
	buf[2] = '\0';
	printString(buf);
*/	

	i = mod(currentProcess+1,MAX_PROCESS);
	processTable[currentProcess].sp = sp;
	while (1) {
		if (processTable[i].isActive && !processTable[i].isWaiting) {
			currentProcess = i;
			break;
		}
		i = mod(i+1,MAX_PROCESS);
	}
	returnFromTimer((currentProcess*0x1000) + 0x2000, processTable[currentProcess].sp);
}

void handleInterrupt21(int ax, int bx, int cx, int dx) {
	if (ax == 0) {
		printString(bx);
	} else if (ax == 1) {
		readString(bx);
	} else if (ax == 2) {
		readSector(bx, cx);
	} else if (ax == 3) {
		readFile(bx,cx);
	} else if (ax == 4){
		executeProgram(bx);
	} else if (ax == 5) {
		terminateProgram();
	} else if (ax == 6) {
		writeSector(bx, cx);
	} else if (ax == 8) {
		writeFile(bx, cx, dx);
	} else if (ax == 9) {
		killProcess(bx);
	} else if (ax == 10) {
		wait(bx);	
	}else {
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

int writeSector(char* buffer, int sector) {
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

	ah = 3;
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

void readFile(char* filename, char* file_buffer) {
	int i;
	int j;
	int k;
	int is_match;
	int num_files;
	int name_size;
	char dir_buffer[512];
	num_files = 512/32; /* 16 */
	name_size = 6;
	readSector(dir_buffer,2);
	for (i = 0; i < 32*num_files; i += 32) {
		is_match = 1;
		j = 0;
		while (filename[j] != '\0' && j < 6) {
		/*for (j = 0; j < name_size; j++) { */
			is_match &= (dir_buffer[i+j] == filename[j]);
			j++;
		}
		j = 0;

		if (is_match) {
			/* load file into buffer */
			for (k = 6; k < 32; k++) {
				if (dir_buffer[i+k] == 0) {
					return;
				}
				readSector(file_buffer,dir_buffer[i+k]);
				file_buffer +=  512;
			}	
			return;
		}
	}
	return;
}

void writeFile(char* filename, char* file_contents, int num_sectors) {
	char map_bfr[512];
	char dir_bfr[512];
	char file_bfr[512];
	int free_dir_i,map_i,j,k, sector_count;
	
	for (k = 0; k < 512; k++) {
		map_bfr[k] = '\0';
		dir_bfr[k] = '\0';
		file_bfr[k] = '\0';
	}

	readSector(map_bfr, 1);
	readSector(dir_bfr, 2);
	sector_count = 0;
	j = 0;
	/* check for open sectors */
	for (k = 0; k < 512; k++) {
		if (map_bfr[k] == 0) {
			sector_count++;
		}
	}

	if (num_sectors > sector_count) { return; }

	/* Mark the first free spot in directory*/
	for (free_dir_i = 0;free_dir_i < 512; free_dir_i+= 32) {
		if (dir_bfr[free_dir_i] == 0x00) {
			for (k = 0; k < 32; k++) {
				dir_bfr[free_dir_i + k] = 0;
			}

			j = 0;
			while (filename[j] != '\0' && j < 6) {
				dir_bfr[free_dir_i + j] = filename[j];
				j++;
			}

			break;
		}
	}
	/* Mark first free spot in map_bfr */
	j = 0;
	for (map_i = 0; map_i < 512; map_i++) {
		if (num_sectors <= 0) {
			break;	
		}
		if (map_bfr[map_i] == 0) {
			map_bfr[map_i] = 0xFF;
			dir_bfr[free_dir_i + 6 + j++] = map_i;
			k = 0;
			while (k < 512) {
			/* while (file_contents[k] != '\0' && k<512) { */
				file_bfr[k] = file_contents[k];
				k++;
			}
			file_contents += 512;
			writeSector(file_bfr, map_i);
			num_sectors--;
		}
	}
	writeSector(map_bfr,1);
	writeSector(dir_bfr,2);
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

void printNumber(short number) {
  char tmp[7];
  char output[7];
  char c;
  short orig, i, j;

  for(i=0;i<7;i++) {
    output[i] = 0;
    tmp[i] = 0;
  }

  i = 0;
  orig = number;
  if (number < 0) number = -number;

  do {
    tmp[i++] = mod(number, 10) + '0';
  } while ((number = div(number,10)) > 0);

  if (orig < 0)  tmp[i++] = '-';

  i--;
  for (j = 0; i>=0; i--, j++) {
      output[j] = tmp[i];
  }

  printString(output);
}

void executeProgram(char* name) {
	char buffer[13312];
	
	int addr = 0x0000;
	int i, j, segment;

	setKernelDataSegment();
	for(i = 0; i < MAX_PROCESS; i++){
		if(processTable[i].isActive == 0){
			processTable[i].isActive = 1;
			processTable[i].sp = 0xFF00;
			restoreDataSegment(); 
			/* 1 - load file into buffer */
			readFile(name, buffer);

			segment = 0x2000 + (0x1000 * i);
			/* 2 - transfer file from buffer into start of mem at the segment in the parameter.*/
			for (j = 0; j < MAX_FILE_SIZE; j++) {
				putInMemory(segment,addr + j, buffer[j]);
			}

			initializeProgram(segment);
			break;
		}
	}
}

void killProcess(int pid) {
	setKernelDataSegment();
	processTable[pid].isActive = 0;
	processTable[pid].sp = 0xff00;

	if (processTable[0].waitingOn == pid) {
		processTable[0].waitingOn = -1;
		processTable[0].isWaiting = 0;
	}
	restoreDataSegment();
}

void terminateProgram() {
	setKernelDataSegment();
	processTable[currentProcess].sp = 0xff00;
	processTable[currentProcess].isActive = 0;
	if (processTable[0].waitingOn == currentProcess) {
		processTable[0].waitingOn = -1;
		processTable[0].isWaiting = 0;
	}
	while(1);
}

void wait(char* filename) {
	char buffer[13312];
	int addr = 0x0000;
	int i, j, segment;

	setKernelDataSegment();
	processTable[0].isWaiting = 1;
	for(i = 0; i < MAX_PROCESS; i++){
		if(processTable[i].isActive == 0){
			processTable[0].waitingOn = i;
			processTable[i].isActive = 1;
			processTable[i].sp = 0xFF00;

			restoreDataSegment(); 
			/* 1 - load file into buffer */
			readFile(filename, buffer);

			segment = 0x2000 + (0x1000 * i);
			/* 2 - transfer file from buffer into start of mem at the segment in the parameter.*/
			for (j = 0; j < MAX_FILE_SIZE; j++) {
				putInMemory(segment,addr + j, buffer[j]);
			}

			initializeProgram(segment);
			break;
		}
	}
}
