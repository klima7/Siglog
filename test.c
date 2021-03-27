#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include "siglog.h"

char* dump_handler();

char a = 1;
short b = 1;
int c = 1;
long d = 1;

float e = 1;
double f = 1;

unsigned char g = 1;
unsigned short h = 1;
unsigned int i = 1;
unsigned long j = 1;

void dump_function(FILE *file) {
    fprintf(file, "a: %c\n", a);
    fprintf(file, "b: %d\n", b);
    fprintf(file, "c: %d\n", c);
}

int main() {

    // Signal 34
    printf("pid=%d\n", getpid());

    int res = siglog_init(-1, -1, SIGLOG_DISABLED, "file.txt");
    assert(res == 0);

    siglog_register_dump_function(dump_function);

    sleep(2);



    while(1) {
        siglog_max("Hello there %d", 4);
        siglog_standard("Hello there %d", 4);
        siglog_min("Hello there %d", 4);
        a++;
        b++;
        c++;
        d++;
        e++;
        f++;
        g++;
        h++;
        i++;
        j++;
        sleep(2);
    }

    siglog_free();
    return 0;
}