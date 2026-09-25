// Tetris: the game loop and the scoring that ties the other files together.
// See game.h for how the game is split up.

#include <curses.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <fstream>

#include "game.h"

// How long a piece rests on the ground before it locks in place
const int LOCK_DELAY_MS = 500;

// Points for clearing 1, 2, 3 or 4 lines at once, multiplied by the level
const int LINE_SCORES[5] = { 0, 100, 300, 500, 800 };

// Points per row for dropping a piece yourself, not multiplied by the level
const int SOFT_DROP_POINTS = 1;
const int HARD_DROP_POINTS = 2;

// Where the best score is saved, in the folder the game is run from
const char HIGH_SCORE_FILE[] = "highscore.txt";

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

// Play one game from an empty board, putting the final score in finalScore
// Returns true if the game ended with game over, false if the player quit with ESC
bool playGame(int& finalScore, int startLevel) {
    // Start with an empty board
    clearBoard();

    // Game state
    int score = 0;
    int lines = 0;
    int level = startLevel;
    bool gameOver = false;

    // Time of the last automatic drop
    long long lastDropTime = getTimeMs();

    // Whether the piece is resting on the ground, and since when
    bool landed = false;
    long long landedTime = 0;

    // Start each game with a fresh bag of pieces
    resetBag();

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
        drawScreen(score, lines, level);
        drawGhost();
        drawPiece();
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
            for (int y = 0; y < SHAPE_SIZE; y++) {
                for (int x = 0; x < SHAPE_SIZE; x++) {
                    if (currentShape[y][x] == 1) {
                        board[currentY + y][currentX + x] = currentPiece + 1;
                    }
                }
            }

            // Clear completed lines and update score
            flashFullLines();
            int cleared = clearLines();
            if (cleared > 0) {
                lines += cleared;

                // Reward multi-line clears more
                score += LINE_SCORES[cleared] * level;

                // Go up a level every 10 lines
                level = startLevel + lines / 10;
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

            // The next piece starts its own drop and lock timers.
            // The time is read again because the line flash takes a moment.
            landed = false;
            lastDropTime = getTimeMs();
        }
    }

    // Draw the last board, so the piece that ended the game is visible
    // behind the game over message
    if (gameOver) {
        drawScreen(score, lines, level);
        refresh();
    }

    finalScore = score;
    return gameOver;
}

int main() {
    // Initialize curses
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

    // Show title screen with the best score so far, and let the player pick a level
    int highScore = loadHighScore();
    int startLevel = showTitleScreen(highScore);

    // The player quit from the title screen
    if (startLevel == 0) {
        endwin();
        return 0;
    }

    // Center the board
    centerBoard();

    // Seed the random generator so each game has a different piece order
    srand(static_cast<unsigned int>(time(nullptr)));

    // Keep starting new games until the player quits
    bool playAgain = true;
    while (playAgain) {
        int score = 0;
        bool gameOver = playGame(score, startLevel);

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
