#include "logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

const char* ltos[] = {
    "INFO",
    "ERROR"
};

int print_log(FILE* file, const int line, const char* func, const LogLevel level, const char *format, ...){
    int char_num;
    va_list ap;
    time_t *cur;
    struct tm *now, rt;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    cur = (time_t *)&(tv.tv_sec);
    now = localtime_r(cur, &rt);

    fprintf(file, "[%04d-%02d-%02dT%02d:%02d:%02d.%06d] [%s] [%lu] [%s:%d] ",
             now->tm_year+1900, now->tm_mon+1, now->tm_mday,
             now->tm_hour, now->tm_min, now->tm_sec, tv.tv_usec,
             ltos[level],
             (unsigned long)pthread_self(),
             func, line);

    va_start(ap, format);
    char_num = vfprintf(file, format, ap);
    va_end(ap);
    fflush(file);
    return char_num;
}

char* get_file_path(const char* file_home, ...){
    va_list args;
    va_start(args, file_home);

    va_list args_copy;
    va_copy(args_copy, args);
    size_t length = compute_length(file_home, args_copy);
    va_end(args_copy);

    char* ret = (char*)malloc((length + 1) * sizeof(char));
    if (!ret) {
        return NULL;
    }

    strcpy(ret, file_home);
    const char* str;
    while ((str = va_arg(args, const char*)) != NULL) {
        strcat(ret, str);
    }

    va_end(args);
    return ret;
}

size_t compute_length(const char* first, va_list args) {
    size_t length = strlen(first);
    const char* str;
    while ((str = va_arg(args, const char*)) != NULL) {
        length += strlen(str);
    }
    return length;
}