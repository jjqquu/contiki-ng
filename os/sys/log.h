/*
 * Copyright (c) 2017, Inria.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Header file for the logging system
 * \author
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 */

/** \addtogroup sys
 * @{ */

/**
 * \defgroup log Per-module, per-level logging
 * @{
 *
 * The log module performs per-module, per-level logging
 *
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include "sys/log-conf.h"

/* The different log levels available */
#define LOG_LEVEL_NONE         0 /* No log */
#define LOG_LEVEL_ERR          1 /* Errors */
#define LOG_LEVEL_WARN         2 /* Warnings */
#define LOG_LEVEL_INFO         3 /* Basic info */
#define LOG_LEVEL_DBG          4 /* Detailled debug */

/* Log coloring */
#define TC_RESET   "\033[0m"
#define TC_BLACK   "\033[0;30m"
#define TC_RED     "\033[0;31m"
#define TC_GREEN   "\033[0;32m"
#define TC_YELLOW  "\033[0;33m"
#define TC_BLUE    "\033[0;34m"
#define TC_MAGENTA "\033[0;35m"
#define TC_CYAN    "\033[0;36m"
#define TC_WHITE   "\033[0;37m"

#define LOG_COLOR_RESET TC_RESET

#ifndef LOG_COLOR_ERR
#define LOG_COLOR_ERR   TC_RED
#endif

#ifndef LOG_COLOR_WARN
#define LOG_COLOR_WARN  TC_YELLOW
#endif

#ifndef LOG_COLOR_INFO
#define LOG_COLOR_INFO  TC_BLUE
#endif

#ifndef LOG_COLOR_DBG
#define LOG_COLOR_DBG   TC_WHITE
#endif

#ifndef LOG_COLOR_PRI
#define LOG_COLOR_PRI   TC_CYAN
#endif

/* Per-module log level */

struct log_module {
  const char *name;
  int *curr_log_level;
  int max_log_level;
};

extern int curr_log_level_main;

extern struct log_module all_modules[];

#define LOG_LEVEL_MAIN                        MIN((LOG_CONF_LEVEL_MAIN), curr_log_level_main)

/* Main log function */

#define LOG(newline, level, levelstr, levelcolor, ...) do {  \
                            if(level <= (LOG_LEVEL)) { \
                              if(newline) { \
                                if(LOG_WITH_COLOR) { \
                                  LOG_OUTPUT(levelcolor); \
                                } \
                                if(LOG_WITH_MODULE_PREFIX) { \
                                  LOG_OUTPUT_PREFIX(level, levelstr, LOG_MODULE); \
                                } \
                                if(LOG_WITH_LOC) { \
                                  LOG_OUTPUT("[%s: %d] ", __FILE__, __LINE__); \
                                } \
                                if(LOG_WITH_COLOR) { \
                                  LOG_OUTPUT(LOG_COLOR_RESET); \
                                } \
                              } \
                              LOG_OUTPUT(__VA_ARGS__); \
                            } \
                          } while (0)

#define LOG_BYTES(level, data, length) do {  \
                           if(level <= (LOG_LEVEL)) { \
                             log_bytes(data, length); \
                           } \
                         } while (0)

/* More compact versions of LOG macros */
#define LOG_PRINT(...)         LOG(1, 0, "PRI", LOG_COLOR_PRI, __VA_ARGS__)
#define LOG_ERR(...)           LOG(1, LOG_LEVEL_ERR, "ERR", LOG_COLOR_ERR, __VA_ARGS__)
#define LOG_WARN(...)          LOG(1, LOG_LEVEL_WARN, "WARN", LOG_COLOR_WARN, __VA_ARGS__)
#define LOG_INFO(...)          LOG(1, LOG_LEVEL_INFO, "INFO", LOG_COLOR_INFO, __VA_ARGS__)
#define LOG_DBG(...)           LOG(1, LOG_LEVEL_DBG, "DBG", LOG_COLOR_DBG, __VA_ARGS__)

#define LOG_PRINT_(...)         LOG(0, 0, "PRI", LOG_COLOR_PRI, __VA_ARGS__)
#define LOG_ERR_(...)           LOG(0, LOG_LEVEL_ERR, "ERR", LOG_COLOR_ERR, __VA_ARGS__)
#define LOG_WARN_(...)          LOG(0, LOG_LEVEL_WARN, "WARN", LOG_COLOR_WARN, __VA_ARGS__)
#define LOG_INFO_(...)          LOG(0, LOG_LEVEL_INFO, "INFO", LOG_COLOR_INFO, __VA_ARGS__)
#define LOG_DBG_(...)           LOG(0, LOG_LEVEL_DBG, "DBG", LOG_COLOR_DBG, __VA_ARGS__)

#define LOG_PRINT_BYTES(data, length)   LOG_BYTES(0, data, length)
#define LOG_ERR_BYTES(data, length)     LOG_BYTES(LOG_LEVEL_ERR, data, length)
#define LOG_WARN_BYTES(data, length)    LOG_BYTES(LOG_LEVEL_WARN, data, length)
#define LOG_INFO_BYTES(data, length)    LOG_BYTES(LOG_LEVEL_INFO, data, length)
#define LOG_DBG_BYTES(data, length)     LOG_BYTES(LOG_LEVEL_DBG, data, length)

/* For checking log level.
   As this builds on curr_log_level variables, this should not be used
   in pre-processor macros. Use in a C 'if' statement instead, e.g.:
   if(LOG_INFO_ENABLED) { ... }
   Note that most compilers will still be able to strip the code out
   for low enough log levels configurations. */
#define LOG_ERR_ENABLED        ((LOG_LEVEL) >= LOG_LEVEL_ERR)
#define LOG_WARN_ENABLED       ((LOG_LEVEL) >= LOG_LEVEL_WARN)
#define LOG_INFO_ENABLED       ((LOG_LEVEL) >= LOG_LEVEL_INFO)
#define LOG_DBG_ENABLED        ((LOG_LEVEL) >= LOG_LEVEL_DBG)

/**
 * Logs a byte array as hex characters
 * \param data The byte array
 * \param length The length of the byte array
*/
void log_bytes(const void *data, size_t length);

/**
 * Sets a log level at run-time. Logs are included in the firmware via
 * the compile-time flags in log-conf.h, but this allows to force lower log
 * levels, system-wide.
 * \param module The target module string descriptor
 * \param level The log level
*/
void log_set_level(const char *module, int level);

/**
 * Returns the current log level.
 * \param module The target module string descriptor
 * \return The current log level
*/
int log_get_level(const char *module);

/**
 * Returns a textual description of a log level
 * \param level log level
 * \return The textual description
*/
const char *log_level_to_str(int level);

#endif /* __LOG_H__ */

/** @} */
/** @} */
