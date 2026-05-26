#include <curses.h>

int main() {
    initscr();
    printw("PDCurses is working!");
    refresh();
    getch();
    endwin();
    return 0;
}