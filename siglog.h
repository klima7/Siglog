#ifndef UNTITLED2_SIGLOG_H
#define UNTITLED2_SIGLOG_H

// Definitions
#define DEFAULT_LEVEL_SIGNAL SIGRTMIN
#define DEFAULT_DUMP_SIGNAL SIGRTMIN+1

// Types
enum siglog_level_t { SIGLOG_DISABLED, SIGLOG_WARNING, SIGLOG_INFO, SIGLOG_DEBUG };
enum siglog_type_t { SIGLOG_CHAR, SIGLOG_SHORT, SIGLOG_INT, SIGLOG_LONG, SIGLOG_FLOAT, SIGLOG_DOUBLE, SIGLOG_UCHAR, SIGLOG_USHORT, SIGLOG_UINT, SIGLOG_ULONG};

// Typedefs
typedef enum siglog_level_t SIGLOG_LEVEL;
typedef enum siglog_type_t SIGLOG_TYPE;

// Lifecycle functions
int siglog_init(int level_signal_nr, int dump_signal_nr, SIGLOG_LEVEL start_level, char* path);
void siglog_free();

// Logging functions
void siglog_log(SIGLOG_LEVEL level, char *fmt, ...);
void siglog_debug(char *fmt, ...);
void siglog_info(char *fmt, ...);
void siglog_warning(char *fmt, ...);

// Dumping functions
int siglog_dump_var(char *name, SIGLOG_TYPE type, void *data);
int siglog_dump_char(char *name, char *val);
int siglog_dump_short(char *name, short *val);
int siglog_dump_int(char *name, int *val);
int siglog_dump_long(char *name, long *val);
int siglog_dump_float(char *name, float *val);
int siglog_dump_double(char *name, double *val);
int siglog_dump_uchar(char *name, unsigned char *val);
int siglog_dump_ushort(char *name, unsigned short *val);
int siglog_dump_uint(char *name, unsigned int *val);
int siglog_dump_ulong(char *name, unsigned long *val);

#endif
