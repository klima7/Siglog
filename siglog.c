#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include "siglog.h"

#define DATE_FORMAT "%m/%d/%y %H:%M:%S"

// File scoped global variables
static int initialized = 0;
static pthread_t tid_level, tid_dump;
static sem_t sem_dump;
static pthread_mutex_t log_mutex, level_mutex;
static pthread_cond_t level_cond;
static volatile int level_signal_val;
static SIGLOG_LEVEL current_level;
static FILE *log_file;
static DUMP_FUNCTION dump_function;


// Prototypes of file scoped functions
static void* level_thread(void* arg);
static void level_handler(int signo, siginfo_t *info, void *other);
static void* dump_thread(void* arg);
static void dump_handler();
static FILE * create_dump_file();
static char *str_level(SIGLOG_LEVEL lvl);
static void vlog(SIGLOG_LEVEL level, char *fmt, va_list vargs);


static void* level_thread(void* arg)
{
    while(1)
    {
        pthread_mutex_lock(&level_mutex);

        while(level_signal_val == -1)
            pthread_cond_wait(&level_cond, &level_mutex);

        if(level_signal_val < SIGLOG_DISABLED || level_signal_val > SIGLOG_MAX) continue;
        pthread_mutex_lock(&log_mutex);
        current_level = level_signal_val;
        printf("Logging level changed to %d\n", current_level);
        level_signal_val = -1;
        pthread_mutex_unlock(&log_mutex);

        pthread_mutex_unlock(&level_mutex);
    }
}

static void level_handler(int signo, siginfo_t *info, void *other) {
    printf("1\n");
    pthread_mutex_lock(&level_mutex);
    printf("2\n");
    level_signal_val = info->si_value.sival_int;
    pthread_cond_signal(&level_cond);
    pthread_mutex_unlock(&level_mutex);
}

static void* dump_thread(void* arg)
{
    while(1)
    {
        sem_wait(&sem_dump);
        if(dump_function != NULL) {
            FILE *dump_file = create_dump_file();
            dump_function(dump_file);
            fclose(dump_file);
        }
    }
}

static void dump_handler() {
    sem_post(&sem_dump);
}

static FILE * create_dump_file() {
    char time_buffer[50];

    // Get date
    time_t time_raw = time(NULL);
    struct tm *time_info = localtime(&time_raw);
    strftime(time_buffer, 50, "%m.%d.%y-%H:%M:%S", time_info);

    // Construct filename and open file
    char filename[100];
    sprintf(filename, "DUMP-%s", time_buffer);
    FILE *file = fopen(filename, "w");

    // Write header line to dump file
    fprintf(file, "%s\n", filename);
    fprintf(file, "---------------------\n");

    return file;
}

int siglog_init(int level_signal_nr, int dump_signal_nr, SIGLOG_LEVEL start_level, char* path) {

    // Variables
    sigset_t set, set_backup;
    struct sigaction act;
    int err;

    if(initialized == 1) return 1;

    current_level = start_level;
    level_signal_val = -1;

    // Determine signals numbers
    if(level_signal_nr == -1) level_signal_nr = DEFAULT_LEVEL_SIGNAL;
    if(dump_signal_nr == -1) dump_signal_nr = DEFAULT_DUMP_SIGNAL;

    // Open file
    log_file = fopen(path, "a+");
    if(log_file == NULL) return 1;

    // Set thread mask
    sigemptyset(&set);
    sigaddset(&set, level_signal_nr);
    sigaddset(&set, dump_signal_nr);
    err = pthread_sigmask(SIG_UNBLOCK, &set, &set_backup);
    if(err != 0)  {
        fclose(log_file);
        return 1;
    }

    // Init semaphor
    err = sem_init(&sem_dump, 0, 0);
    if(err != 0) return 1;

    // Init mutexes
    err = pthread_mutex_init(&log_mutex, NULL);
    if(err != 0) {
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        sem_destroy(&sem_dump);
        fclose(log_file);
    }
    err = pthread_mutex_init(&level_mutex, NULL);
    if(err != 0) {
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        pthread_mutex_destroy(&log_mutex);
        sem_destroy(&sem_dump);
        fclose(log_file);
    }

    // Init cond
    err = pthread_cond_init(&level_cond, NULL);
    if(err != 0) {
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&level_mutex);
    }

    // Set first async handler
    sigfillset(&set);
    act.sa_sigaction = level_handler;
    act.sa_mask = set;
    act.sa_flags = SA_SIGINFO;
    err = sigaction(level_signal_nr, &act, NULL);
    if(err != 0) {
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&level_mutex);
        pthread_cond_destroy(&level_cond);
    }

    // Set second async handler
    sigfillset(&set);
    act.sa_sigaction = dump_handler;
    act.sa_mask = set;
    act.sa_flags = SA_SIGINFO;
    err = sigaction(dump_signal_nr, &act, NULL);
    if(err != 0) {
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&level_mutex);
        pthread_cond_destroy(&level_cond);
    }

    // Create first thread
    err = pthread_create(&tid_level, NULL, level_thread, NULL);
    if(err != 0) {
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&level_mutex);
        pthread_cond_destroy(&level_cond);
        return 1;
    }

    // Create second thread
    err = pthread_create(&tid_dump, NULL, dump_thread, NULL);
    if(err != 0) {
        pthread_cancel(tid_level);
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&level_mutex);
        pthread_cond_destroy(&level_cond);
        return 1;
    }

    // Detach first thread
    err = pthread_detach(tid_level);
    if(err != 0) {
        pthread_cancel(tid_level);
        pthread_cancel(tid_dump);
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&level_mutex);
        pthread_cond_destroy(&level_cond);
        return 1;
    }

    // Detach second thread
    err = pthread_detach(tid_dump);
    if(err != 0) {
        pthread_cancel(tid_level);
        pthread_cancel(tid_dump);
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&level_mutex);
        pthread_cond_destroy(&level_cond);
        return 1;
    }

    initialized = 1;
    return 0;
}

void siglog_free() {
    if(!initialized) return;

    pthread_cancel(tid_level);
    pthread_cancel(tid_dump);
    sem_destroy(&sem_dump);
    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&level_mutex);
    pthread_cond_destroy(&level_cond);
    fclose(log_file);
}

static char *str_level(SIGLOG_LEVEL lvl) {
    switch(lvl) {
        case SIGLOG_MAX: return "DEBUG";
        case SIGLOG_STANDARD: return "INFO";
        case SIGLOG_MIN: return "WARNING";
        case SIGLOG_DISABLED: return "DISABLED";
    }
}

static void vlog(SIGLOG_LEVEL level, char *fmt, va_list vargs) {
    pthread_mutex_lock(&log_mutex);
    if(level<=current_level) {

        // Get date
        time_t time_raw = time(NULL);
        struct tm *time_info = localtime(&time_raw);
        char time_buffer[50];
        strftime(time_buffer, 50, DATE_FORMAT, time_info);

        // Write to file
        fprintf(log_file, "%s | %s | ", time_buffer, str_level(level));
        vfprintf(log_file, fmt, vargs);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    pthread_mutex_unlock(&log_mutex);
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

void siglog_register_dump_function(DUMP_FUNCTION fun) {
    dump_function = fun;
}
