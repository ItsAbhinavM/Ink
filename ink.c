#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ncurses.h>
#define min(a,b) ((a) < (b) ? (a) : (b))
struct termios orig_termios;


char *clipboard = NULL;
size_t clipboard_length = 0;

void die(const char *s) {
    endwin();  
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
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}
typedef struct {
    char *text;
    size_t length;
    size_t capacity;
    int curr_line;
    int curr_col;
    int cursor_row;
    int cursor_col;
    int line_lengths[1000]; 
    int total_lines;
} TextBuffer;

// TextBuffer buffer = {NULL, 0, 0, 0, 0, {0}, 0};
TextBuffer buffer = {0};
int is_insert_mode = 0;

typedef struct {
    char action_type; 
    size_t position;
    char character; 
} Action;

#define MAX_HISTORY 1000
Action undo_stack[MAX_HISTORY];
Action redo_stack[MAX_HISTORY];
int undo_top = -1;
int redo_top = -1;


void initTextBuffer() {
    buffer.capacity = 4096;  
    buffer.text = (char *)malloc(buffer.capacity);
    if (!buffer.text) die("malloc");
    buffer.length = 0;
    buffer.curr_line = 0;
    buffer.curr_col = 0;
    buffer.total_lines = 1;
    buffer.line_lengths[0] = 0;
}

void updateLineInfo() {
    buffer.total_lines = 1;
    buffer.line_lengths[0] = 0;
    int curr_line_length = 0;

    for (size_t i = 0; i < buffer.length; i++) {
        if (buffer.text[i] == '\n') {
            buffer.line_lengths[buffer.total_lines - 1] = curr_line_length;
            buffer.total_lines++;
            curr_line_length = 0;
        } else {
            curr_line_length++;
        }
    }
    buffer.line_lengths[buffer.total_lines - 1] = curr_line_length;
}
void pushUndo(Action action) {
    if (undo_top < MAX_HISTORY - 1) {
        undo_stack[++undo_top] = action;
    }
}

Action popUndo() {
    return undo_top >= 0 ? undo_stack[undo_top--] : (Action){0};
}

void pushRedo(Action action) {
    if (redo_top < MAX_HISTORY - 1) {
        redo_stack[++redo_top] = action;
    }
}

Action popRedo() {
    return redo_top >= 0 ? redo_stack[redo_top--] : (Action){0};
}


void insertChar(char c) {
    buffer.curr_line = buffer.cursor_row;
    buffer.curr_col = buffer.cursor_col;
    if (buffer.length + 1 >= buffer.capacity) {
        buffer.capacity *= 2;
        buffer.text = (char *)realloc(buffer.text, buffer.capacity);
        if (!buffer.text) die("realloc");
    }
    size_t abs_pos = 0;
    for (int i = 0; i < buffer.curr_line; i++) {
        abs_pos += buffer.line_lengths[i] + 1;
    }
    abs_pos += buffer.curr_col;

    Action action = {'i', abs_pos, c};
    pushUndo(action);
    redo_top = -1; 

    if (c == '\n') {
        memmove(buffer.text + abs_pos + 1, buffer.text + abs_pos, buffer.length - abs_pos);
        buffer.text[abs_pos] = c;
        buffer.length++;
        buffer.total_lines++;
    } else {
        memmove(buffer.text + abs_pos + 1, buffer.text + abs_pos, buffer.length - abs_pos);
        buffer.text[abs_pos] = c;
        buffer.length++;
    }

    updateLineInfo();

    if (c == '\n') {
        buffer.cursor_row++;
        buffer.cursor_col = 0;
        buffer.curr_line = buffer.cursor_row;
        buffer.curr_col = 0;
    } else {
        buffer.cursor_col++;
        buffer.curr_col++;
    }
}
void deleteChar() {
    buffer.curr_line = buffer.cursor_row;
    buffer.curr_col = buffer.cursor_col;

    if (buffer.length == 0) return;

    size_t abs_pos = 0;
    for (int i = 0; i < buffer.curr_line; i++) {
        abs_pos += buffer.line_lengths[i] + 1;
    }
    abs_pos += buffer.curr_col;

    if (abs_pos == 0) return;

    
    char deleted_char = buffer.text[abs_pos - 1];
    Action action = {'d', abs_pos - 1, deleted_char};
    pushUndo(action);
    redo_top = -1; 

    if (buffer.curr_col == 0 && buffer.curr_line > 0) {
        size_t prev_line_length = buffer.line_lengths[buffer.curr_line - 1];

        buffer.cursor_row--;
        buffer.cursor_col = prev_line_length;

        memmove(buffer.text + abs_pos - 1, buffer.text + abs_pos, buffer.length - abs_pos);
        buffer.length--;
    } else if (buffer.curr_col > 0) {
        memmove(buffer.text + abs_pos - 1, buffer.text + abs_pos, buffer.length - abs_pos);
        buffer.length--;
        buffer.cursor_col--;
    }

    updateLineInfo();
}

