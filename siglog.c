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
static pthread_mutex_t log_mutex, level_mutex, dump_mutex;
static pthread_cond_t level_cond;
static volatile int level_signal_val;
static SIGLOG_LEVEL current_level;
static FILE *log_file;

static struct siglog_element_t *dump_elements;
static int dump_elements_capacity;
static int dump_elements_size;


// File scoped types
static struct siglog_element_t {
    char *name;
    volatile void *data;
    enum siglog_type_t type;
};


// Prototypes of file scoped functions
static void* level_thread(void* arg);
static void level_handler(int signo, siginfo_t *info, void *other);
static void* dump_thread(void* arg);
static void dump_handler();
static FILE * create_dump_file();
static void dump_elements_to_file(FILE *dump_file);
static void dump_element_to_file(FILE *dump_file, struct siglog_element_t *element);
static char *str_level(SIGLOG_LEVEL lvl);
static void vlog(SIGLOG_LEVEL level, char *fmt, va_list vargs);


static void* level_thread(void* arg)
{
    while(1)
    {
        pthread_mutex_lock(&level_mutex);

        while(level_signal_val == -1)
            pthread_cond_wait(&level_cond, &level_mutex);

        if(level_signal_val < SIGLOG_DISABLED || level_signal_val > SIGLOG_DEBUG) continue;
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
        FILE *dump_file = create_dump_file();
        dump_elements_to_file(dump_file);
        fclose(dump_file);
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

static void dump_elements_to_file(FILE *dump_file) {
    pthread_mutex_lock(&dump_mutex);
    for(int i=0; i < dump_elements_size; i++) {
        dump_element_to_file(dump_file, dump_elements + i);
    }
    pthread_mutex_unlock(&dump_mutex);
}

static void dump_element_to_file(FILE *dump_file, struct siglog_element_t *element) {
    fprintf(dump_file, "%s: ", element->name);

    if(element->type == SIGLOG_CHAR) fprintf(dump_file, "%c", *((volatile char *)element->data));
    else if(element->type == SIGLOG_SHORT) fprintf(dump_file, "%d", *((volatile short *)element->data));
    else if(element->type == SIGLOG_INT) fprintf(dump_file, "%d", *((volatile int *)element->data));
    else if(element->type == SIGLOG_LONG) fprintf(dump_file, "%ld", *((volatile long *)element->data));
    else if(element->type == SIGLOG_FLOAT) fprintf(dump_file, "%f", *((volatile float *)element->data));
    else if(element->type == SIGLOG_DOUBLE) fprintf(dump_file, "%lf", *((volatile double *)element->data));
    else if(element->type == SIGLOG_UCHAR) fprintf(dump_file, "%u", *((volatile unsigned char *)element->data));
    else if(element->type == SIGLOG_USHORT) fprintf(dump_file, "%hu", *((volatile unsigned short *)element->data));
    else if(element->type == SIGLOG_UINT) fprintf(dump_file, "%u", *((volatile unsigned int *)element->data));
    else if(element->type == SIGLOG_ULONG) fprintf(dump_file, "%lu", *((volatile unsigned long *)element->data));

    fprintf(dump_file, "\n");
    fflush(dump_file);
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
    err = pthread_mutex_init(&dump_mutex, NULL);
    if(err != 0) {
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&level_mutex);
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
        pthread_mutex_destroy(&dump_mutex);
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
        pthread_mutex_destroy(&dump_mutex);
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
        pthread_mutex_destroy(&dump_mutex);
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
        pthread_mutex_destroy(&dump_mutex);
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
        pthread_mutex_destroy(&dump_mutex);
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
        pthread_mutex_destroy(&dump_mutex);
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
        pthread_mutex_destroy(&dump_mutex);
        pthread_cond_destroy(&level_cond);
        return 1;
    }

    // Alloc array
    dump_elements_capacity = 8;
    dump_elements = (struct siglog_element_t *)calloc(sizeof(struct siglog_element_t), dump_elements_capacity);
    if(dump_elements == NULL) {
        pthread_cancel(tid_level);
        pthread_cancel(tid_dump);
        pthread_sigmask(SIG_SETMASK, &set_backup, NULL);
        sem_destroy(&sem_dump);
        fclose(log_file);
        pthread_mutex_destroy(&log_mutex);
        pthread_mutex_destroy(&level_mutex);
        pthread_mutex_destroy(&dump_mutex);
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
    free(dump_elements);
    fclose(log_file);
}

static char *str_level(SIGLOG_LEVEL lvl) {
    switch(lvl) {
        case SIGLOG_DEBUG: return "DEBUG";
        case SIGLOG_INFO: return "INFO";
        case SIGLOG_WARNING: return "WARNING";
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

void siglog_debug(char *fmt, ...) {
    va_list valist;
    va_start(valist, fmt);
    vlog(SIGLOG_DEBUG, fmt, valist);
    va_end(valist);
}

void siglog_info(char *fmt, ...) {
    va_list valist;
    va_start(valist, fmt);
    vlog(SIGLOG_INFO, fmt, valist);
    va_end(valist);
}

void siglog_warning(char *fmt, ...) {
    va_list valist;
    va_start(valist, fmt);
    vlog(SIGLOG_WARNING, fmt, valist);
    va_end(valist);
}

int siglog_dump_var(char *name, SIGLOG_TYPE type, void *data) {
    struct siglog_element_t element;
    element.name = name;
    element.type = type;
    element.data = data;

    pthread_mutex_lock(&dump_mutex);

    if(dump_elements_size >= dump_elements_capacity) {
        struct siglog_element_t *new_dump_elements = (struct siglog_element_t *)realloc(dump_elements, sizeof(struct siglog_element_t) * dump_elements_capacity * 2);
        if(new_dump_elements == NULL) return 1;
        dump_elements = new_dump_elements;
        dump_elements_capacity = dump_elements_capacity * 2;
    }

    dump_elements[dump_elements_size++] = element;

    pthread_mutex_unlock(&dump_mutex);
}

int siglog_dump_char(char *name, char *val) { return siglog_dump_var(name, SIGLOG_CHAR, val); };
int siglog_dump_short(char *name, short *val) { return siglog_dump_var(name, SIGLOG_SHORT, val); };
int siglog_dump_int(char *name, int *val) { return siglog_dump_var(name, SIGLOG_INT, val); };
int siglog_dump_long(char *name, long *val) { return siglog_dump_var(name, SIGLOG_LONG, val); };
int siglog_dump_float(char *name, float *val) { return siglog_dump_var(name, SIGLOG_FLOAT, val); };
int siglog_dump_double(char *name, double *val) { return siglog_dump_var(name, SIGLOG_DOUBLE, val); };
int siglog_dump_uchar(char *name, unsigned char *val) { return siglog_dump_var(name, SIGLOG_UCHAR, val); };
int siglog_dump_ushort(char *name, unsigned short *val) { return siglog_dump_var(name, SIGLOG_USHORT, val); };
int siglog_dump_uint(char *name, unsigned int *val) { return siglog_dump_var(name, SIGLOG_UINT, val); };
int siglog_dump_ulong(char *name, unsigned long *val) { return siglog_dump_var(name, SIGLOG_ULONG, val); };