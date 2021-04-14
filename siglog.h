#ifndef UNTITLED2_SIGLOG_H
#define UNTITLED2_SIGLOG_H

// Types
enum siglog_level_t { SIGLOG_DISABLED, SIGLOG_MAX, SIGLOG_STANDARD, SIGLOG_MIN };
typedef enum siglog_level_t SIGLOG_LEVEL;
typedef void (*DUMP_FUNCTION)(FILE *file);

// Lifecycle functions
int siglog_init(int level_signal, int dump_signal, SIGLOG_LEVEL level, char* dir_path);
void siglog_free();

// Logging functions
void siglog_log(SIGLOG_LEVEL level, char *fmt, ...);
void siglog_min(char *fmt, ...);
void siglog_standard(char *fmt, ...);
void siglog_max(char *fmt, ...);

// Dumping functions
int siglog_register_dump_function(DUMP_FUNCTION fun);

#endif
