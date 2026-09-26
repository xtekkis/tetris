// Everything that puts something on screen: the board, the side panels,
// the falling piece, the ghost piece and the line clear flash.

#include <curses.h>

#include "game.h"

// Where the board is drawn. centerBoard() works these out from the window size.
int BOARD_X = 12;
int BOARD_Y = 1;

// Where the side panels sit, measured from the top left corner of the board.
// If these change, check MIN_TERM_WIDTH and MIN_TERM_HEIGHT in game.h still fit.
const int LEFT_PANEL_OFFSET = 14;
const int RIGHT_PANEL_OFFSET = BOARD_WIDTH * 2 + 4;
const int HOLD_PANEL_ROW = 13;
const int CONTROLS_PANEL_ROW = 9;

// How many times completed rows blink before they disappear, and how long each step lasts
const int FLASH_STEPS = 4;
const int FLASH_STEP_MS = 60;

// Set up one color for each of the 7 pieces
void initColors() {
    if (!has_colors()) {
        return;
    }

    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);     // I
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);   // O
    init_pair(3, COLOR_MAGENTA, COLOR_BLACK);  // T
    init_pair(4, COLOR_GREEN, COLOR_BLACK);    // S
    init_pair(5, COLOR_RED, COLOR_BLACK);      // Z
    init_pair(6, COLOR_BLUE, COLOR_BLACK);     // J
    init_pair(7, COLOR_WHITE, COLOR_BLACK);    // L
}

// Put the board in the middle of the terminal
void centerBoard() {
    int termHeight, termWidth;
    getmaxyx(stdscr, termHeight, termWidth);
    BOARD_X = (termWidth / 2) - BOARD_WIDTH;
    BOARD_Y = (termHeight / 2) - (BOARD_HEIGHT / 2);
}