void undo() {
    if (undo_top < 0) return;

    Action action = popUndo();
    pushRedo(action);

    size_t abs_pos = action.position;
    if (action.action_type == 'i') {
        
        memmove(buffer.text + abs_pos, buffer.text + abs_pos + 1, buffer.length - abs_pos - 1);
        buffer.length--;

        
        buffer.cursor_row = 0;
        buffer.cursor_col = 0;
        for (int i = 0; i < buffer.total_lines && abs_pos > buffer.line_lengths[i]; i++) {
            abs_pos -= (buffer.line_lengths[i] + 1);
            buffer.cursor_row++;
        }
        buffer.cursor_col = abs_pos;

    } else if (action.action_type == 'd') {
        
        memmove(buffer.text + abs_pos + 1, buffer.text + abs_pos, buffer.length - abs_pos);
        buffer.text[abs_pos] = action.character;
        buffer.length++;

        
        buffer.cursor_row = 0;
        buffer.cursor_col = 0;
        for (int i = 0; i < buffer.total_lines && abs_pos >= buffer.line_lengths[i]; i++) {
            abs_pos -= (buffer.line_lengths[i] + 1);
            buffer.cursor_row++;
        }
        buffer.cursor_col = abs_pos + 1; 
    }

    updateLineInfo();
}


void redo() {
    if (redo_top < 0) return;

    Action action = popRedo();
    pushUndo(action);

    size_t abs_pos = action.position;

    if (action.action_type == 'i') { 
        memmove(buffer.text + abs_pos + 1, buffer.text + abs_pos, buffer.length - abs_pos);
        buffer.text[abs_pos] = action.character;
        buffer.length++;

        
        buffer.cursor_row = 0;
        buffer.cursor_col = 0;
        for (int i = 0; i < buffer.total_lines && abs_pos > buffer.line_lengths[i]; i++) {
            abs_pos -= (buffer.line_lengths[i] + 1);
            buffer.cursor_row++;
        }
        buffer.cursor_col = abs_pos;

    } else if (action.action_type == 'd') { 
        memmove(buffer.text + abs_pos, buffer.text + abs_pos + 1, buffer.length - abs_pos - 1);
        buffer.length--;

        
        buffer.cursor_row = 0;
        buffer.cursor_col = 0;
        for (int i = 0; i < buffer.total_lines && abs_pos > buffer.line_lengths[i]; i++) {
            abs_pos -= (buffer.line_lengths[i] + 1);
            buffer.cursor_row++;
        }
        buffer.cursor_col = abs_pos;
    }

    updateLineInfo(); 
}





int is_visual_mode = 0;
int selection_start_row = -1;
int selection_start_col = -1;




