#include <curses.h>

// Board dimensions
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;

// Board offset
int BOARD_X = 12;
int BOARD_Y = 1;

// The game board
int board[BOARD_HEIGHT][BOARD_WIDTH] = { 0 };

// The 7 tetromino shapes
// 1 = block, 0 = empty
const int TETROMINOES[7][4][4] = {
    // I piece
    {
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    // O piece
    {
        {0, 0, 0, 0},
        {0, 1, 1, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0}
    },
    // T piece
    {
        {0, 0, 0, 0},
        {0, 1, 0, 0},
        {1, 1, 1, 0},
        {0, 0, 0, 0}
    },
    // S piece
    {
        {0, 0, 0, 0},
        {0, 1, 1, 0},
        {1, 1, 0, 0},
        {0, 0, 0, 0}
    },
    // Z piece
    {
        {0, 0, 0, 0},
        {1, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0}
    },
    // J piece
    {
        {0, 0, 0, 0},
        {1, 0, 0, 0},
        {1, 1, 1, 0},
        {0, 0, 0, 0}
    },
    // L piece
    {
        {0, 0, 0, 0},
        {0, 0, 1, 0},
        {1, 1, 1, 0},
        {0, 0, 0, 0}
    }
};

// Current piece state
int currentPiece = 0;
int currentX = 3;
int currentY = 0;

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

// Draw the stats panel to the left of the board
void drawStats(int score, int lines, int level) {
    int statsX = BOARD_X - 14;
    int statsY = BOARD_Y;

    mvprintw(statsY, statsX, "+----------+");
    mvprintw(statsY + 1, statsX, "|  STATS   |");
    mvprintw(statsY + 2, statsX, "+----------+");
    mvprintw(statsY + 3, statsX, "| Score    |");
    mvprintw(statsY + 4, statsX, "| %-8d |", score);
    mvprintw(statsY + 5, statsX, "+----------+");
    mvprintw(statsY + 6, statsX, "| Lines    |");
    mvprintw(statsY + 7, statsX, "| %-8d |", lines);
    mvprintw(statsY + 8, statsX, "+----------+");
    mvprintw(statsY + 9, statsX, "| Level    |");
    mvprintw(statsY + 10, statsX, "| %-8d |", level);
    mvprintw(statsY + 11, statsX, "+----------+");
}

// Check if the piece can move to the given position
bool isValidPosition(int piece, int posX, int posY) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (TETROMINOES[piece][y][x] == 1) {
                int newX = posX + x;
                int newY = posY + y;

                // Check boundaries
                if (newX < 0 || newX >= BOARD_WIDTH) return false;
                if (newY >= BOARD_HEIGHT) return false;

                // Check if cell is already occupied
                if (newY >= 0 && board[newY][newX] == 1) return false;
            }
        }
    }
    return true;
}

// Draw the current falling piece on the board
void drawPiece() {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (TETROMINOES[currentPiece][y][x] == 1) {
                int screenX = BOARD_X + (currentX + x) * 2;
                int screenY = BOARD_Y + currentY + y;
                mvprintw(screenY, screenX, "[]");
            }
        }
    }
}

int main() {
    // Initialize PDCurses
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    // Get terminal size and center the board
    int termHeight, termWidth;
    getmaxyx(stdscr, termHeight, termWidth);
    BOARD_X = (termWidth / 2) - BOARD_WIDTH;
    BOARD_Y = (termHeight / 2) - (BOARD_HEIGHT / 2);

    // Draw the board and stats
    drawBoard();
    drawPiece();
    drawStats(0, 0, 0);
    refresh();

    // Wait for keypress then exit
    getch();
    endwin();
    return 0;
}