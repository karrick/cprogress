#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct progress {
    char *formatted;         /* NULL terminated string ready to use by client */
    int _size;               /* not for client: size of malloc */
    int _width;              /* not for client: number of columns to fill */
    char _spinner;            /* not for client: used when updating the ticker */
};

static int debug = 0;

const char cr[] = "\033[G";
const char prefix[] = "\033[G\033[7m";
const char suffix[] = "\033[27m";
const char spinner[] = "-\\|/";
const int crlen = sizeof(cr) - 1;
const int plen = sizeof(prefix) - 1;
const int slen = sizeof(suffix) - 1;
const int splen = sizeof(spinner) - 1;

int initProgress(struct progress *p, int width) {
    if (width < 1) {
        return -1;
    }
    int size = width + plen + slen + 1;
    p->formatted = malloc(size);   /* 3 bytes for column 1 + 4 bytes for reverse + 5 bytes for reverse off */
    if (p->formatted == NULL) {
        return -1;
    }
    if (debug) memset(p->formatted, '?', size);

    // Initialize for display before update is called for the first time.
    /* memset(p->formatted, 0, size); /\* prevent valgrind warning *\/ */
    /* p->formatted[0] = 0;              // NULL terminator */
    /* if (debug) p->formatted[size-1] = 0; // NULL terminator (??? not sure whether should be in init or each update) */
    p->_size = size;
    p->_width = width;
    if (debug > 1) fprintf(stderr, "p->_width: %d; p->_size: %d\n", p->_width, p->_size);

    return 0;
}

struct progress * mallocProgress(int width) {
    struct progress *p = (struct progress *) malloc(sizeof(struct progress));
    if (p == NULL) {
        return NULL;
    }
    if (initProgress(p, width) == -1) {
        free(p);
        return NULL;
    }
    return p;
}

void doneProgress(struct progress *p) {
    free(p->formatted);
}

void displayProgress(struct progress *p, FILE * stream) {
    fputs(p->formatted, stream);
    fflush(stream);
}

// appendPercentage appends the percentage indictation at right edge of buffer.
void appendPercentage(struct progress *p, const int percentage, int foo, char * start) {
    *start = '%';
    foo--;
    int pc = percentage;
    do {
        start--;
        if (foo == 0) {
            memcpy(start - slen + 1, suffix, slen);
            start -= slen;
        }
        --foo;

        *start = (pc % 10) + '0';
        pc /= 10;
    } while (pc > 0);
}

/* updatePercentage updates p->formatted to reflect percentage complete. */
void updatePercentage(struct progress *p, const char *message, const int percentage) {
    if (debug) memset(p->formatted, '?', p->_size-1);
    if (debug) p->formatted[p->_size-1] = 0; /* NULL terminator */

    char *buf = p->formatted;

    /* All percentage updates start with reverse video prefix. */
    memcpy(buf, prefix, plen);
    buf += plen;

    /* Determine how many characters the percentage portion of the status will
       consume and how many characters are remaining for the message. */
    int pc = percentage;
    int pclen = 1;                 /* percent sign consumes one byte */
    int mcap = p->_width - 1;       /* percent sign consumes one byte */
    do {
        pc /= 10;
        mcap--;
        pclen++;
    } while (pc > 0);
    /* POST: pclen is number of bytes percentage indication will require,
       including the percent sign. mcap is the maximum number of bytes available
       to the left of the percentage indication. */

    /* Reverse video count: 80 columns * 50 percent / 100 = 40 columns. Handle
       when percentage is larger than 100. */
    int rc = percentage < 100 ? p->_width * percentage / 100 : p->_width;
    int lmessage = strlen(message);
    int lblanks = mcap - lmessage;

    if (debug > 1) fprintf(stderr, "\npercentage: %3d; pclen: %d; mcap: %2d; lmessage: %2d; rc: %3d; lblanks: %d\n", percentage, pclen, mcap, lmessage, rc, lblanks);

    if (lblanks < 0) {
        if (debug > 1) fprintf(stderr, "TODO: trim message too long\n");
        abort();
    }

    int mi = lmessage - rc;     /* 16 total message - 3 reverse video = 13 after reverse */
    char * pstart = p->formatted + p->_size - 2;

    if (mi > 0) {
        if (debug > 1) fprintf(stderr, "split in message: %ld\n", buf - p->formatted);

        memcpy(buf, message, rc);
        buf += rc;

        memcpy(buf, suffix, slen);
        buf += slen;

        memcpy(buf, message+rc, mi);
        buf += mi;

        memset(buf, debug ? 'a' : ' ', lblanks);
        buf += lblanks;

        appendPercentage(p, percentage, -1, pstart);
    } else if (rc == lmessage) {
        if (debug > 1) fprintf(stderr, "split after message; before spaces: %ld\n", buf - p->formatted);

        memcpy(buf, message, lmessage);
        buf += lmessage;

        memcpy(buf, suffix, slen);
        buf += slen;

        memset(buf, debug ? 'b' : ' ', lblanks);
        buf += lblanks;

        appendPercentage(p, percentage, -1, pstart);
    } else if (rc < mcap) {
        if (debug > 1) fprintf(stderr, "split in spaces: %ld\n", buf - p->formatted);

        memcpy(buf, message, lmessage);
        buf += lmessage;

        memset(buf, debug ? 'c' : ' ', -mi);
        buf -= mi;

        memcpy(buf, suffix, slen);
        buf += slen;

        memset(buf, debug ? 'd' : ' ', lblanks + mi);
        buf += lblanks + mi;

        appendPercentage(p, percentage, -1, pstart);
    } else if (rc == mcap) {
        if (debug > 1) fprintf(stderr, "split after spaces; before percentage: %ld\n", buf - p->formatted);

        memcpy(buf, message, lmessage);
        buf += lmessage;

        memset(buf, debug ? 'e' : ' ', lblanks);
        buf += lblanks;

        memcpy(buf, suffix, slen);
        buf += slen;

        appendPercentage(p, percentage, -1, pstart);
    } else if (rc < (mcap + pclen)) {
        int foo = pclen - rc + mcap;
        if (debug > 1) fprintf(stderr, "split in percentage: %d\n", foo);

        memcpy(buf, message, lmessage);
        buf += lmessage;

        memset(buf, debug ? 'f' : ' ', lblanks);
        buf += lblanks;

        memcpy(buf, suffix, slen);
        buf += slen;

        appendPercentage(p, percentage, foo, pstart);
    } else {
        if (debug > 1) fprintf(stderr, "all reverse video\n");

        memcpy(buf, message, lmessage);
        buf += lmessage;

        memset(buf, debug ? 'g' : ' ', lblanks);
        buf += lblanks;

        appendPercentage(p, percentage, -1, p->formatted + p->_size - slen - 2);
        buf += pclen;

        memcpy(buf, suffix, slen);
        buf += slen;
    }

    buf = 0;
}

