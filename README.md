# Ink

## Overview

This project is a minimalistic Vim-inspired text editor implemented in C, using the `ncurses` library for terminal-based rendering.&#x20;

## Features

- **Command, Insert, and Visual Modes**:
  - Command mode for navigation and commands.
  - Insert mode for editing text.
  - Visual mode for selecting and copying text.
- **Undo/Redo Stack**: Unlimited undo and redo operations within the defined history limit.
- **Text Navigation**: Cursor movement across lines and columns.
- **File Operations**:
  - Load a file when starting the editor.
  - Save changes back to the file.
- **Clipboard Support**:
  - Copy selected text in visual mode.
  - Paste clipboard contents.

### Prerequisites

Ensure you have the following installed:

- GCC (GNU Compiler Collection)
- `ncurses` library

### Compilation

To compile the project, run:

```bash
gcc -o ink ink.c -lncurses
```

This command compiles the `vim_editor.c` source file and links it with the `ncurses` library.

## Usage

Run the compiled binary:

```bash
./ink [filename]
```

- If a `filename` is provided, the file will be loaded into the editor. If not, a blank buffer is opened.

### Key Bindings

#### General

- `q` - Quit the editor
- `s` - Save the file

#### Modes

- `i` - Enter insert mode
- `v` - Enter visual mode
- `Esc` - Exit insert or visual mode, returning to command mode

#### Navigation

- `h` - Move cursor left
- `j` - Move cursor down
- `k` - Move cursor up
- `l` - Move cursor right

#### Editing

- `u` - Undo the last action
- `r` - Redo the last undone action
- `y` - Copy selected text in visual mode
- `p` - Paste clipboard contents
- `Backspace` - Delete the character before the cursor
- `Enter` - Insert a new line

### Visual Mode

- Start by pressing `v` in command mode.
- Use navigation keys (`h`, `j`, `k`, `l`) to select text.
- Press `y` to copy the selection.

### Clipboard

- After copying text in visual mode, press `p` in command mode to paste the contents at the cursor's position.

## Code Structure

### Main Components

1. **TextBuffer**: Manages the text being edited, including line lengths and cursor positions.
2. **Undo/Redo System**: Tracks changes to allow reversing or reapplying edits.
3. **File I/O**:
   - `loadFile()`: Loads the contents of a file into the editor.
   - `saveFile()`: Saves the current buffer to a file.
4. **Rendering**: Displays the text buffer in the terminal with line numbers and mode indicators.

### Key Functions

- `insertChar()`: Inserts a character at the current cursor position.
- `deleteChar()`: Deletes the character before the cursor.
- `undo()` and `redo()`: Manage the history of edits.
- `render()`: Refreshes the screen to reflect changes.

## Dependencies

- `ncurses`: Used for terminal rendering and handling user input.

##