// Draw the board border and cells
void drawBoard() {
    // Draw top border
    mvprintw(BOARD_Y - 1, BOARD_X - 1, "+--------------------+");

    // Draw sides and cells
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        mvprintw(BOARD_Y + y, BOARD_X - 1, "|");
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] != 0) {
                attron(COLOR_PAIR(board[y][x]));
                mvprintw(BOARD_Y + y, BOARD_X + x * 2, "[]");
                attroff(COLOR_PAIR(board[y][x]));
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
static void drawStats(int score, int lines, int level) {
    int statsX = BOARD_X - LEFT_PANEL_OFFSET;
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

// Draw a panel with a title and a piece inside it, used for HOLD and NEXT.
// Pass EMPTY_HOLD as the piece to leave the box empty.
static void drawPiecePanel(int panelY, int panelX, const char* title, int piece) {
    mvprintw(panelY, panelX, "+--------+");
    mvprintw(panelY + 1, panelX, "|%s|", title);
    mvprintw(panelY + 2, panelX, "+--------+");

    // The box is as tall as a piece shape
    for (int y = 0; y < SHAPE_SIZE; y++) {
        mvprintw(panelY + 3 + y, panelX, "|        |");
    }
    mvprintw(panelY + 3 + SHAPE_SIZE, panelX, "+--------+");

    // Nothing to show before the player holds a piece
    if (piece == EMPTY_HOLD) {
        return;
    }

    attron(COLOR_PAIR(piece + 1));
    for (int y = 0; y < SHAPE_SIZE; y++) {
        for (int x = 0; x < SHAPE_SIZE; x++) {
            if (TETROMINOES[piece][y][x] == 1) {
                mvprintw(panelY + 3 + y, panelX + 1 + x * 2, "[]");
            }
        }
    }
    attroff(COLOR_PAIR(piece + 1));
}

// Draw the held piece panel below the stats panel
static void drawHold() {
    drawPiecePanel(BOARD_Y + HOLD_PANEL_ROW, BOARD_X - LEFT_PANEL_OFFSET, "  HOLD  ", heldPiece);
}

// Draw the next piece preview panel
static void drawNextPiece() {
    drawPiecePanel(BOARD_Y, BOARD_X + RIGHT_PANEL_OFFSET, "  NEXT  ", nextPiece);
}

// Draw the controls panel below the next piece preview
static void drawControls() {
    int controlsX = BOARD_X + RIGHT_PANEL_OFFSET;
    int controlsY = BOARD_Y + CONTROLS_PANEL_ROW;

    mvprintw(controlsY, controlsX, "+-----------+");
    mvprintw(controlsY + 1, controlsX, "| CONTROLS  |");
    mvprintw(controlsY + 2, controlsX, "+-----------+");
    mvprintw(controlsY + 3, controlsX, "| A - Left  |");
    mvprintw(controlsY + 4, controlsX, "| D - Right |");
    mvprintw(controlsY + 5, controlsX, "| S - Down  |");
    mvprintw(controlsY + 6, controlsX, "| W - Rotate|");
    mvprintw(controlsY + 7, controlsX, "| or arrows |");
    mvprintw(controlsY + 8, controlsX, "| SPC - Drop|");
    mvprintw(controlsY + 9, controlsX, "| C - Hold  |");
    mvprintw(controlsY + 10, controlsX, "| P - Pause |");
    mvprintw(controlsY + 11, controlsX, "| ESC - Quit|");
    mvprintw(controlsY + 12, controlsX, "+-----------+");
}

// Draw the board and all the panels, but not the falling piece
void drawScreen(int score, int lines, int level) {
    // erase() only redraws what changed, which avoids flicker
    erase();
    drawBoard();
    drawStats(score, lines, level);
    drawHold();
    drawNextPiece();
    drawControls();
}

// Draw the current falling piece
void drawPiece() {
    attron(COLOR_PAIR(currentPiece + 1));
    for (int y = 0; y < SHAPE_SIZE; y++) {
        for (int x = 0; x < SHAPE_SIZE; x++) {
            if (currentShape[y][x] == 1) {
                int screenX = BOARD_X + (currentX + x) * 2;
                int screenY = BOARD_Y + currentY + y;
                mvprintw(screenY, screenX, "[]");
            }
        }
    }
    attroff(COLOR_PAIR(currentPiece + 1));
}

// Draw ghost piece showing where current piece will land
void drawGhost() {
    // Find how far down the piece can fall
    int ghostY = currentY;
    while (isValidPosition(currentX, ghostY + 1)) {
        ghostY++;
    }

    // Only draw if ghost is different from current position
    if (ghostY == currentY) return;

    // Draw ghost with different character
    for (int y = 0; y < SHAPE_SIZE; y++) {
        for (int x = 0; x < SHAPE_SIZE; x++) {
            if (currentShape[y][x] == 1) {
                int screenX = BOARD_X + (currentX + x) * 2;
                int screenY = BOARD_Y + ghostY + y;
                mvprintw(screenY, screenX, "::");
            }
        }
    }
}

// Blink the completed rows so the player sees which lines are going
void flashFullLines() {
    // Work out which rows are full
    bool full[BOARD_HEIGHT];
    bool anyFull = false;

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        full[y] = true;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 0) {
                full[y] = false;
                break;
            }
        }
        if (full[y]) {
            anyFull = true;
        }
    }

    if (!anyFull) {
        return;
    }

    for (int step = 0; step < FLASH_STEPS; step++) {
        drawBoard();

        // On every other step, cover the finished rows so they stand out
        if (step % 2 == 0) {
            for (int y = 0; y < BOARD_HEIGHT; y++) {
                if (!full[y]) {
                    continue;
                }
                for (int x = 0; x < BOARD_WIDTH; x++) {
                    mvprintw(BOARD_Y + y, BOARD_X + x * 2, "##");
                }
            }
        }

        refresh();
        napms(FLASH_STEP_MS);
    }
}
