/*
 * cronoutils -- utilities for the cronolog program
 * $Id: cronoutils.h,v 1.7 1998/03/10 10:58:40 andrew Exp $
 *
 * Copyright (C) 1996-1998 by Ford & Mason Ltd.
 *
 * Written by Andrew Ford
 *	mailto:A.Ford@ford-mason.co.uk
 * 	http://www.ford-mason.co.uk/aford/
 *
 * The file LICENSE specifies the terms and conditions for redistribution.
 *
 * The latest version of cronolog can be found at:
 *
 *	http://www.ford-mason.co.uk/resources/cronolog/
 *
 * For platforms that don't declare getopt() in header files the symbol
 * NEED_GETOPT_DEFS can be defined and declarations are provided here.
 */

#if !defined(_CRONOUTILS_H_)

/* Header files */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "config.h"

#if !HAVE_LOCALIME_R
struct tm *localtime_r(const time_t *, struct tm *);
#endif

/* Some operating systems don't declare getopt() */

#ifdef NEED_GETOPT_DEFS
int getopt(int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;
#endif


/* Constants for seconds per minute, hour, day and week */

#define SECS_PER_MIN		60
#define SECS_PER_HOUR		(60 * SECS_PER_MIN)
#define SECS_PER_DAY		(24 * SECS_PER_HOUR)
#define SECS_PER_WEEK		(7  * SECS_PER_DAY)


/* Allowances for daylight saving time changes and leap second.
 * Used for calculating the start of the next period.  
 * (does Unix actually know about leap seconds?)
 */

#define LEAP_SECOND_ALLOWANCE	2
#define DST_ALLOWANCE		(3 * SECS_PER_HOUR + LEAP_SECOND_ALLOWANCE)


/* If log files are not rotated then this is when the first file
 * should be closed. */

#define FAR_DISTANT_FUTURE	LONG_MAX


/* How often the log is rotated */

typedef enum 
{
    PER_SECOND, PER_MINUTE, HOURLY, DAILY, WEEKLY, MONTHLY, YEARLY, ONCE_ONLY, UNKNOWN
}
PERIODICITY;


/* Function prototypes */

void		create_subdirs(char *);
void		create_link(char *, const char *, mode_t);
PERIODICITY	determine_periodicity(char *);
time_t		start_of_next_period(time_t, PERIODICITY);
time_t		start_of_this_period(time_t, PERIODICITY);
void		print_debug_msg(char *msg, ...);
time_t		parse_time(char *time_str, int);
char 		*timestamp(time_t thetime);


/* Global variables */

extern FILE	*debug_file;
extern char	*periods[];


/* Usage message and DEBUG macro. 
 */ 

#ifdef DEBUG
#undef DEBUG
#endif

#define DEBUG(msg_n_args)	do { if (debug_file) print_debug_msg  msg_n_args; } while (0)

#endif
