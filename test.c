#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include "siglog.h"

char* dump_handler();

int main() {

    // Signal 34
    printf("pid=%d\n", getpid());
    printf("main: %d", pthread_self());

    int res = siglog_init(-1, -1, SIGLOG_LVL_STD, "file.txt");
    assert(res == 0);
    siglog_register_dump_handler(dump_handler);

    sleep(2);



    while(1) {
        siglog_std("Hello there %d", 4);
        sleep(1);
    }

    siglog_free();
    return 0;
}

char* dump_handler() {
    char *str = malloc(1000);
    snprintf(str, 100, "Zmienna 1: %d\n"
                       "Zmienna 2: %c"
                       , 4,
                       6);
    return str;
}