#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include "utils.h"

static const char *log_level_strings[] = {
  "ERROR", "INFO", "DEBUG"
};

static infac_log_level current_log_level = INFAC_LOG_INFO;

void infac_set_log_level(infac_log_level level) {
  current_log_level = level;
}

void infac_log(infac_log_level level, const char *fmt, ...) {
  if (level > current_log_level) return;

  time_t timer = time(NULL);
  struct tm *time_info = localtime(&timer);
  char time_buf[16];
  strftime(time_buf, sizeof(time_buf), "%b %e %H:%M:%S", time_info);

  va_list arguments;
  va_start(arguments, fmt);
  char message_buf[100];
  vsnprintf(message_buf, sizeof(message_buf), fmt, arguments);
  va_end(arguments);

  FILE *output = (level == INFAC_LOG_ERROR) ? stderr : stdout;

  fprintf(output, "%s %s: %s\n", time_buf, log_level_strings[level], message_buf);
}
