#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include "siglog.h"

#define DATE_FORMAT "%m/%d/%y %H:%M:%S"

// Global variables
static int initialized;
static char *logging_dir_path;
static SIGLOG_LEVEL current_level;
static int level_signal_no, dump_signal_no;
static pthread_t level_tid, dump_tid;
static FILE *log_file;

// Variables to handle dump
static DUMP_FUNCTION *dump_functions;
static int dump_functions_size;
static int dump_functions_capacity;

// Variables to ensure threadsafety of logging and adding dump functions concurrently
static pthread_mutex_t log_mutex;
static pthread_mutex_t dump_mutex;

// Prototypes of file scoped functions
static void* level_thread(void* arg);
static void level_handler(int signo, siginfo_t *info, void *other);
static void* dump_thread(void* arg);
static void dump_handler();
static FILE * create_dump_file();
static char *str_level(SIGLOG_LEVEL lvl);
static void vlog(SIGLOG_LEVEL level, char *fmt, va_list vargs);

// Variables to synchronize level signal handling
static pthread_mutex_t level_mutex;
static pthread_cond_t level_cond;
static volatile int level_signal_val;

static void level_handler(int signo, siginfo_t *info, void *other) {
    pthread_mutex_lock(&level_mutex);
    level_signal_val = info->si_value.sival_int;
    pthread_cond_signal(&level_cond);
    pthread_mutex_unlock(&level_mutex);}

static void* level_thread(void* arg)
{
    // Set thread mask
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, level_signal_no);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);

    // Wait for level change signal and handle
    while(1)
    {
        pthread_mutex_lock(&level_mutex);

        while(level_signal_val == -1)
            pthread_cond_wait(&level_cond, &level_mutex);

        if(level_signal_val < SIGLOG_DISABLED || level_signal_val > SIGLOG_MAX) continue;
        current_level = level_signal_val;
        level_signal_val = -1;

        pthread_mutex_unlock(&level_mutex);
    }
}

// Variables to synchronize dump signal handling
static sem_t sem_dump;

static void dump_handler() {
    sem_post(&sem_dump);
}

static void* dump_thread(void* arg)
{
    // Set thread mask
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, dump_signal_no);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);

    // Wait for dump signal and handle
    while(1)
    {
        sem_wait(&sem_dump);
        FILE *dump_file = create_dump_file();
        for(int i=0; i<dump_functions_size; i++)
            dump_functions[i](dump_file);
        fclose(dump_file);
    }
}

static FILE * create_dump_file() {
    char time_buffer[50];

    // Get date
    time_t time_raw = time(NULL);
    struct tm *time_info = localtime(&time_raw);
    strftime(time_buffer, 50, "%m.%d.%y-%H:%M:%S", time_info);

    // Construct filename
    char filename[50];
    sprintf(filename, "DUMP-%s", time_buffer);

    // Construct path
    char path[100];
    if(logging_dir_path != NULL) sprintf(path, "%s/%s", logging_dir_path, filename);
    else sprintf(path, filename);

    // Open file
    FILE *file = fopen(path, "w");

    // Write header line to dump file
    fprintf(file, "%s\n", filename);
    fprintf(file, "---------------------\n");

    return file;
}

