#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int debug = 0;
static int nc = 1;
static int waitkey = 0;

struct progress {
    char * buf;
    int width;
    char foo, bar;
};

int initProgress(struct progress *p, int width, char foo, char bar) {
    if (width < 1) {
        return -1;
    }
    p->buf = malloc(width);   /* TODO can we eliminate extra allocated byte */
    if (p->buf == NULL) {
        return -1;
    }
    if (debug) memset(p->buf, '?', width); /* debug: do not need except during debug */
    p->buf[width-2] = '%';      /* will always be a percent symbol */
    p->buf[width-1] = 0;        /* last byte NULL for end of string */
    p->foo = foo;
    p->bar = bar;
    p->width = width;
    return 0;
}

struct progress * mallocProgress(int width, char foo, char bar) {
    struct progress * p = (struct progress *) malloc(sizeof(struct progress));
    if (p == NULL) {
        return NULL;
    }
    if (initProgress(p, width, foo, bar) == -1) {
        free(p);
        return NULL;
    }
    return p;
};

void freeProgress(struct progress * p) {
    free(p->buf);
}

void progressDisplay(struct progress * p, const char *message, const int row, int percentage) {
    int lm = strlen(message);
    /* TODO: handle case when message longer than cols */

    int reverse = p->width * percentage / 100; /* 80 * 50 / 100 = 40 */

    int idx = p->width - 3;

    do {
        p->buf[idx] = (percentage % 10) + '0';
        percentage /= 10;
        idx--;
    } while (percentage > 0);

    memcpy(p->buf, message, lm);
    // 0123456789012345678901234567890123456789
    // please wait...                      13%. <- NULL terminator
    //                                    ^ idx = 35
    // ***
    //   int spaces = idx - lm + 1; // = 35 - 14 + 1 = 22
    int f = reverse - lm;
    if (f > 0) {
        // NOTE: f could be too large, beyond the percentage point
        int ff = idx - f;
        if (ff > 0) {
            memset(p->buf + lm, debug ? 'f' : ' ', ff);
        }
    }


    // p->width - 1 = NULL
    // p->width - 2 = '%'
    // p->width - 3 = 3
    // p->width - 4 = 1
    // idx = 35
    // lm = 14
    // idx = p->width - 5 = 40 - 5 = 35
    //
    // reverse = 12:
    // reverse = 14:
    // reverse = 15;
    // reverse = 34:
    // reverse = 35:
    // reverse = 36:

    if (0) {
        for (int i = lm; i <= idx; i++) {
            p->buf[i] = i < reverse ? p->foo : p->bar;
        }
    } else if (0) {
        int spaces = idx - lm + 1;
        int leftCount = spaces; // - reverse;
        if (leftCount > 0) memset(p->buf + lm, 'f', leftCount);
        int rightCount = 0;
        if (rightCount > 0) memset(p->buf + lm + spaces, 'r', rightCount);
    }

    /* memset(p->buf + lm + idx + 1, p->bar, p->width - idx - 1); */
    /* memset(p->buf + lm + idx + 1 + 10, p->bar, p->width - idx - 20); */

    char t;

    if (reverse < p->width) {
        t = p->buf[reverse];         /* save char in buffer */
        p->buf[reverse] = 0;         /* replace with NULL for end of string */
        if (nc) {
            attron(A_REVERSE);
            mvprintw(row, 0, "%s", p->buf);
        } else {
            printf("%s*", p->buf);
        }
        if (nc) {
            attroff(A_REVERSE);
            p->buf[reverse] = t;    /* replace temporary NULL with saved char */
            mvprintw(row, reverse, "%s", p->buf+reverse);
        } else {
            printf("%s\n", p->buf+reverse);
        }
    } else {
        if (nc) {
            attron(A_REVERSE);
            mvprintw(row, 0, "%s", p->buf); /* entire buffer is printed in reverse */
            attroff(A_REVERSE);
        } else {
            printf("%s*\n", p->buf);
        }
    }

    if (nc) refresh();
}

int main(int argc, char **argv) {
    int x, y;
    int rows, cols;

    char *message = strdup(argv[argc-1]);
    if (message == NULL) {
        fputs("malloc failure\n", stderr);
        exit(1);
    }

    cols=101;

    if (nc) {
        initscr();
        getmaxyx(stdscr, rows, cols);
    }

    struct progress * p;
    p = mallocProgress(cols, ' ', ' ');
    if (p == NULL) {
        fprintf(stderr, "cannot mallocProgress.\n");
        return 1;
    }

    if (nc)
        getyx(stdscr, y, x);

    for (int i = 0; i <= 100; i++) {
        progressDisplay(p, message, y, i);
        if (waitkey)
            getch();
        else
            usleep(50000); /* 0.05 second */
    }

    if (nc) {
        addch('\n');
        printw("hit any key to exit");
        getch();
        endwin();
    }

    freeProgress(p);
    free(p);
    free(message);
    return 0;
}
