typedef enum infac_log_level {
  INFAC_LOG_ERROR,
  INFAC_LOG_INFO,
  INFAC_LOG_DEBUG
} infac_log_level;

void infac_set_log_level(infac_log_level level);
void infac_log(infac_log_level level, const char *fmt, ...);

#define infac_log_error(...) infac_log(INFAC_LOG_ERROR, __VA_ARGS__)
#define infac_log_info(...) infac_log(INFAC_LOG_INFO, __VA_ARGS__)
#define infac_log_debug(...) infac_log(INFAC_LOG_DEBUG, __VA_ARGS__)
