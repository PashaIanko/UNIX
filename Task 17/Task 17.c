#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#define TIME_TO_WAIT 5
#define MIN_SYMB_NUMB 1

#define ISATTY_ERR -1
#define TCGET_ERR -2
#define TCSET_ERR -3

#define MAX_SIZE 40


char line[MAX_SIZE];


int find_prev_space(int pos) {
	while (pos > 0 && !isspace(line[pos - 1])) {
		--pos;
	}
}

int main() {
  if (!isatty(STDIN_FILENO)) {
	fprintf(stderr, "Standard input is not a terminal\n");
	return ISATTY_ERR;
  }

  struct termios oldattr, newattr;
  if (tcgetattr(STDIN_FILENO, &oldattr) == -1) {
	perror("Error while getting terminal attributes\n");
	return TCGET_ERR;
  }

  newattr = oldattr;
  /*disable echo (task), disable canonical input, disable reaction
		to INTR, QUIT, SUSP, --> symbol # no longer works*/

  newattr.c_lflag = ~(ISIG | ICANON | ECHO) & ICRNL; /*flag ICRNL to react to ENTER
							(did not react w/o the flag)*/
  newattr.c_cc[VMIN] = MIN_SYMB_NUMB; /*min symbols count for non-canonical input*/
  newattr.c_cc[VTIME] = TIME_TO_WAIT; /*min time to wait dcsec (wait a bit to catch Ctrl+D*/
  //newattr.c_cc[VKILL] = 'k';
  if (tcsetattr(STDIN_FILENO, TCSANOW, &newattr) == -1) {
	perror("Error while setting terminal options\n");
	return TCSET_ERR;
  }

  int pos = 0;
  char ch;

  while(read(STDIN_FILENO, &ch, 1) > 0) {
	if (ch == CEOT && pos == 0) {
		printf("LOG: caught CTRL+D\n");
		break;
	} 
	else if (ch == newattr.c_cc[VERASE] && pos > 0) {
		write(STDOUT_FILENO, "\b \b", 3);
		--pos;
	} 
	else if (ch == newattr.c_cc[VKILL]) {
      	/*Ctrl + U*/
		while (pos > 0) {
			write(STDOUT_FILENO, "\b \b", 3);
			--pos;
		}
	} 
	else if (ch == CWERASE) {

		//    Ctrl + W
		while (pos > 0 && isspace(line[pos - 1])) {
		        write(STDOUT_FILENO, "\b \b", 3);
		        --pos;
		}
      		while (pos > 0 && !isspace(line[pos - 1])) {
        		write(STDOUT_FILENO, "\b \b", 3);
        		--pos;
      		}
    	} 
	else if (ch == '\n') {
		write(STDOUT_FILENO, &ch, 1);
      		pos = 0;
    	} 
    	else if (!isprint(ch)) {
      		/*Unprintable */
      		printf("\nBeep-beep\n");
      		write(STDOUT_FILENO, "\a", 1);
    	} 
    	else {
      		write(STDOUT_FILENO, &ch, 1);
      		line[pos++] = ch;
    	}

     	if (pos == MAX_SIZE) {
      		if (!isspace(ch)) {
        	int saved = pos;
        	pos = find_prev_space(pos);
		if (pos > 0) {
			int newpos = 0;
			int i = 0;
			for (i = pos; i < saved; ++i) {
				write(STDOUT_FILENO, "\b \b", 3);
				line[newpos++] = line[i];
			}
          		pos = newpos;
          		write(STDOUT_FILENO, "\n", 1);

          		for (i = 0; i < pos; ++i) {
            			write(STDOUT_FILENO, line + i, 1);
          		}
     		} 
		else {
          		write(STDOUT_FILENO, "\n", 1);
        	}
      } 
      else {
	write(STDOUT_FILENO, "\n", 1);
        pos = 0;
      }
    }
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
  return 0;
}