void render() {
    clear();
    int line_num_width = 5;  
    for (int line = 0; line < buffer.total_lines; line++) {
        mvprintw(line, 0, "%3d ", line + 1);
        size_t line_start = 0;
        for (int prev_line = 0; prev_line < line; prev_line++) {
            line_start += buffer.line_lengths[prev_line] + 1; 
        }

        for (int col = 0; col < buffer.line_lengths[line]; col++) {
            if (is_visual_mode &&
                ((line > selection_start_row || 
                 (line == selection_start_row && col >= selection_start_col)) &&
                (line < buffer.cursor_row || 
                 (line == buffer.cursor_row && col <= buffer.cursor_col)))) {
                attron(A_REVERSE);
            }
            mvaddch(line, line_num_width + col, buffer.text[line_start + col]);
            attroff(A_REVERSE);
        }
    }

    move(buffer.curr_line, line_num_width + buffer.curr_col);
    curs_set(1);
    if (is_insert_mode) {
        mvprintw(LINES - 1, 0, "-- INSERT MODE --");
    } else if (is_visual_mode) {
        mvprintw(LINES - 1, 0, "-- VISUAL MODE --");
    }  else {
        mvprintw(LINES - 1, 0, "-- COMMAND MODE --");
    }
    move(buffer.cursor_row, line_num_width + buffer.cursor_col);
    refresh();
}

int getLineLength(int line) {
    return buffer.line_lengths[line];
}

void moveCursorUp() {
    if (buffer.cursor_row > 0) {
        buffer.cursor_row--;
        buffer.cursor_col = min(buffer.cursor_col, getLineLength(buffer.cursor_row));
    }
}

void moveCursorDown() {
    if (buffer.cursor_row < buffer.total_lines - 1) {
        buffer.cursor_row++;
        buffer.cursor_col = min(buffer.cursor_col, getLineLength(buffer.cursor_row));
    }
}

void moveCursorLeft() {
    if (buffer.cursor_col > 0) {
        buffer.cursor_col--;
    } else if (buffer.cursor_row > 0) {
        buffer.cursor_row--;
        buffer.cursor_col = buffer.line_lengths[buffer.cursor_row];
    }
}

void moveCursorRight() {
    if (buffer.cursor_col < buffer.line_lengths[buffer.cursor_row]) {
        buffer.cursor_col++;
    } else if (buffer.cursor_row < buffer.total_lines - 1) {
        buffer.cursor_row++;
        buffer.cursor_col = 0;
    }
}

void loadFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        mvprintw(LINES - 2, 0, "Error opening file: %s", filename);
        refresh();
        return;
    }

    buffer.length = 0;
    buffer.curr_line = 0;
    buffer.curr_col = 0;

    int c;
    while ((c = fgetc(file)) != EOF) {
        insertChar(c);
    }

    fclose(file);
    updateLineInfo();
}

void saveFile(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        mvprintw(LINES - 2, 0, "Error saving file: %s", filename);
        refresh();
        return;
    }

    fwrite(buffer.text, 1, buffer.length, file);
    fclose(file);
    mvprintw(LINES - 2, 0, "File saved successfully!");
    refresh();
}

void resetSelection() {
    is_visual_mode = 0;
    selection_start_row = -1;
    selection_start_col = -1;
}


void updateSelection(int row, int col) {
    if (selection_start_row == -1) {
        selection_start_row = row;
        selection_start_col = col;
    }
    
    int start_row = selection_start_row;
    int start_col = selection_start_col;
    int end_row = row;
    int end_col = col;

    if (row < start_row || 
        (row == start_row && col < start_col)) {
        start_row = row;
        start_col = col;
        end_row = selection_start_row;
        end_col = selection_start_col;
    }

    buffer.cursor_row = end_row;
    buffer.cursor_col = end_col;
    selection_start_row = start_row;
    selection_start_col = start_col;
}

