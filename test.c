#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include "siglog.h"

char* dump_handler();

int var1 = 3;
int var2 = -4;
float float_var = 3.1415;
double double_var = 3.1415;

int main() {

    // Signal 34
    printf("pid=%d\n", getpid());

    int res = siglog_init(-1, -1, SIGLOG_INFO, "file.txt");
    assert(res == 0);

    siglog_trace_int("var1", &var1);
    siglog_trace_int("var2", &var2);
    siglog_trace_float("float_var", &float_var);
    siglog_trace_double("double_var", &double_var);

    sleep(2);



    while(1) {
        siglog_info("Hello there %d", 4);
        usleep(500);
        var1++;
        var2--;
        float_var += 0.001;
        double_var -= 0.001;
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