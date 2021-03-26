#ifndef UNTITLED2_SIGLOG_H
#define UNTITLED2_SIGLOG_H

// Definitions

#define DEFAULT_LEVEL_SIGNAL SIGRTMIN
#define DEFAULT_DUMP_SIGNAL SIGRTMIN+1

// Types

enum siglog_level_t { SIGLOG_DEBUG, SIGLOG_INFO, SIGLOG_WARNING, SIGLOG_DISABLED };

typedef enum siglog_level_t SIGLOG_LEVEL;

typedef char* (*SIGLOG_DUMP_HANDLER)();

// Functions

int siglog_init(int level_signal_nr, int dump_signal_nr, SIGLOG_LEVEL start_level, char* path);

void siglog_register_dump_handler(SIGLOG_DUMP_HANDLER);

void siglog_free();

void siglog_log(SIGLOG_LEVEL level, char *fmt, ...);

void siglog_debug(char *fmt, ...);

void siglog_info(char *fmt, ...);

void siglog_warning(char *fmt, ...);

#endif
