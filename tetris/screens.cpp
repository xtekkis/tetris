// The screens outside the game loop: the title screen, the pause screen,
// the game over screen, and what to do when the window is resized.

#include <curses.h>

#include "game.h"

// Highest level the player can start on
const int MAX_START_LEVEL = 10;

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

// Pause the game until P is pressed again
// Returns true if the player pressed ESC to quit instead
bool pauseGame() {
    // Wait for P to resume or ESC to quit
    timeout(-1);

    while (true) {
        mvprintw(BOARD_Y + BOARD_HEIGHT / 2, BOARD_X + BOARD_WIDTH - 3, "PAUSED");
        refresh();

        int key = getch();

        if (key == 27) {
            timeout(INPUT_WAIT_MS);
            return true;
        }
        else if (key == 'p' || key == 'P') {
            timeout(INPUT_WAIT_MS);
            return false;
        }
        else if (key == KEY_RESIZE) {
            // The board moves, so clear the old picture and show PAUSED in its new place
            handleResize();
            erase();
        }
    }
}

// Display title screen before the game starts
// Returns the level to start on, or 0 if the player quit with ESC
int showTitleScreen(int highScore) {
    int startLevel = 1;

    // Wait for the player to choose a level and start
    timeout(-1);
    while (true) {
        // Read the size every time, so the title stays centered after a resize
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
        mvprintw(centerY + 1, centerX - 10, "|    and curses     |");
        mvprintw(centerY + 2, centerX - 10, "|                   |");
        mvprintw(centerY + 3, centerX - 10, "+-------------------+");
        mvprintw(centerY + 5, centerX - 10, "Best score: %d", highScore);
        mvprintw(centerY + 7, centerX - 10, "Start level: %-2d", startLevel);
        mvprintw(centerY + 8, centerX - 10, "A and D change the level");
        mvprintw(centerY + 9, centerX - 10, "ENTER starts, ESC quits");
        refresh();

        int key = getch();

        if (key == 27) {
            timeout(INPUT_WAIT_MS);
            return 0;
        }
        else if (key == '\n' || key == '\r' || key == KEY_ENTER || key == ' ') {
            break;
        }
        else if ((key == 'a' || key == 'A' || key == KEY_LEFT) && startLevel > 1) {
            startLevel--;
        }
        else if ((key == 'd' || key == 'D' || key == KEY_RIGHT) && startLevel < MAX_START_LEVEL) {
            startLevel++;
        }
        else if (key == KEY_RESIZE) {
            handleResize();
        }
    }

    timeout(INPUT_WAIT_MS);
    return startLevel;
}

// Show the game over message and wait for the player's choice
// Returns true if the player wants to play again
bool askPlayAgain(int score, int highScore, bool newBest) {
    // Wait for R to play again or ESC to quit
    timeout(-1);

    while (true) {
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

        int key = getch();

        if (key == 'r' || key == 'R') {
            timeout(INPUT_WAIT_MS);
            return true;
        }
        else if (key == 27) {
            timeout(INPUT_WAIT_MS);
            return false;
        }
        else if (key == KEY_RESIZE) {
            // The board moves, so clear the old picture and draw it in its new place
            handleResize();
            erase();
            drawBoard();
        }
    }
}
