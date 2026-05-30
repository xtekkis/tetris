#include <curses.h>
#include <cstdlib>

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

    // Draw sides and cells
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        mvprintw(BOARD_Y + y, BOARD_X - 1, "|");
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 1) {
                mvprintw(BOARD_Y + y, BOARD_X + x * 2, "[]");
            }
            else {
                mvprintw(BOARD_Y + y, BOARD_X + x * 2, ". ");
            }
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

// Check for and clear completed lines
int clearLines() {
    int linesCleared = 0;

    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        // Check if row is full
        bool full = true;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 0) {
                full = false;
                break;
            }
        }

        if (full) {
            linesCleared++;

            // Shift everything above down
            for (int row = y; row > 0; row--) {
                for (int x = 0; x < BOARD_WIDTH; x++) {
                    board[row][x] = board[row - 1][x];
                }
            }

            // Clear top row
            for (int x = 0; x < BOARD_WIDTH; x++) {
                board[0][x] = 0;
            }

            // Check the same row again since rows shifted down
            y++;
        }
    }
    return linesCleared;
}

int main() {
    // Initialize PDCurses
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(100);

    // Get terminal size and center the board
    int termHeight, termWidth;
    getmaxyx(stdscr, termHeight, termWidth);
    BOARD_X = (termWidth / 2) - BOARD_WIDTH;
    BOARD_Y = (termHeight / 2) - (BOARD_HEIGHT / 2);

    // Game state
    int score = 0;
    int lines = 0;
    int level = 1;
    bool gameOver = false;

    while (!gameOver) {
        // Draw everything
        clear();
        drawBoard();
        drawPiece();
        drawStats(score, lines, level);
        refresh();

        // Auto drop
        static int dropCounter = 0;
        dropCounter++;
        if (dropCounter >= 5) {
            dropCounter = 0;
            if (isValidPosition(currentPiece, currentX, currentY + 1)) {
                currentY++;
            }
            else {
                // Place piece on board
                for (int y = 0; y < 4; y++) {
                    for (int x = 0; x < 4; x++) {
                        if (TETROMINOES[currentPiece][y][x] == 1) {
                            board[currentY + y][currentX + x] = 1;
                        }
                    }
                }
                // Spawn new piece
                currentPiece = rand() % 7;
                currentX = 3;
                currentY = 0;

                // Check game over
                if (!isValidPosition(currentPiece, currentX, currentY)) {
                    gameOver = true;
                }
            }
        }

        // Clear completed lines and update score
        int cleared = clearLines();
        if (cleared > 0) {
            lines += cleared;
            score += cleared * 100 * level;
        }

        // Handle input
        int key = getch();

        //27 is the Escape key
        if (key == 27) {
            break;
        }
        else if (key == 'a') {
            if (isValidPosition(currentPiece, currentX - 1, currentY)) {
                currentX--;
            }
        }
        else if (key == 'd') {
            if (isValidPosition(currentPiece, currentX + 1, currentY)) {
                currentX++;
            }
        }
        else if (key == 's') {
            if (isValidPosition(currentPiece, currentX, currentY + 1)) {
                currentY++;
            }
        }
    }

    endwin();
    return 0;
}