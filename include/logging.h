#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <errno.h>
#include <string.h>
#include <stdio.h>

#define log_info(file, fmt, args...) print_log(file, __LINE__, __func__, LOG_LEVEL_INFO, fmt, ## args)
#define log_error(file) print_log(file, __LINE__, __func__, LOG_LEVEL_ERROR, "%s\n", strerror(errno))
#define log(file, level, fmt, args...) print_log(file, __LINE__, __func__, level, fmt, ## args)
#define LOG_FILE_HOME "./log/"

typedef enum{
    LOG_LEVEL_INFO,
    LOG_LEVEL_ERROR
} LogLevel;

extern const char* logtostr[];

int print_log(FILE* file, const int line, const char *func, const LogLevel level, const char *format, ... );
char* get_file_path(const char* file_home, ...);
size_t compute_length(const char* first, va_list args);
#endif