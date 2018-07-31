#ifndef _LOG_H
#define _LOG_H

enum LOG_LEVEL {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_MAX
};

void write_log(int log_level, const char *code_path,
               int line, const char *fmt, ...);

#define DEBUGLOG(fmt, ...) write_log(LOG_LEVEL_DEBUG, __FILE__,\
                __LINE__, fmt, ##__VA_ARGS__);


#define ERRORLOG(fmt, ...) write_log(LOG_LEVEL_ERROR, __FILE__,\
                __LINE__, fmt, ##__VA_ARGS__);

#endif // _LOG_H