void updateSpinner(struct progress *p, const char *message) {
    char c = spinner[p->_spinner];
    p->_spinner++;
    if (p->_spinner == splen) p->_spinner = 0;

    if (debug) memset(p->formatted, '?', p->_size-1);

    memcpy(p->formatted, cr, crlen);

    int lmessage = strlen(message);
    memcpy(p->formatted + crlen, message, lmessage);

    // Doing some other stuff: 9.................................................. \

    if (debug > 1) fprintf(stderr, "\np->width: %d\n", p->_width);
    int mcap = p->_width - 1;       /* percent sign consumes one byte */
    int lblanks = mcap - lmessage;

    if (debug > 1) fprintf(stderr, "\nmcap: %2d; lmessage: %2d; lblanks: %d\n", mcap, lmessage, lblanks);

    if (lblanks < 0) {
        if (debug > 1) fprintf(stderr, "TODO: trim message too long\n");
        abort();
    }

    memset(p->formatted + crlen + lmessage, debug ? '.' : ' ', lblanks);

    p->formatted[crlen + lmessage + lblanks] = c;
    p->formatted[crlen + lmessage + lblanks + 1] = 0;
}

int main(int argc, char **argv) {
    int cols;

    char *message = strdup(argv[argc-1]);
    if (message == NULL) {
        fputs("malloc failure\n", stderr);
        exit(1);
    }

    cols = 80;

    struct progress * p;
    p = mallocProgress(cols);
    if (p == NULL) {
        fprintf(stderr, "cannot mallocProgress.\n");
        return 1;
    }

    if (debug) {
        for (int i = 0; i < cols; i++) {
            putchar('-');
        }
        putchar('\n');
        printf("%s\n", p->formatted);
    }

    int waitkey = 0;

    if (0) {
        for (int i = 0; i < 2; i++) {
            updatePercentage(p, message, i);
            printf("%s", p->formatted);
            waitkey ? getchar() : usleep(100000);
        }
        for (int i = 9; i < 19; i++) {
            updatePercentage(p, message, i);
            printf("%s", p->formatted);
            waitkey ? getchar() : usleep(100000);
        }
        for (int i = 20; i < 90; i+=10) {
            updatePercentage(p, message, i);
            printf("%s", p->formatted);
            waitkey ? getchar() : usleep(100000);
        }
        for (int i = 95; i <= 101; i++) {
            updatePercentage(p, message, i);
            printf("%s", p->formatted);
            waitkey ? getchar() : usleep(100000);
        }
    } else if (1) {
        for (int i = 0; i <= 100; i++) {
            updatePercentage(p, message, i);
            displayProgress(p, stdout);
            waitkey ? getchar() : usleep(20000);
        }
    }
    putchar('\n');

    for (int i = 0; i <= 100; i++) {
        char *m;
        asprintf(&m, "Doing some other stuff: %d", i);
        updateSpinner(p, m);
        displayProgress(p, stdout);
        waitkey ? getchar() : usleep(20000);
        free(m);
    }
    putchar('\n');

    doneProgress(p);
    free(p);
    free(message);
    return 0;
}
