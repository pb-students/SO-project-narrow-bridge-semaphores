#include "urandom.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int urandom_fd = -2;

void urandom_init() {
    urandom_fd = open("/dev/urandom", O_RDONLY);

    if (urandom_fd == -1) {
        fprintf(stderr, "Error opening [/dev/urandom]: %i\n", urandom_fd);
        exit(EXIT_FAILURE);
    }
}

unsigned long urandom() {
    unsigned long buf_impl;
    unsigned long *buf = &buf_impl;

    if (urandom_fd == -2) {
        urandom_init();
    }

    read(urandom_fd, buf, sizeof(long));
    return buf_impl;
}
