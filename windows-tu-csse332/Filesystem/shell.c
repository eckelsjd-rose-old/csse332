#define MAX_FILE_SIZE 13312
#define MAX_ARG_SIZE 20
#define CMD_NAME_LEN 10
void parseCmd(char**,char*);
int len(char*);
int stringEquals(char*, char*);
void printNumber(short); 
int mod(int, int);
int div(int, int);

int main() {
    
	while(1){
    int i,j;
    char cmd[512];
		char buffer[MAX_FILE_SIZE];
		char dir[512];
		char filename[8];

		for (i = 0; i < MAX_FILE_SIZE; i++) {
			buffer[i] = '\0';
		}

		cmd[0] = '\0';
		cmd[8] = '\0';
		dir[0] = '\0';
		filename[0] = '\0';
		i = 0;
		j = 0;

        /* step 1: fetch command name*/
        interrupt(0x21, 0,"SHELL>",0,0);
        interrupt(0x21, 1, cmd, 0, 0);
		interrupt(0x21, 0,"\r\n",0,0);

		/* type messag */
        if (cmd[0] == 't' && cmd[1] == 'y' && cmd[2] == 'p' && cmd[3] == 'e'){
			j = 0;
			while (cmd[5+j] != '\0' && j < 6 && cmd[5+j] != '\n') {
				filename[j] = cmd[5+j];
				j++;
			}
			filename[j] = '\0';
			interrupt(0x21, 3, filename, buffer, 0, 0); /* readFile filename into buffer */
			interrupt(0x21, 0, buffer, 0, 0); /* print buffer */
        }
		/* execute progra */
		else if (cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'e' && cmd[3] == 'c' && cmd[4] == 'u' && cmd[5] == 't' && cmd[6] == 'e') {
			j = 0;
			while (cmd[8+j] != '\0' && j < 6 && cmd[8+j] != '\n') {
				filename[j] = cmd[8+j];
				j++;
			}
			filename[j] = '\0';
			interrupt(0x21, 4, filename, 0x2000, 0, 0); /* executeProgram */
		} 
		/* dir */
		else if (cmd[0] == 'd' && cmd[1] == 'i' && cmd[2] == 'r') {
			interrupt(0x21, 2, dir, 2, 0); /* readSector 2 */
			for (i = 0; i < 512; i+= 32) {
				j = 0;
				while (dir[i+j] != '\0' && j < 6) {
					filename[j] = dir[i+j];
					j++;
				}
				filename[j++] = ' ';
				filename[j] = '\0';
				interrupt(0x21,0,filename,0,0);
			}
			interrupt(0x21, 0, "\r\n",0,0);
		}
		/* copy */
		else if (cmd[0] == 'c' && cmd[1] == 'o' && cmd[2] == 'p' && cmd[3] == 'y') {
			int i,j, num_sectors;
			char arg1[7];
			char arg2[7];
			arg1[0] = '\0';
			arg2[0] = '\0';
			i = 0;
			while (cmd[i + 5] != ' ' && cmd[i + 5] != '\0') {
				arg1[i] = cmd[i + 5];
				i++;
			}
			arg1[i] = '\0';
			i++;
			j = 0;
			while (cmd[i+5] != ' ' && cmd[i + 5] != '\0' && cmd[i +5] != '\n') {
				arg2[j] = cmd[i + 5];
				i++;
				j++;
			}
			arg2[j] = '\0';
			interrupt(0x21,3, arg1, buffer,0); 
			num_sectors = sizeof(buffer) / 512;
			interrupt(0x21, 8, arg2, buffer, num_sectors);
		}
		/* bad command */
		else {
            interrupt(0x21, 0,"Bad Command!!\r\n",0,0);
        }
    }
}

/* assume both are equal size*/
int stringEquals(char* toTest, char* target) {
    int i, toTestLen, targetLen;
    i = 0;

    toTestLen = len(toTest);
    targetLen = len(target);

    printNumber(toTestLen);
    printNumber(targetLen);


    if(toTestLen != targetLen){
        interrupt(0x21, 0,"lengths not equal",0,0);
        return 0;
    }

    while(toTest[i] != '\0'){
        if(toTest[i] != target[i]){
            return 0;
        }
    }

    return 1;
}

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
  interrupt(0x21, 0, output, 0, 0);
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
