#include <curses.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <fstream>

// Board dimensions
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;

// Number of different pieces
const int PIECE_COUNT = 7;

// Column a new piece starts in
const int SPAWN_X = 3;

// How long a piece rests on the ground before it locks in place
const int LOCK_DELAY_MS = 500;

// How long the game waits for a key press before drawing the next frame
const int INPUT_WAIT_MS = 50;

// Points for clearing 1, 2, 3 or 4 lines at once, multiplied by the level
const int LINE_SCORES[5] = { 0, 100, 300, 500, 800 };

// Where the best score is saved, in the folder the game is run from
const char HIGH_SCORE_FILE[] = "highscore.txt";

// Points per row for dropping a piece yourself, not multiplied by the level
const int SOFT_DROP_POINTS = 1;
const int HARD_DROP_POINTS = 2;

// Smallest terminal the board and the side panels fit in
const int MIN_TERM_WIDTH = 54;
const int MIN_TERM_HEIGHT = 23;

// Board offset
int BOARD_X = 12;
int BOARD_Y = 1;

// The game board
// 0 is empty, 1 to 7 is the piece that filled the cell, which gives it its color
int board[BOARD_HEIGHT][BOARD_WIDTH] = { 0 };

// The 7 tetromino shapes
// 1 = block, 0 = empty
const int TETROMINOES[PIECE_COUNT][4][4] = {
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

// Value of heldPiece when the player has not held anything yet
const int EMPTY_HOLD = -1;

// Current piece state
int currentPiece = 0;
int nextPiece = 0;
int heldPiece = EMPTY_HOLD;

// The player can only hold once per piece, until the next piece spawns
bool holdUsed = false;

int currentX = SPAWN_X;
int currentY = 0;

// The pieces still to come, dealt 7 at a time in a random order
int bag[PIECE_COUNT];

// How far through the bag we are, starting empty so the first piece refills it
int bagIndex = PIECE_COUNT;

// Active piece shape
int currentShape[4][4];

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

// Read the best score from the save file, or 0 if there is none yet
int loadHighScore() {
    std::ifstream file(HIGH_SCORE_FILE);

    int score = 0;
    if (!(file >> score) || score < 0) {
        score = 0;
    }
    return score;
}

// Save the best score. If the file cannot be written, for example on a
// read only disk, the score is simply not saved.
void saveHighScore(int score) {
    std::ofstream file(HIGH_SCORE_FILE);
    file << score;
}

// Get the current time in milliseconds
long long getTimeMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// Get the time in milliseconds between automatic drops for a level
int getDropInterval(int level) {
    // Start at 500 ms and get 50 ms faster each level
    int interval = 500 - (level - 1) * 50;

    // Never drop faster than every 100 ms
    if (interval < 100) {
        interval = 100;
    }
    return interval;
}

// Copy one 4x4 shape into another
void copyShape(const int from[4][4], int to[4][4]) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            to[y][x] = from[y][x];
        }
    }
}

// Copy a tetromino into the active shape
void loadPiece(int piece) {
    copyShape(TETROMINOES[piece], currentShape);
}

// Refill the bag with all 7 pieces in a random order
void fillBag() {
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

    int previousHeld = heldPiece;
    heldPiece = currentPiece;

    if (previousHeld == EMPTY_HOLD) {
        // Nothing was held yet, so carry on with the next piece
        currentPiece = nextPiece;
        nextPiece = takePieceFromBag();
    }
    else {
        currentPiece = previousHeld;
    }

    // The swapped in piece starts at the top again
    currentX = SPAWN_X;
    currentY = 0;
    loadPiece(currentPiece);

    holdUsed = true;
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

// Draw the held piece panel below the stats panel
void drawHold() {
    int holdX = BOARD_X - 14;
    int holdY = BOARD_Y + 13;

    mvprintw(holdY, holdX, "+--------+");
    mvprintw(holdY + 1, holdX, "|  HOLD  |");
    mvprintw(holdY + 2, holdX, "+--------+");

    // Draw empty hold area
    for (int y = 0; y < 4; y++) {
        mvprintw(holdY + 3 + y, holdX, "|        |");
    }
    mvprintw(holdY + 7, holdX, "+--------+");

    // Nothing to show until the player holds a piece
    if (heldPiece == EMPTY_HOLD) {
        return;
    }

    attron(COLOR_PAIR(heldPiece + 1));
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (TETROMINOES[heldPiece][y][x] == 1) {
                mvprintw(holdY + 3 + y, holdX + 1 + x * 2, "[]");
            }
        }
    }
    attroff(COLOR_PAIR(heldPiece + 1));
}

