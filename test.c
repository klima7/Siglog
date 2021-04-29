/*
 * This file is a test of siglog library
 * This program consists of two tasks executing in two separate threads to emphasize threadsafety
 *
 *  Logging in this example is by default disabled, to change logging level send signal LEVEL_SIGNAL with value:
 *  0 - disable logging
 *  1 - set logging priority to max (less verbose)
 *  2 - set logging priority to standard
 *  3 - set logging priority to min (most verbose)
 *
 *  To create dump file send signal DUMP_SIGNAL
 *
 *  Log file and dump files will be saved in current directory (in this example, it's adjustable)
 */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include <signal.h>
#include "siglog.h"

#define LEVEL_SIGNAL SIGRTMIN
#define DUMP_SIGNAL SIGRTMIN+1

// -------------- Fibonacci task -------------

int fib_a;
int fib_b;
int fib_c;

// Dump handler used in dump operation
void fibbonacci_dump(FILE *dump) {
    fprintf(dump, "fib_a: %d\n", fib_a);
    fprintf(dump, "fib_b: %d\n", fib_b);
    fprintf(dump, "fib_c: %d\n", fib_c);
}

// Simple thread doing something with Fibbonacci numbers
_Noreturn void *fibbonacci_thread(void *arg) {
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


// --------------- Prime task ----------------

int primes_count;
int prime_next;

// Dump handler used in dump operation
void prime_dump(FILE *dump) {
    fprintf(dump, "primes_count: %d\n", primes_count);
    fprintf(dump, "prime_next: %d\n", prime_next);
}

// Simple thread doing something with prime numbers
_Noreturn void *prime_thread(void *arg) {
    siglog_register_dump_function(prime_dump);
    siglog_standard("Starting searching prime numbers");
    prime_next = 2;

    while(1) {
        siglog_min("Checking whether number %d is prime", prime_next);
        int is_prime = 1;
        for(int i=2; i<=sqrt(prime_next); i++) {
            if(prime_next % i == 0) {
                is_prime = 0;
                break;
            }
        }

        if(is_prime) {
            siglog_max("Prime number %d found", prime_next);
            primes_count++;
        }

        siglog_min("Incrementing prime_next");
        prime_next++;
        sleep(1);
    }
}


// ------------------- main --------------------

int main() {
    printf("pid=%d; level change signal: %d; dump signal: %d\n", getpid(), LEVEL_SIGNAL, DUMP_SIGNAL);

    // Init library
    int err = siglog_init(LEVEL_SIGNAL, DUMP_SIGNAL, SIGLOG_DISABLED, NULL);
    assert(err == 0);

    // Create threads
    pthread_t fibonacci_thread_tid, prime_thread_tid;
    pthread_create(&fibonacci_thread_tid, NULL, fibbonacci_thread, NULL);
    pthread_create(&prime_thread_tid, NULL, prime_thread, NULL);

    // Join threads
    pthread_join(fibonacci_thread_tid, NULL);
    pthread_join(prime_thread_tid, NULL);

    // Free library resources
    siglog_free();

    return 0;
}