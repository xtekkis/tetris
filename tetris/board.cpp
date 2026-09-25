// The rules of the game: where pieces can go, rotating them, clearing lines
// and dealing the next piece. Nothing in here draws on screen.

#include "game.h"

#include <cstdlib>

int board[BOARD_HEIGHT][BOARD_WIDTH] = { 0 };

// The 7 tetromino shapes
// 1 = block, 0 = empty
const int TETROMINOES[PIECE_COUNT][SHAPE_SIZE][SHAPE_SIZE] = {
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
int nextPiece = 0;
int heldPiece = EMPTY_HOLD;

// The player can only hold once per piece, until the next piece spawns
bool holdUsed = false;

int currentX = SPAWN_X;
int currentY = 0;

// Active piece shape
int currentShape[SHAPE_SIZE][SHAPE_SIZE];

// The pieces still to come, dealt 7 at a time in a random order
static int bag[PIECE_COUNT];

// How far through the bag we are, starting empty so the first piece refills it
static int bagIndex = PIECE_COUNT;

// Copy one 4x4 shape into another
void copyShape(const int from[SHAPE_SIZE][SHAPE_SIZE], int to[SHAPE_SIZE][SHAPE_SIZE]) {
    for (int y = 0; y < SHAPE_SIZE; y++) {
        for (int x = 0; x < SHAPE_SIZE; x++) {
            to[y][x] = from[y][x];
        }
    }
}

// Copy a tetromino into the active shape
void loadPiece(int piece) {
    copyShape(TETROMINOES[piece], currentShape);
}

// Check if the current shape can be at the given position
bool isValidPosition(int posX, int posY) {
    for (int y = 0; y < SHAPE_SIZE; y++) {
        for (int x = 0; x < SHAPE_SIZE; x++) {
            if (currentShape[y][x] == 1) {
                int newX = posX + x;
                int newY = posY + y;

                // Check boundaries
                if (newX < 0 || newX >= BOARD_WIDTH) return false;
                if (newY >= BOARD_HEIGHT) return false;

                // Check if cell is already occupied
                if (newY >= 0 && board[newY][newX] != 0) return false;
            }
        }
    }
    return true;
}

// Empty the whole board, ready for a new game
void clearBoard() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x] = 0;
        }
    }
}

// Check for and clear completed lines
int clearLines() {
    int linesCleared = 0;

    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
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

            y++;
        }
    }
    return linesCleared;
}

// Rotate the current shape 90 degrees clockwise
void rotatePiece() {
    int temp[SHAPE_SIZE][SHAPE_SIZE] = { 0 };

    // Transpose and reverse to rotate clockwise
    for (int y = 0; y < SHAPE_SIZE; y++) {
        for (int x = 0; x < SHAPE_SIZE; x++) {
            temp[x][SHAPE_SIZE - 1 - y] = currentShape[y][x];
        }
    }

    // Keep a copy of the old shape, then apply the rotation
    int backupShape[SHAPE_SIZE][SHAPE_SIZE];
    copyShape(currentShape, backupShape);
    copyShape(temp, currentShape);

    // If the rotated piece does not fit, try moving it up to two cells sideways
    const int shifts[5] = { 0, -1, 1, -2, 2 };
    for (int i = 0; i < 5; i++) {
        if (isValidPosition(currentX + shifts[i], currentY)) {
            currentX += shifts[i];
            return;
        }
    }

    // It does not fit anywhere, so undo the rotation
    copyShape(backupShape, currentShape);
}

// Refill the bag with all 7 pieces in a random order
static void fillBag() {
    for (int i = 0; i < PIECE_COUNT; i++) {
        bag[i] = i;
    }

    // Shuffle by swapping each piece with a random one at or before it
    for (int i = PIECE_COUNT - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = bag[i];
        bag[i] = bag[j];
        bag[j] = temp;
    }

    bagIndex = 0;
}

// Start a new bag, so a new game does not carry on the old piece order
void resetBag() {
    bagIndex = PIECE_COUNT;
}

// Take the next piece out of the bag, refilling it when it runs out
int takePieceFromBag() {
    if (bagIndex >= PIECE_COUNT) {
        fillBag();
    }

    int piece = bag[bagIndex];
    bagIndex++;
    return piece;
}

// Swap the current piece with the held piece, once per piece
void holdPiece() {
    if (holdUsed) {
        return;
    }

    // Work out which piece would come in
    int incoming = nextPiece;
    if (heldPiece != EMPTY_HOLD) {
        incoming = heldPiece;
    }

    // Remember the current piece in case the swap does not fit
    int backupShape[SHAPE_SIZE][SHAPE_SIZE];
    copyShape(currentShape, backupShape);
    int backupX = currentX;
    int backupY = currentY;

    // The swapped in piece starts at the top again
    currentX = SPAWN_X;
    currentY = 0;
    loadPiece(incoming);

    // If the stack is too high for it, leave everything as it was
    if (!isValidPosition(currentX, currentY)) {
        copyShape(backupShape, currentShape);
        currentX = backupX;
        currentY = backupY;
        return;
    }

    if (heldPiece == EMPTY_HOLD) {
        // Nothing was held yet, so carry on with the next piece
        nextPiece = takePieceFromBag();
    }

    heldPiece = currentPiece;
    currentPiece = incoming;

    holdUsed = true;
}