// Draw the next piece preview panel
void drawNextPiece() {
    int previewX = BOARD_X + BOARD_WIDTH * 2 + 4;
    int previewY = BOARD_Y;

    mvprintw(previewY, previewX, "+--------+");
    mvprintw(previewY + 1, previewX, "|  NEXT  |");
    mvprintw(previewY + 2, previewX, "+--------+");

    // Draw empty preview area
    for (int y = 0; y < 4; y++) {
        mvprintw(previewY + 3 + y, previewX, "|        |");
    }
    mvprintw(previewY + 7, previewX, "+--------+");

    // Draw the next piece centered inside the box
    attron(COLOR_PAIR(nextPiece + 1));
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (TETROMINOES[nextPiece][y][x] == 1) {
                mvprintw(previewY + 3 + y, previewX + 1 + x * 2, "[]");
            }
        }
    }
    attroff(COLOR_PAIR(nextPiece + 1));
}

// Draw the controls panel below the next piece preview
void drawControls() {
    int controlsX = BOARD_X + BOARD_WIDTH * 2 + 4;
    int controlsY = BOARD_Y + 9;

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

// Check if the current shape can be at the given position
bool isValidPosition(int posX, int posY) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
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

// Draw the current falling piece
void drawPiece() {
    attron(COLOR_PAIR(currentPiece + 1));
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
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
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (currentShape[y][x] == 1) {
                int screenX = BOARD_X + (currentX + x) * 2;
                int screenY = BOARD_Y + ghostY + y;
                mvprintw(screenY, screenX, "::");
            }
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
    int temp[4][4] = { 0 };

    // Transpose and reverse to rotate clockwise
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            temp[x][3 - y] = currentShape[y][x];
        }
    }

    // Keep a copy of the old shape, then apply the rotation
    int backupShape[4][4];
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

// Pause the game until P is pressed again
// Returns true if the player pressed ESC to quit instead
bool pauseGame() {
    mvprintw(BOARD_Y + BOARD_HEIGHT / 2, BOARD_X + BOARD_WIDTH - 3, "PAUSED");
    refresh();

    // Wait for P to resume or ESC to quit
    timeout(-1);
    int key = 0;
    while (key != 'p' && key != 'P' && key != 27) {
        key = getch();
    }
    timeout(INPUT_WAIT_MS);

    return key == 27;
}

// Put the board in the middle of the terminal
void centerBoard() {
    int termHeight, termWidth;
    getmaxyx(stdscr, termHeight, termWidth);
    BOARD_X = (termWidth / 2) - BOARD_WIDTH;
    BOARD_Y = (termHeight / 2) - (BOARD_HEIGHT / 2);
}

// Deal with the terminal being resized while playing: pick up the new size,
// ask for a bigger window while it is too small, then center the board again
void handleResize() {
#ifdef PDCURSES
    // PDCurses needs to be told to pick up the new size. ncurses does it itself.
    resize_term(0, 0);
#endif

    int termHeight, termWidth;
    getmaxyx(stdscr, termHeight, termWidth);

    while (termWidth < MIN_TERM_WIDTH || termHeight < MIN_TERM_HEIGHT) {
        erase();
        mvprintw(0, 0, "Window is %d x %d", termWidth, termHeight);
        mvprintw(1, 0, "Please make it at least %d x %d", MIN_TERM_WIDTH, MIN_TERM_HEIGHT);
        refresh();

        getch();
#ifdef PDCURSES
        resize_term(0, 0);
#endif
        getmaxyx(stdscr, termHeight, termWidth);
    }

    centerBoard();
}

