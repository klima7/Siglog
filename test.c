#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include "siglog.h"


// -------- Fibbonacci task -------

int fib_a;
int fib_b;
int fib_c;

void fibbonacci_dump(FILE *dump) {
    fprintf(dump, "fib_a: %d\n", fib_a);
    fprintf(dump, "fib_b: %d\n", fib_b);
    fprintf(dump, "fib_c: %d\n", fib_c);
}

void *fibbonacci_thread(void *arg) {
    siglog_register_dump_function(fibbonacci_dump);

    siglog_standard("Starting calculating fibonacci numbers");
    fib_a = 1;
    fib_b = 1;

    while(1) {
        fib_c = fib_a + fib_b;
        siglog_standard("Next fibonacci number is %d", fib_c);

        sleep(1);

        fib_a = fib_b;
        fib_b = fib_c;
        siglog_min("Fibonacci variables swaped");

        if(fib_c > 1000) {
            siglog_max("Reseting fibonacci variables");
            fib_a = 1;
            fib_b = 1;
        }

        sleep(1);
    }
}

// --------- prime task ----------

int prime_next;

void prime_dump(FILE *dump) {
    fprintf(dump, "prime_next: %d\n", prime_next);
}

int is_prime(int num) {
    for(int i=2; i<=sqrt(num); i++) {
        if(num % i == 0)
            return 1;
    }
    return 0;
}

void *prime_thread(void *arg) {
    siglog_register_dump_function(prime_dump);
    siglog_standard("Starting searching prime numbers");
    prime_next = 2;
    while(1) {
        siglog_min("Checking whether number %d is prime", prime_next);
        if(is_prime(prime_next)) {
            siglog_max("Prime number %d found", prime_next);
        }
        prime_next++;
        sleep(1);
    }
}

// ------------- main --------------

int main() {

    pthread_t fibonacci_thread_tid, prime_thread_tid;

    // Signal 34
    printf("pid=%d\n", getpid());

    siglog_init(-1, -1, SIGLOG_STANDARD, NULL);

    // Create threads
    pthread_create(&fibonacci_thread_tid, NULL, fibbonacci_thread, NULL);
    pthread_create(&prime_thread_tid, NULL, prime_thread, NULL);

    // Join threads
    while(1) sleep(1);

    return 0;
}