int siglog_init(int level_signal, int dump_signal, SIGLOG_LEVEL level, char* path) {

    // Local variables
    sigset_t set;
    struct sigaction act;
    int err;

    if(initialized == 1)
        return 1;

    // Determine signals numbers
    if(level_signal == -1) level_signal = DEFAULT_LEVEL_SIGNAL;
    if(dump_signal == -1) dump_signal = DEFAULT_DUMP_SIGNAL;

    // Init some global vars
    dump_functions_capacity = 8;
    current_level = level;
    level_signal_val = -1;
    logging_dir_path = path;
    level_signal_no = level_signal;
    dump_signal_no = dump_signal;

    // Open file
    char filename[100] = {};
    if(path != NULL) sprintf(filename, "%s/%s", path, "logs");
    else sprintf(filename, "logs");
    log_file = fopen(filename, "a+");
    if(log_file == NULL) return 1;

    // Init semaphor
    err = sem_init(&sem_dump, 0, 0);
    if(err != 0) return 1;

    // Init mutexes
    err = pthread_mutex_init(&level_mutex, NULL);
    if(err != 0) {
        sem_destroy(&sem_dump);
        fclose(log_file);
    }

    err = pthread_mutex_init(&log_mutex, NULL);
    if(err != 0) {
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&level_mutex);
    }

    err = pthread_mutex_init(&dump_mutex, NULL);
    if(err != 0) {
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&level_mutex);
        pthread_mutex_destroy(&log_mutex);
    }

    // Init cond
    err = pthread_cond_init(&level_cond, NULL);
    if(err != 0) {
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&level_mutex);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&dump_mutex);
    }

    // Ignore all RT signals
    act.sa_handler = SIG_IGN;
    for(int i=SIGRTMIN; i<=SIGRTMAX; i++)
        sigaction(i, &act, NULL);

    // Set first RT signal handler
    sigfillset(&set);
    act.sa_sigaction = level_handler;
    act.sa_mask = set;
    act.sa_flags = SA_SIGINFO;
    err = sigaction(level_signal, &act, NULL);
    if(err != 0) {
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&level_mutex);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&dump_mutex);
        pthread_cond_destroy(&level_cond);
    }

    // Set second RT signal handler
    sigfillset(&set);
    act.sa_sigaction = dump_handler;
    act.sa_mask = set;
    err = sigaction(dump_signal, &act, NULL);
    if(err != 0) {
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&level_mutex);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&dump_mutex);
        pthread_cond_destroy(&level_cond);
    }

    // Create first thread
    err = pthread_create(&level_tid, NULL, level_thread, NULL);
    if(err != 0) {
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&level_mutex);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&dump_mutex);
        pthread_cond_destroy(&level_cond);
        return 1;
    }

    // Create second thread
    err = pthread_create(&dump_tid, NULL, dump_thread, NULL);
    if(err != 0) {
        pthread_cancel(level_tid);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&level_mutex);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&dump_mutex);
        pthread_cond_destroy(&level_cond);
        return 1;
    }

    // Detach first thread
    err = pthread_detach(level_tid);
    if(err != 0) {
        pthread_cancel(level_tid);
        pthread_cancel(dump_tid);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&level_mutex);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&dump_mutex);
        pthread_cond_destroy(&level_cond);
        return 1;
    }

    // Detach second thread
    err = pthread_detach(dump_tid);
    if(err != 0) {
        pthread_cancel(level_tid);
        pthread_cancel(dump_tid);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&level_mutex);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&dump_mutex);
        pthread_cond_destroy(&level_cond);
        return 1;
    }

    // Alloc array
    dump_functions = (DUMP_FUNCTION *)calloc(sizeof(DUMP_FUNCTION), dump_functions_capacity);
    if(dump_functions == NULL) {
        pthread_cancel(level_tid);
        pthread_cancel(dump_tid);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&level_mutex);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&dump_mutex);
        pthread_cond_destroy(&level_cond);
        return 1;
    }

    initialized = 1;
    return 0;
}

void siglog_free() {
    if(!initialized) return;

    pthread_cancel(level_tid);
    pthread_cancel(dump_tid);
    pthread_join(level_tid, NULL);
    pthread_join(dump_tid, NULL);
    sem_destroy(&sem_dump);
    pthread_mutex_destroy(&level_mutex);
    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&dump_mutex);
    pthread_cond_destroy(&level_cond);
    free(dump_functions);
    fclose(log_file);
}

static char *str_level(SIGLOG_LEVEL lvl) {
    switch(lvl) {
        case SIGLOG_MAX: return "MAX";
        case SIGLOG_STANDARD: return "STANDARD";
        case SIGLOG_MIN: return "MIN";
        case SIGLOG_DISABLED: return "DISABLED";
    }
}

static void vlog(SIGLOG_LEVEL level, char *fmt, va_list vargs) {
    if(!initialized || level == SIGLOG_DISABLED) return;

    if(level<=current_level) {

        // Get date
        time_t time_raw = time(NULL);
        struct tm *time_info = localtime(&time_raw);
        char time_buffer[50];
        strftime(time_buffer, 50, DATE_FORMAT, time_info);

        // Write to file
        pthread_mutex_lock(&log_mutex);

        fprintf(log_file, "%s | %-8s | ", time_buffer, str_level(level));
        vfprintf(log_file, fmt, vargs);
        fprintf(log_file, "\n");
        fflush(log_file);

        pthread_mutex_unlock(&log_mutex);
    }

}

void siglog_log(SIGLOG_LEVEL level, char *fmt, ...) {
    va_list valist;
    va_start(valist, fmt);
    vlog(level, fmt, valist);
    va_end(valist);
}

void siglog_max(char *fmt, ...) {
    va_list valist;
    va_start(valist, fmt);
    vlog(SIGLOG_MAX, fmt, valist);
    va_end(valist);
}

void siglog_standard(char *fmt, ...) {
    va_list valist;
    va_start(valist, fmt);
    vlog(SIGLOG_STANDARD, fmt, valist);
    va_end(valist);
}

void siglog_min(char *fmt, ...) {
    va_list valist;
    va_start(valist, fmt);
    vlog(SIGLOG_MIN, fmt, valist);
    va_end(valist);
}

int siglog_register_dump_function(DUMP_FUNCTION fun) {
    if(!initialized) return 1;

    pthread_mutex_lock(&dump_mutex);

    if(dump_functions_size >= dump_functions_capacity) {
        DUMP_FUNCTION *new_dump_functions = (DUMP_FUNCTION *)realloc(dump_functions, sizeof(DUMP_FUNCTION) * dump_functions_capacity * 2);
        if(new_dump_functions == NULL) return 1;
        dump_functions = new_dump_functions;
        dump_functions_capacity = dump_functions_capacity * 2;
    }
    dump_functions[dump_functions_size++] = fun;

    pthread_mutex_unlock(&dump_mutex);

    return 0;
}