// Display title screen before the game starts
void showTitleScreen(int highScore) {
    int termHeight, termWidth;
    getmaxyx(stdscr, termHeight, termWidth);
    int centerX = termWidth / 2;
    int centerY = termHeight / 2;

    clear();
    mvprintw(centerY - 4, centerX - 10, "+-------------------+");
    mvprintw(centerY - 3, centerX - 10, "|                   |");
    mvprintw(centerY - 2, centerX - 10, "|      TETRIS       |");
    mvprintw(centerY - 1, centerX - 10, "|                   |");
    mvprintw(centerY, centerX - 10, "|   Built with C++  |");
    mvprintw(centerY + 1, centerX - 10, "|    and PDCurses   |");
    mvprintw(centerY + 2, centerX - 10, "|                   |");
    mvprintw(centerY + 3, centerX - 10, "+-------------------+");
    mvprintw(centerY + 5, centerX - 10, "Best score: %d", highScore);
    mvprintw(centerY + 6, centerX - 10, "Press any key to start");
    refresh();

    // Wait for keypress
    timeout(-1);
    getch();
    timeout(INPUT_WAIT_MS);
}

// Play one game from an empty board, putting the final score in finalScore
// Returns true if the game ended with game over, false if the player quit with ESC
bool playGame(int& finalScore) {
    // Start with an empty board
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x] = 0;
        }
    }

    // Game state
    int score = 0;
    int lines = 0;
    int level = 1;
    bool gameOver = false;

    // Time of the last automatic drop
    long long lastDropTime = getTimeMs();

    // Whether the piece is resting on the ground, and since when
    bool landed = false;
    long long landedTime = 0;

    // Start each game with a fresh bag of pieces
    bagIndex = PIECE_COUNT;

    // Nothing is held at the start of a game
    heldPiece = EMPTY_HOLD;
    holdUsed = false;

    // Pick the first and next pieces
    currentPiece = takePieceFromBag();
    nextPiece = takePieceFromBag();

    // Load the first piece at the top of the board
    currentX = SPAWN_X;
    currentY = 0;
    loadPiece(currentPiece);

    while (!gameOver) {
        // Draw everything
        // erase() only redraws what changed, which avoids flicker
        erase();
        drawBoard();
        drawGhost();
        drawPiece();
        drawStats(score, lines, level);
        drawHold();
        drawNextPiece();
        drawControls();
        refresh();

        // Handle input first
        int key = getch();

        // Set when the player hard drops, so the piece locks right away
        bool hardDropped = false;

        // 27 is the Escape key
        if (key == 27) {
            break;
        }
        else if (key == 'a' || key == 'A' || key == KEY_LEFT) {
            if (isValidPosition(currentX - 1, currentY)) {
                currentX--;
            }
        }
        else if (key == 'd' || key == 'D' || key == KEY_RIGHT) {
            if (isValidPosition(currentX + 1, currentY)) {
                currentX++;
            }
        }
        else if (key == 's' || key == 'S' || key == KEY_DOWN) {
            if (isValidPosition(currentX, currentY + 1)) {
                currentY++;
                score += SOFT_DROP_POINTS;
            }
        }
        else if (key == 'w' || key == 'W' || key == KEY_UP) {
            rotatePiece();
        }
        else if (key == ' ') {
            // Hard drop: move the piece straight down as far as it can go
            int rowsDropped = 0;
            while (isValidPosition(currentX, currentY + 1)) {
                currentY++;
                rowsDropped++;
            }

            score += rowsDropped * HARD_DROP_POINTS;
            hardDropped = true;
        }
        else if (key == 'c' || key == 'C') {
            holdPiece();
        }
        else if (key == KEY_RESIZE) {
            handleResize();

            // Time passed while the window was being resized, so start the timers again
            lastDropTime = getTimeMs();
            landed = false;
        }
        else if (key == 'p' || key == 'P') {
            if (pauseGame()) {
                break;
            }

            // Don't count the time spent paused towards the next automatic drop
            lastDropTime = getTimeMs();
        }

        long long now = getTimeMs();

        // The piece is resting when it cannot move down any further
        bool resting = !isValidPosition(currentX, currentY + 1);

        // Start the lock delay when the piece lands, and cancel it if it moves off again
        if (resting && !landed) {
            landed = true;
            landedTime = now;
        }
        else if (!resting) {
            landed = false;
        }

        // Drop the piece once enough time has passed, no matter how many keys were pressed
        if (!resting && now - lastDropTime >= getDropInterval(level)) {
            lastDropTime = now;
            currentY++;
        }

        // Lock the piece after a hard drop, or once it has rested for the lock delay
        if (hardDropped || (landed && now - landedTime >= LOCK_DELAY_MS)) {
            // Place piece on board
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    if (currentShape[y][x] == 1) {
                        board[currentY + y][currentX + x] = currentPiece + 1;
                    }
                }
            }

            // Clear completed lines and update score
            int cleared = clearLines();
            if (cleared > 0) {
                lines += cleared;

                // Reward multi-line clears more
                score += LINE_SCORES[cleared] * level;

                // Go up a level every 10 lines
                level = lines / 10 + 1;
            }

            // Spawn next piece
            currentPiece = nextPiece;
            currentX = SPAWN_X;
            currentY = 0;
            loadPiece(currentPiece);
            nextPiece = takePieceFromBag();
            holdUsed = false;

            // Check game over
            if (!isValidPosition(currentX, currentY)) {
                gameOver = true;
            }

            // The next piece starts its own drop and lock timers
            landed = false;
            lastDropTime = now;
        }
    }

    finalScore = score;
    return gameOver;
}

