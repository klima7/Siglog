#ifndef UNTITLED2_SIGLOG_H
#define UNTITLED2_SIGLOG_H

// Definitions
#define DEFAULT_LEVEL_SIGNAL SIGRTMIN
#define DEFAULT_DUMP_SIGNAL SIGRTMIN+1

// Types
enum siglog_level_t { SIGLOG_DISABLED, SIGLOG_MIN, SIGLOG_STANDARD, SIGLOG_MAX };
typedef enum siglog_level_t SIGLOG_LEVEL;
typedef void (*DUMP_FUNCTION)(FILE *file);

// Lifecycle functions
int siglog_init(int level_signal_nr, int dump_signal_nr, SIGLOG_LEVEL start_level, char* path);
void siglog_free();

// Logging functions
void siglog_log(SIGLOG_LEVEL level, char *fmt, ...);
void siglog_min(char *fmt, ...);
void siglog_standard(char *fmt, ...);
void siglog_max(char *fmt, ...);

// Dumping functions
void siglog_register_dump_function(DUMP_FUNCTION fun);

#endif
