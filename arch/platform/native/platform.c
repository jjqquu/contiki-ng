/*
 * Copyright (c) 2002, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki OS
 *
 */

/**
 * \ingroup platform
 *
 * \defgroup native_platform Native platform
 *
 * Platform running in the host (Windows or Linux) environment.
 *
 * Used mainly for development and debugging.
 * @{
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

#include "contiki.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Native"
#define LOG_LEVEL LOG_LEVEL_MAIN

/*---------------------------------------------------------------------------*/
/**
 * \name Native Platform Configuration
 *
 * @{
 */

/*
 * Defines the maximum number of file descriptors monitored by the platform
 * main loop.
 */
#ifdef SELECT_CONF_MAX
#define SELECT_MAX SELECT_CONF_MAX
#else
#define SELECT_MAX 8
#endif

/*
 * Defines the timeout (in msec) of the select operation if no monitored file
 * descriptors becomes ready.
 */
#ifdef SELECT_CONF_TIMEOUT
#define SELECT_TIMEOUT SELECT_CONF_TIMEOUT
#else
#define SELECT_TIMEOUT 1000
#endif

/*
 * Adds the STDIN file descriptor to the list of monitored file descriptors.
 */
#ifdef SELECT_CONF_STDIN
#define SELECT_STDIN SELECT_CONF_STDIN
#else
#define SELECT_STDIN 1
#endif
/** @} */
/*---------------------------------------------------------------------------*/

static const struct select_callback *select_callback[SELECT_MAX];
static int select_max = 0;

/*---------------------------------------------------------------------------*/
int
select_set_callback(int fd, const struct select_callback *callback)
{
  int i;
  if(fd >= 0 && fd < SELECT_MAX) {
    /* Check that the callback functions are set */
    if(callback != NULL &&
       (callback->set_fd == NULL || callback->handle_fd == NULL)) {
      callback = NULL;
    }

    select_callback[fd] = callback;

    /* Update fd max */
    if(callback != NULL) {
      if(fd > select_max) {
        select_max = fd;
      }
    } else {
      select_max = 0;
      for(i = SELECT_MAX - 1; i > 0; i--) {
        if(select_callback[i] != NULL) {
          select_max = i;
          break;
        }
      }
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
#if SELECT_STDIN
static int
stdin_set_fd(fd_set *rset, fd_set *wset)
{
  FD_SET(STDIN_FILENO, rset);
  return 1;
}
static int (*input_handler)(unsigned char c);

void
native_uart_set_input(int (*input)(unsigned char c))
{
  input_handler = input;
}
static void
stdin_handle_fd(fd_set *rset, fd_set *wset)
{
  char c;
  if(FD_ISSET(STDIN_FILENO, rset)) {
    if(read(STDIN_FILENO, &c, 1) > 0) {
      input_handler(c);
    }
  }
}
const static struct select_callback stdin_fd = {
  stdin_set_fd, stdin_handle_fd
};
#endif /* SELECT_STDIN */
/*---------------------------------------------------------------------------*/
int contiki_argc = 0;
char **contiki_argv;
/*---------------------------------------------------------------------------*/
void
platform_process_args(int argc, char **argv)
{
  /* crappy way of remembering and accessing argc/v */
  contiki_argc = argc;
  contiki_argv = argv;

  /* native under windows is hardcoded to use the first one or two args */
  /* for wpcap configuration so this needs to be "removed" from         */
  /* contiki_args (used by the native-border-router) */
#ifdef __CYGWIN__
  contiki_argc--;
  contiki_argv++;
#ifdef UIP_FALLBACK_INTERFACE
  contiki_argc--;
  contiki_argv++;
#endif
#endif
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one()
{
  return;
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two()
{
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three()
{
  /* Make standard output unbuffered. */
  setvbuf(stdout, (char *)NULL, _IONBF, 0);
}
/*---------------------------------------------------------------------------*/
void
platform_main_loop()
{
#if SELECT_STDIN
  select_set_callback(STDIN_FILENO, &stdin_fd);
#endif /* SELECT_STDIN */
  while(1) {
    fd_set fdr;
    fd_set fdw;
    int maxfd;
    int i;
    int retval;
    struct timeval tv;

    retval = process_run();

    tv.tv_sec = retval ? 0 : SELECT_TIMEOUT / 1000;
    tv.tv_usec = retval ? 1 : (SELECT_TIMEOUT * 1000) % 1000000;

    FD_ZERO(&fdr);
    FD_ZERO(&fdw);
    maxfd = 0;
    for(i = 0; i <= select_max; i++) {
      if(select_callback[i] != NULL && select_callback[i]->set_fd(&fdr, &fdw)) {
        maxfd = i;
      }
    }

    retval = select(maxfd + 1, &fdr, &fdw, NULL, &tv);
    if(retval < 0) {
      if(errno != EINTR) {
        perror("select");
      }
    } else if(retval > 0) {
      /* timeout => retval == 0 */
      for(i = 0; i <= maxfd; i++) {
        if(select_callback[i] != NULL) {
          select_callback[i]->handle_fd(&fdr, &fdw);
        }
      }
    }

    etimer_request_poll();
  }

  return;
}
/*---------------------------------------------------------------------------*/
void
log_message(char *m1, char *m2)
{
  fprintf(stderr, "%s%s\n", m1, m2);
}
/*---------------------------------------------------------------------------*/
void
uip_log(char *m)
{
  fprintf(stderr, "%s\n", m);
}
/*---------------------------------------------------------------------------*/
/** @} */
