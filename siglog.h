#ifndef UNTITLED2_SIGLOG_H
#define UNTITLED2_SIGLOG_H

// Definitions

#define DEFAULT_LEVEL_SIGNAL SIGRTMIN
#define DEFAULT_DUMP_SIGNAL SIGRTMIN+1

// Types

enum siglog_level_t { SIGLOG_DEBUG, SIGLOG_INFO, SIGLOG_WARNING, SIGLOG_DISABLED };

typedef enum siglog_level_t SIGLOG_LEVEL;

enum siglog_type_t { SIGLOG_CHAR, SIGLOG_SHORT, SIGLOG_INT, SIGLOG_LONG, SIGLOG_FLOAT, SIGLOG_DOUBLE};

typedef enum siglog_type_t SIGLOG_TYPE;

struct siglog_element_t {
    char *name;
    volatile void *data;
    enum siglog_type_t type;
};

typedef struct siglog_element_t SIGLOG_ELEMENT;

typedef char* (*SIGLOG_DUMP_HANDLER)();

// Functions

int siglog_init(int level_signal_nr, int dump_signal_nr, SIGLOG_LEVEL start_level, char* path);

void siglog_free();

void siglog_log(SIGLOG_LEVEL level, char *fmt, ...);

void siglog_debug(char *fmt, ...);

void siglog_info(char *fmt, ...);

void siglog_warning(char *fmt, ...);

int siglog_trace(char *name, SIGLOG_TYPE type, void *data);

void siglog_trace_char(char *name, char *val);
void siglog_trace_short(char *name, short *val);
void siglog_trace_int(char *name, int *val);
void siglog_trace_long(char *name, long *val);
void siglog_trace_float(char *name, float *val);
void siglog_trace_double(char *name, double *val);

#endif
