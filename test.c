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

int main() {

    // Signal 34
    printf("pid=%d\n", getpid());

    int res = siglog_init(-1, -1, SIGLOG_INFO, "file.txt");
    assert(res == 0);

    siglog_dump_char("char", &a);
    siglog_dump_short("short", &b);
    siglog_dump_int("int", &c);
    siglog_dump_long("long", &d);

    siglog_dump_float("float", &e);
    siglog_dump_double("double", &f);

    siglog_dump_uchar("unsigned char", &g);
    siglog_dump_ushort("unsigned short", &h);
    siglog_dump_uint("unsigned int", &i);
    siglog_dump_ulong("unsigned long", &j);

    sleep(2);



    while(1) {
        siglog_info("Hello there %d", 4);
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