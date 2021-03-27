#ifndef UNTITLED2_SIGLOG_H
#define UNTITLED2_SIGLOG_H

// Definitions
#define DEFAULT_LEVEL_SIGNAL SIGRTMIN
#define DEFAULT_DUMP_SIGNAL SIGRTMIN+1

// Types
enum siglog_level_t { SIGLOG_DEBUG, SIGLOG_INFO, SIGLOG_WARNING, SIGLOG_DISABLED };
enum siglog_type_t { SIGLOG_CHAR, SIGLOG_SHORT, SIGLOG_INT, SIGLOG_LONG, SIGLOG_FLOAT, SIGLOG_DOUBLE, SIGLOG_UCHAR, SIGLOG_USHORT, SIGLOG_UINT, SIGLOG_ULONG};
struct siglog_element_t {
    char *name;
    volatile void *data;
    enum siglog_type_t type;
};

// Typedefs
typedef enum siglog_level_t SIGLOG_LEVEL;
typedef enum siglog_type_t SIGLOG_TYPE;
typedef struct siglog_element_t SIGLOG_ELEMENT;

// Functions
int siglog_init(int level_signal_nr, int dump_signal_nr, SIGLOG_LEVEL start_level, char* path);
void siglog_free();

void siglog_log(SIGLOG_LEVEL level, char *fmt, ...);
void siglog_debug(char *fmt, ...);
void siglog_info(char *fmt, ...);
void siglog_warning(char *fmt, ...);

int siglog_dump(char *name, SIGLOG_TYPE type, void *data);
void siglog_dump_char(char *name, char *val);
void siglog_dump_short(char *name, short *val);
void siglog_dump_int(char *name, int *val);
void siglog_dump_long(char *name, long *val);
void siglog_dump_float(char *name, float *val);
void siglog_dump_double(char *name, double *val);
void siglog_dump_uchar(char *name, unsigned char *val);
void siglog_dump_ushort(char *name, unsigned short *val);
void siglog_dump_uint(char *name, unsigned int *val);
void siglog_dump_ulong(char *name, unsigned long *val);

#endif
