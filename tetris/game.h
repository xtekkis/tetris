#pragma once

// Shared settings, shared variables and the functions each file provides.
// The game is split into four files:
//   board.cpp    the rules: piece positions, rotation, clearing lines, the bag
//   draw.cpp     everything that draws on screen
//   screens.cpp  the title, pause and game over screens
//   main.cpp     the game loop that ties it together

// Board dimensions
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;

// Number of different pieces
const int PIECE_COUNT = 7;

// Piece shapes are stored in a 4 by 4 grid
const int SHAPE_SIZE = 4;

// Column a new piece starts in
const int SPAWN_X = 3;

// Value of heldPiece when the player has not held anything yet
const int EMPTY_HOLD = -1;

// How long the game waits for a key press before drawing the next frame
const int INPUT_WAIT_MS = 50;

// Smallest terminal the board and the side panels fit in
const int MIN_TERM_WIDTH = 54;
const int MIN_TERM_HEIGHT = 23;

// ---------------------------------------------------------------------------
// Shared variables, created in board.cpp
// ---------------------------------------------------------------------------

// The game board
// 0 is empty, 1 to 7 is the piece that filled the cell, which gives it its color
extern int board[BOARD_HEIGHT][BOARD_WIDTH];

// The 7 tetromino shapes
// 1 = block, 0 = empty
extern const int TETROMINOES[PIECE_COUNT][SHAPE_SIZE][SHAPE_SIZE];

// Current piece state
extern int currentPiece;
extern int nextPiece;
extern int heldPiece;

// The player can only hold once per piece, until the next piece spawns
extern bool holdUsed;

extern int currentX;
extern int currentY;

// Active piece shape
extern int currentShape[SHAPE_SIZE][SHAPE_SIZE];

// Where the board is drawn, created in draw.cpp
extern int BOARD_X;
extern int BOARD_Y;

// ---------------------------------------------------------------------------
// board.cpp: the rules
// ---------------------------------------------------------------------------

void copyShape(const int from[SHAPE_SIZE][SHAPE_SIZE], int to[SHAPE_SIZE][SHAPE_SIZE]);
void loadPiece(int piece);
bool isValidPosition(int posX, int posY);
void clearBoard();
int clearLines();
void rotatePiece();

// Pieces are dealt 7 at a time in a random order
void resetBag();
int takePieceFromBag();

// Swap the current piece with the held piece, once per piece
void holdPiece();

// ---------------------------------------------------------------------------
// draw.cpp: drawing
// ---------------------------------------------------------------------------

void initColors();
void centerBoard();
void drawBoard();
void drawScreen(int score, int lines, int level);
void drawPiece();
void drawGhost();
void flashFullLines();

// ---------------------------------------------------------------------------
// screens.cpp: the screens outside the game loop
// ---------------------------------------------------------------------------

void handleResize();
bool pauseGame();
int showTitleScreen(int highScore);
bool askPlayAgain(int score, int highScore, bool newBest);