// Show the game over message and wait for the player's choice
// Returns true if the player wants to play again
bool askPlayAgain(int score, int highScore, bool newBest) {
    int centerY = BOARD_Y + BOARD_HEIGHT / 2;

    mvprintw(centerY, BOARD_X + BOARD_WIDTH - 5, "GAME OVER");
    mvprintw(centerY + 2, BOARD_X + BOARD_WIDTH - 6, "Score: %-6d", score);

    if (newBest) {
        mvprintw(centerY + 3, BOARD_X + BOARD_WIDTH - 6, "NEW BEST!   ");
    }
    else {
        mvprintw(centerY + 3, BOARD_X + BOARD_WIDTH - 6, "Best: %-6d", highScore);
    }

    mvprintw(centerY + 5, BOARD_X + BOARD_WIDTH - 7, "R - Play again");
    mvprintw(centerY + 6, BOARD_X + BOARD_WIDTH - 5, "ESC - Quit");
    refresh();

    // Wait for R to play again or ESC to quit
    timeout(-1);
    int key = 0;
    while (key != 'r' && key != 'R' && key != 27) {
        key = getch();
    }
    timeout(INPUT_WAIT_MS);

    return key == 'r' || key == 'R';
}

int main() {
    // Initialize PDCurses
    initscr();
    raw();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(INPUT_WAIT_MS);
    initColors();

    // Get terminal size
    int termHeight, termWidth;
    getmaxyx(stdscr, termHeight, termWidth);

    // Stop if the window is too small, otherwise the panels end up off screen
    if (termWidth < MIN_TERM_WIDTH || termHeight < MIN_TERM_HEIGHT) {
        endwin();
        printf("This window is %d columns by %d rows.\n", termWidth, termHeight);
        printf("Tetris needs at least %d by %d, so please make it bigger.\n",
            MIN_TERM_WIDTH, MIN_TERM_HEIGHT);
        return 1;
    }

    // Show title screen with the best score so far
    int highScore = loadHighScore();
    showTitleScreen(highScore);

    // Center the board
    centerBoard();

    // Seed the random generator so each game has a different piece order
    srand(static_cast<unsigned int>(time(nullptr)));

    // Keep starting new games until the player quits
    bool playAgain = true;
    while (playAgain) {
        int score = 0;
        bool gameOver = playGame(score);

        // Only ask to play again if the game ended, not when the player quit with ESC
        if (gameOver) {
            bool newBest = score > highScore;
            if (newBest) {
                highScore = score;
                saveHighScore(highScore);
            }

            playAgain = askPlayAgain(score, highScore, newBest);
        }
        else {
            playAgain = false;
        }
    }

    endwin();
    return 0;
}