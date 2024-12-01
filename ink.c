#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

struct termios orig_termios;

void die(const char *s) {
  perror(s);
  exit(1);
}

void disableRawMode() {
 if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    die("tcsetattr");
} // This is to enable the characters being shown in the terminal on exit as the previous functions disables it 

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);
  struct termios raw = orig_termios;
  raw.c_oflag &= ~(OPOST); // Disables the output processing that is it disable the \n that comes behind when we press enter 
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); // Disables the ctrl-s and ctrl-q and ctrl-m
  raw.c_cflag |= (CS8); // Sets the character size to 8 bits per byte
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); // Make the terminal to read one character at a time instead of line at a time // SIG is to disable the signals like ctrl-c and ctrl-z and ctrl-v
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  raw.c_cc[VMIN] = 0; // This is to set the minimum number of bytes of input needed before read() can return
  raw.c_cc[VTIME] = 1; // This is to set the maximum amount of time to wait before read() returns
} // This function is to disable the character from being shown in the terminal when typing 



int main() {
    enableRawMode();
    char c ;
    while (1) {
    char c = '\0';
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("read");
    read(STDIN_FILENO, &c, 1);
    if (iscntrl(c)) {
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }
    if (c == 'q') break;
  }
  return 0 ;
}
