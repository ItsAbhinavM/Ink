#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <termios.h>
#include <ncurses.h>

struct termios orig_termios;

void die(const char *s) {
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_oflag &= ~(OPOST);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 1;
}

typedef struct {
    char *text;
    size_t length;
    size_t capacity;
} TextBuffer;

TextBuffer buffer = {NULL, 0, 0};

void initTextBuffer() {
    buffer.capacity = 1024;
    buffer.text = (char *)malloc(buffer.capacity);
    if (!buffer.text) die("malloc");
    buffer.length = 0;
}

void appendTextBuffer(char c) {
    if (buffer.length + 1 >= buffer.capacity) {
        buffer.capacity *= 2;
        buffer.text = (char *)realloc(buffer.text, buffer.capacity);
        if (!buffer.text) die("realloc");
    }
    buffer.text[buffer.length++] = c;
}

void deleteTextBuffer() {
    if (buffer.length > 0) {
        buffer.length--;
    }
}

void render() {
    clear();
    for (size_t i = 0; i < buffer.length; i++) {
        addch(buffer.text[i]);
    }
    refresh();
}

void loadFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file for reading");
        return;
    }
    char c;
    while ((c = fgetc(file)) != EOF) {
        appendTextBuffer(c);
    }
    fclose(file);
}

void saveFile(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file for writing");
        return;
    }
    fwrite(buffer.text, 1, buffer.length, file);
    fclose(file);
}

int main(int argc, char *argv[]) {
    enableRawMode();
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    initTextBuffer();

    if (argc > 1) {
        loadFile(argv[1]);
    }

    int ch;
    while ((ch = getch()) != 'q') {
        if (ch == 127) {
            deleteTextBuffer();
        } else if (ch == 19) { 
            if (argc > 1) {
                saveFile(argv[1]);
            } else {
                printw("No file specified to save.\n");
                refresh();
            }
        } else if (ch != 27) {
            appendTextBuffer(ch);
        }
        render();
    }

    endwin();
    free(buffer.text);
    return 0;
}