void copy() {
    if (selection_start_row == -1) return; 

    if (clipboard) {
        free(clipboard);
        clipboard = NULL;
    }

    int start_row = selection_start_row;
    int start_col = selection_start_col;
    int end_row = buffer.cursor_row;
    int end_col = buffer.cursor_col;

    if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
        int temp_row = start_row;
        int temp_col = start_col;
        start_row = end_row;
        start_col = end_col;
        end_row = temp_row;
        end_col = temp_col;
    }

    clipboard_length = 0;
    for (int row = start_row; row <= end_row; row++) {
        int row_start = 0;
        for (int prev_row = 0; prev_row < row; prev_row++) {
            row_start += buffer.line_lengths[prev_row] + 1;
        }

        int start = (row == start_row) ? start_col : 0;
        int end = (row == end_row) ? end_col : buffer.line_lengths[row];

        clipboard_length += (end - start + ((row != end_row) ? 1 : 0));
    }

    clipboard = malloc(clipboard_length + 1);
    if (!clipboard) die("malloc clipboard");

    size_t clipboard_index = 0;
    for (int row = start_row; row <= end_row; row++) {
        int row_start = 0;
        for (int prev_row = 0; prev_row < row; prev_row++) {
            row_start += buffer.line_lengths[prev_row] + 1;
        }

        int start = (row == start_row) ? start_col : 0;
        int end = (row == end_row) ? end_col : buffer.line_lengths[row];

        memcpy(clipboard + clipboard_index, 
            buffer.text + row_start + start, 
            end - start);
        clipboard_index += (end - start);

        if (row != end_row) {
            clipboard[clipboard_index++] = '\n';
        }
    }
    clipboard[clipboard_index] = '\0';
}

void paste() {
    if (!clipboard || clipboard_length == 0) return;  

    for (size_t i = 0; i < clipboard_length; i++) {
        insertChar(clipboard[i]);
    }
}


int main(int argc, char *argv[]) {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    
    enableRawMode();
    initTextBuffer();
    if (argc > 1) {
        loadFile(argv[1]);
    }

    int ch;
    while (1) {
        render();
        ch = getch();

        if (is_insert_mode) {
            switch (ch) {
                case 27: 
                    is_insert_mode = 0;
                    break;
                case KEY_BACKSPACE:
                case 127:
                    deleteChar();
                    break;
                case KEY_ENTER:
                case 10:
                case '\r':
                    insertChar('\n');
                    break;
                default:
                    if (ch >= 32 && ch <= 126) {  
                        insertChar(ch);
                    }
                    break;
            }
        } 
        else if (is_visual_mode) {
            switch (ch) {
                case 'y':  
                    copy();
                    resetSelection();
                    break;
                case 27:  // Escape key 
                    resetSelection();
                    break;
                case 'h':
                    moveCursorLeft();
                    is_visual_mode = 1;
                    updateSelection(buffer.cursor_row, buffer.cursor_col);
                    break;
                case 'j':
                    moveCursorDown();
                    is_visual_mode = 1;
                    updateSelection(buffer.cursor_row, buffer.cursor_col);
                    break;
                case 'k':
                    moveCursorUp();
                    is_visual_mode = 1;
                    updateSelection(buffer.cursor_row, buffer.cursor_col);
                    break;
                case 'l':
                    moveCursorRight();
                    is_visual_mode = 1;
                    updateSelection(buffer.cursor_row, buffer.cursor_col);
                    break;
            }
        }
        else {
            switch (ch) {
                case 'q':
                    endwin();
                    free(buffer.text);
                    return 0;
                case 's':
                    if (argc > 1) {
                        saveFile(argv[1]);
                    }
                    break;
                case 'i':
                    is_insert_mode = 1;
                    break;
                case 'v':
                    is_visual_mode = 1;
                    selection_start_row = buffer.cursor_row;
                    selection_start_col = buffer.cursor_col;
                    break;
                case 'h':
                    moveCursorLeft();
                    updateSelection(buffer.cursor_row, buffer.cursor_col);
                    break;
                case 'j':
                    moveCursorDown();
                    updateSelection(buffer.cursor_row, buffer.cursor_col);
                    break;
                case 'k':
                    moveCursorUp();
                    updateSelection(buffer.cursor_row, buffer.cursor_col);
                    break;
                case 'l':
                    moveCursorRight();
                    updateSelection(buffer.cursor_row, buffer.cursor_col);
                    break;
                case 'u': 
                    undo();
                    break;
                case 'r': 
                    redo();
                    break;
                case 'y': 
                    copy();
                    break;
                case 'p': 
                    paste();
                    break;
                default:
                    break;
            }
        }
    }
}
