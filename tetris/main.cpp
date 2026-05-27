#include <curses.h>

// Board dimensions
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;

// Board offset on screen (where to draw it)
const int BOARD_X = 12;
const int BOARD_Y = 1;

// The game board
int board[BOARD_HEIGHT][BOARD_WIDTH] = { 0 };

// Draw the empty board border and grid
void drawBoard() {
    // Draw top border
    mvprintw(BOARD_Y - 1, BOARD_X - 1, "+--------------------+");

    // Draw sides and empty cells
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        mvprintw(BOARD_Y + y, BOARD_X - 1, "|");
        for (int x = 0; x < BOARD_WIDTH; x++) {
            mvprintw(BOARD_Y + y, BOARD_X + x * 2, ". ");
        }
        mvprintw(BOARD_Y + y, BOARD_X + BOARD_WIDTH * 2, "|");
    }

    // Draw bottom border
    mvprintw(BOARD_Y + BOARD_HEIGHT, BOARD_X - 1, "+--------------------+");
}

int main() {
    // Initialize PDCurses
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    // Draw the board
    drawBoard();
    refresh();

    // Wait for keypress then exit
    getch();
    endwin();
    return 0;
}