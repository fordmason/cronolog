/*
 * cronolog -- simple program to rotate Apache logs without having to kill the server.
 * $Id: cronolog.c,v 1.10 1996/12/20 10:58:49 andrew Exp $
 *
 * Copyright (C), 1996 Andrew Ford 
 *	mailto:andrew@icarus.demon.co.uk
 * 	http://www.nhbs.co.uk/aford/
 *
 * The latest version of cronolog can be found at:
 *
 *	http://www.nhbs.co.uk/aford/resources/apache/cronolog/
 *
 * The file LICENSE specifies the terms and conditions for redistribution.
 *
 * cronolog is loosly based on a program by Ben Laurie <ben@algroup.co.uk>
 *
 * The argument to this program is the log file name template as an
 * strftime format string.  For example to generate new error and
 * access logs each day stored in subdirectories by year, month and day add
 * the following lines to the httpd.conf:
 *
 *	TransferLog "|/www/etc/cronolog /www/logs/%Y/%m/%d/access.log"
 *	ErrorLog    "|/www/etc/cronolog /www/logs/%Y/%m/%d/error.log"
 *
 * The program creates any new directories that may be needed, unless
 * it is compiled with -DDONT_CREATE_SUBDIRS, in which case any
 * directories needed must exist or the program will abort.
 *
 * When creating new directories, common prefixes of the current log
 * file and the last file for which subdirectories were created are
 * not checked with stat(), unless CHECK_ALL_PREFIX_DIRS is defined.
 *
 * If compiled with -DDEBUG then the option "-x file" specifies that
 * debugging messages should be written to "file" (e.g. /dev/console)
 * or to stderr if "file" is "-".
 *
 * Compiling with -DTESTHARNESS generates a test program for testing
 * the functions that determine the start of each period -- run with:
 *
 * 	testprog spec count 
 *
 * For platforms that don't declare getopt() in header files the symbol
 * NEED_GETOPT_DEFS can be defined and declarations are provided here.
 * 
 * Future directions: cronolog may be rewritten as an Apache module.
 *
 * VERSION HISTORY
 *
 * Version 1.0 06-Dec-1996 - Andrew Ford <andrew@icarus.demon.co.uk>
 *      Initial version sent to Apache developers' mailing list 
 *	(cronolog was then called strftimelog)
 *    
 * Version 1.1 09-Dec-1996 - Andrew Ford <andrew@icarus.demon.co.uk>
 *	Fixed problem with log files being created with the wrong
 *	dates.  (When a new log file is due to be created, the time
 *	used for filling out the template should be the start of the
 *	current period and not the time at which a new log file is
 *	due.  This was observed with daily logs where there were no
 *	transfers for two days; the next log file generated had the
 *	date of the first of the days with no transfers.)
 *
 *	Also added more comments and fixed handling of week numbers
 *	where weeks start on a Monday.  
 *
 * Version 1.2 15-Dec-1996 - Andrew Ford <andrew@icarus.demon.co.uk>
 *	First version announced on comp.infosys.www.servers.unix.
 *	(renamed to cronolog).
 *	Creates missing directories as needed.
 *
 * Version 1.3 16-Dec-1996 - Andrew Ford <andrew@icarus.demon.co.uk>
 * 	Fixed portability bugs. 
 *
 * Version 1.4 20-Dec-1996 - Andrew Ford <andrew@icarus.demon.co.uk>
 *	Optimization of create_subdirs: don't bother stat'ing directories
 *	that form part of a common prefix with the last filename checked. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>


/* Some operating systems don't declare getopt() */

#ifdef NEED_GETOPT_DEFS
int getopt(int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;
#endif


/* Size of read buffer and filename buffer */

#define BUFSIZE			65536
#define MAX_PATH		1024


/* Default permissions for files and directories that are created */

#ifndef FILE_MODE
#define FILE_MODE	( S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH )
#endif

#ifndef DIR_MODE
#define DIR_MODE	( S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH )
#endif




/* Seconds per minute, hour and day */

#define SECS_PER_MIN		60
#define SECS_PER_HOUR		(60 * SECS_PER_MIN)
#define SECS_PER_DAY		(24 * SECS_PER_HOUR)


/* Allowance for leap seconds (does Unix actually know about them?) */

#define LEAP_SECOND_ALLOWANCE	2


/* If log files are not rotated then this is when the first file
 * should be closed. */

#define FAR_DISTANT_FUTURE	LONG_MAX


/* How often the log is rotated */

typedef enum 
{
    PER_MINUTE, HOURLY, DAILY, WEEKLY, MONTHLY, YEARLY, ONCE_ONLY
}
PERIODICITY;

char	*periods[] = 
{
    "minute",
    "hour",
    "day",
    "week",
    "month",
    "year",
    "aeon"
};


#ifndef DONT_CREATE_SUBDIRS
static void		create_subdirs(char *filename);
#endif
static PERIODICITY	determine_periodicity(char *);
static time_t		start_of_next_period(time_t, PERIODICITY);
static time_t		start_of_this_period(time_t, PERIODICITY);


/* America and Europe disagree on whether weeks start on Sunday or
 * Monday - this is set if a %U specifier is encountered.
 */

int	weeks_start_on_mondays = 0;


/* Usage message and DEBUG macro. 
 */ 
#ifdef TESTHARNESS
#undef DEBUG
#define DEBUG
#endif

#ifndef DEBUG
#undef  DEBUGGING
#define USAGE_MSG1		"usage: %s [-v] <logfile-spec>\n" 
#define DEBUG(msg_n_args)	
#else
#undef DEBUG
#define DEBUGGING
#define USAGE_MSG1		"usage: %s [-x file | -v ] <logfile-spec>\n" 
#define DEBUG(msg_n_args)	do { if (debug_file) debug msg_n_args; } while (0)
static void			debug(char *msg, ...);
FILE				*debug_file = NULL;
#endif

#define USAGE_MSG2	"see \"http://www.nhbs.co.uk/aford/resources/apache/cronolog/\" for further information.\n"
    


/* Main function.  Two alternate versions are provided: the real one
 * and a test harness
 */
#ifndef TESTHARNESS
void 
main(int argc,
     char **argv)
{
    PERIODICITY	periodicity;
    char	*logfile_spec;
    time_t	time_now;
    time_t 	start_of_period;
    time_t	next_period = 0;
    char	filename[MAX_PATH];
    char 	read_buf[BUFSIZE];
    int 	n_bytes_read;
    char	ch;
    struct tm 	*tm;
    int 	log_fd = -1;

    while ((ch = getopt(argc, argv, "vx:")) != EOF)
    {
	switch (ch)
	{
	case 'v':
	    fprintf(stderr, "cronolog version %s\n", VERSION);
	    exit(0);
	    
	case 'x':
#ifdef DEBUGGING
	    if (strcmp(optarg, "-") == 0)
	    {
		debug_file = stderr;
	    }
	    else
	    {
		debug_file = fopen(optarg, "a+");
	    }
#endif
	    break;
	    
	case '?':
	    fprintf(stderr, USAGE_MSG1 USAGE_MSG2, argv[0]);
	    exit(1);
	}
    }

    if (optind == argc)
    {
	fprintf(stderr, USAGE_MSG1 USAGE_MSG2, argv[0]);
	exit(1);
    }
    
    logfile_spec = argv[optind];
    periodicity  = determine_periodicity(logfile_spec);

    DEBUG(("periodicity = %s\n", periods[periodicity]));


    /* Loop, waiting for data on standard input */

    for (;;)
    {
	/* Read a buffer's worth of log file data, exiting on errors
	 * or end of file.
	 */
	n_bytes_read = read(0, read_buf, sizeof read_buf);
	if (n_bytes_read == 0)
	{
	    exit(3);
	}
	if ((n_bytes_read < 0) && (errno != EINTR))
	{
	    exit(4);
	}

	/* If there is a log file open and the current period has
	 * finished, close the log file.
	 */
	time_now = time(NULL);
	if (log_fd >= 0 && (time_now >= next_period))
	{
	    close(log_fd);
	    log_fd = -1;
	}

	/* If there is no log file determine the start of the current
	 * period, generate the log file name from the template,
	 * determine the end of the period and open the new log file.
	 */
	if (log_fd < 0)
	{
	    start_of_period = start_of_this_period(time_now, periodicity);
	    tm = localtime(&start_of_period);
	    strftime(filename, BUFSIZE, logfile_spec, tm);
	    next_period = start_of_next_period(time_now, periodicity);
	
	    DEBUG(("Using log file \"%s\" until %ld (current time is %ld)\n", 
		   filename, next_period, time_now));

	    log_fd = open(filename, O_WRONLY|O_CREAT|O_APPEND, FILE_MODE);

#ifndef DONT_CREATE_SUBDIRS
	    if ((log_fd < 0) && (errno == ENOENT))
	    {
		create_subdirs(filename);
		log_fd = open(filename, O_WRONLY|O_CREAT|O_APPEND, FILE_MODE);
	    }
#endif	    
	    if (log_fd < 0)
	    {
		perror(filename);
		exit(2);
	    }
	}

	/* Write out the log data to the current log file.
	 */
	if (write(log_fd, read_buf, n_bytes_read) != n_bytes_read)
	{
	    perror(filename);
	    exit(5);
	}
    }

    /* NOTREACHED */
}

#else

/* Test harness for determine_periodicity and start_of_this/next_period
 */
void
main(int argc, char **argv)
{
    PERIODICITY periodicity;
    time_t	now;
    struct tm 	*tm;
    char	*spec;
    char	ch;
    int		n;
    int		i;
    char	buf[BUFSIZE];
    char	filename[MAX_PATH];
    int		test_subdir_creation = 0;

    debug_file = stderr;
    
    while ((ch = getopt(argc, argv, "d")) != EOF)
    {
	switch (ch)
	{
	case 'd':
	    test_subdir_creation++;
	    break;
	    
	case '?':
	    fprintf(stderr, "usage: %s [-d] spec count\n", argv[0]);
	    exit(1);
	}
    }

    if (optind != argc - 2)
    {
	fprintf(stderr, "usage: %s [-d] spec count\n", argv[0]);
	exit(1);
    }
    
    spec = argv[optind++];
    n    = atoi(argv[optind]);
    periodicity = determine_periodicity(spec);

    now = time(NULL);
    printf("Rotation period is per %s\n", periods[periodicity]);

    tm = localtime(&now);
    strftime(buf, BUFSIZE, "%c", tm);
    printf("Start time is %s (%ld)\n", buf, now);
    now = start_of_this_period(time(NULL), periodicity);

    for (i = 1; i <= n; i++)
    {
	tm = localtime(&now);
	strftime(buf, BUFSIZE, "%c", tm);
	printf("Period %3d starts at %s (%ld):  ", i, buf, now);
	strftime(filename, MAX_PATH, spec, tm);
	printf("\"%s\"\n", filename);
	if (test_subdir_creation)
	{
	    create_subdirs(filename);
	}
	now = start_of_next_period(now, periodicity);
    }
}
#endif


/* Try to create missing directories on the path of filename.
 *
 * Note that on a busy server there may theoretically be many cronolog
 * processes trying simultaneously to create the same subdirectories
 * so ignore any EEXIST errors on mkdir -- they probably just mean
 * that another process got there first.
 *
 * Unless CHECK_ALL_PREFIX_DIRS is defined, we save the directory of
 * the last file tested -- any common prefix should exist.  This
 * probably only saves a few stat system calls at the start of each
 * log period, but it might as well be done.
 */
#ifndef DONT_CREATE_SUBDIRS
static void
create_subdirs(char *filename)
{
#ifndef CHECK_ALL_PREFIX_DIRS
    static char	lastpath[MAX_PATH] = "";
#endif
    struct stat stat_buf;
    char	dirname[MAX_PATH];
    char	*p;
    
    DEBUG(("Creating missing components of \"%s\"\n", filename));
    for (p = filename; (p = strchr(p, '/')); p++)
    {
	if (p == filename)
	{
	    continue;	    /* Don't bother with the root directory */
	}
	
	memcpy(dirname, filename, p - filename);
	dirname[p-filename] = '\0';
	
#ifndef CHECK_ALL_PREFIX_DIRS
	if (strncmp(dirname, lastpath, strlen(dirname)) == 0)
	{
	    DEBUG(("Initial prefix \"%s\" known to exist\n", dirname));
	    continue;
	}
#endif

	DEBUG(("Testing directory \"%s\"\n", dirname));
	if (stat(dirname, &stat_buf) < 0)
	{
	    if (errno != ENOENT)
	    {
		perror(dirname);
		exit(2);
	    }
	    else
	    {
		DEBUG(("Directory \"%s\" does not exist -- creating\n", dirname));
		if ((mkdir(dirname, DIR_MODE) < 0) && (errno != EEXIST))
		{
		    perror(dirname);
		    exit(2);
		}
	    }
	}
    }
#ifndef CHECK_ALL_PREFIX_DIRS
    strcpy(lastpath, dirname);
#endif
}
#endif



/* Examine the log file name specifier for strftime conversion
 * specifiers and determine the period between log files. 
 * Smallest period allowed is per minute.
 */
PERIODICITY
determine_periodicity(char *spec)
{
    PERIODICITY	periodicity = ONCE_ONLY;
    char 	ch;
    
    DEBUG(("Determining periodicity of \"%s\"\n", spec));
    while ((ch = *spec++) != 0)
    {
	if (ch == '%')
	{
	    ch = *spec++;
	    if (!ch) break;
	    
	    switch (ch)
	    {
	    case 'y':		/* two digit year */
	    case 'Y':		/* four digit year */
		if (periodicity > YEARLY)
		{
		    DEBUG(("%%%c -> yearly\n", ch));
		    periodicity = YEARLY;
		}
		break;

	    case 'b':		/* abbreviated month name */
	    case 'h':		/* abbreviated month name (non-standard) */
	    case 'B':		/* full month name */
	    case 'm':		/* month as two digit number (with
				   leading zero) */
		if (periodicity > MONTHLY)
		{
		    DEBUG(("%%%c -> monthly\n", ch));
		    periodicity = MONTHLY;
		}
  	        break;
		
	    case 'U':		/* week number (weeks start on Sunday) */
	    case 'W':		/* week number (weeks start on Monday) */
	        if (periodicity > WEEKLY)
		{
		    DEBUG(("%%%c -> weeky\n", ch));
		    periodicity = WEEKLY;
		    weeks_start_on_mondays = (ch == 'W');
		}
		break;
		
	    case 'a':		/* abbreviated weekday name */
	    case 'A':		/* full weekday name */
	    case 'd':		/* day of the month (with leading zero) */
	    case 'e':		/* day of the month (with leading space -- non-standard) */
	    case 'j':		/* day of the year (with leading zeroes) */
	    case 'w':		/* day of the week (0-6) */
	    case 'D':		/* full date spec (non-standard) */
	    case 'x':		/* full date spec */
	        if (periodicity > DAILY)
		{
		    DEBUG(("%%%c -> daily\n", ch));
		    periodicity = DAILY;
		}
  	        break;
	    
	    case 'H':		/* hour (24 hour clock) */
	    case 'I':		/* hour (12 hour clock) */
	    case 'p':		/* AM/PM indicator */
	        if (periodicity > HOURLY)
		{
		    DEBUG(("%%%c -> hourly\n", ch));
		    periodicity = HOURLY;
		}
		break;
		
	    case 'M':		/* minute */
	    case 'c':		/* full time and date spec */
	    case 'T':		/* full time spec */
	    case 'r':		/* full time spec (non-standard) */
	    case 'R':		/* full time spec (non-standard) */
	    case 's':		/* seconds since the epoch (GNU non-standard) */
		DEBUG(("%%%c -> per minute\n", ch));
	        periodicity = PER_MINUTE;
		break;
		
	    default:		/* ignore anything else */
		DEBUG(("ignoring %%%c\n", ch));
	        break;
	    }
	}
    }
    return periodicity;
}




/* To determine the time of the start of the next period add just
 * enough to move into the start of the period and then determine the
 * start of the period.
 *
 * There is a potential problem if the start or end of daylight saving
 * time occurs at the start of the period.
 */
static time_t
start_of_next_period(time_t time_now, 
		     PERIODICITY periodicity)
{
    time_t	start_time;
    struct tm	*tm;
    
    switch (periodicity)
    {
    case YEARLY:
	tm = localtime(&time_now);	
	start_time = time_now + (366 - tm->tm_yday) * SECS_PER_DAY + LEAP_SECOND_ALLOWANCE;
	break;

    case MONTHLY:
	tm = localtime(&time_now);	
	start_time = time_now + (32 - tm->tm_mday) * SECS_PER_DAY + LEAP_SECOND_ALLOWANCE;
	break;

    case WEEKLY:
	start_time = time_now + 7 * SECS_PER_DAY + LEAP_SECOND_ALLOWANCE;
	break;
	
    case DAILY:
	start_time = time_now + SECS_PER_DAY + LEAP_SECOND_ALLOWANCE;
	break;
	
    case HOURLY:
	start_time = time_now + SECS_PER_HOUR + LEAP_SECOND_ALLOWANCE;
	break;

    case PER_MINUTE:
	start_time = time_now + SECS_PER_MIN + LEAP_SECOND_ALLOWANCE;
	break;

    default:
	start_time = FAR_DISTANT_FUTURE;
	break;
    }
    return start_of_this_period(start_time, periodicity);
}



/* To determine the time of the start of the period containing a given
 * time just break down the time with localtime and subtract the
 * number of seconds since the start of the period. 
 */
static time_t
start_of_this_period(time_t 		start_time, 
		     PERIODICITY 	periodicity)
{
    struct tm	*tm;
    
    switch (periodicity)
    {
    case YEARLY:
	tm = localtime(&start_time);
	start_time -= (  (SECS_PER_DAY  * tm->tm_yday)
		       + (SECS_PER_HOUR * tm->tm_hour)
		       + (SECS_PER_MIN  * tm->tm_min)
		       + tm->tm_sec);
	break;

    case MONTHLY:
	tm = localtime(&start_time);
	start_time -= (  (SECS_PER_DAY  * (tm->tm_mday - 1))
		       + (SECS_PER_HOUR * tm->tm_hour)
		       + (SECS_PER_MIN  * tm->tm_min)
		       + tm->tm_sec);
	break;

    case WEEKLY:
	tm = localtime(&start_time);
	if (weeks_start_on_mondays)
	{
	    tm->tm_wday = (6 + tm->tm_wday) % 7;
	}
	start_time -= (  (SECS_PER_DAY  * tm->tm_wday)
		       + (SECS_PER_HOUR * tm->tm_hour)
		       + (SECS_PER_MIN  * tm->tm_min)
		       + tm->tm_sec);
	break;
	
    case DAILY:
	tm = localtime(&start_time);
	start_time -= (  (SECS_PER_HOUR * tm->tm_hour)
		       + (SECS_PER_MIN  * tm->tm_min)
		       + tm->tm_sec);
	break;
	
    case HOURLY:
	tm = localtime(&start_time);
	start_time -= (SECS_PER_MIN * tm->tm_min) + tm->tm_sec;
	break;

    case PER_MINUTE:
	tm = localtime(&start_time);
	start_time -= tm->tm_sec;
	break;

    default:
	break;
    }
    return start_time;
}


/* Simple debugging function 
 */
#ifdef DEBUGGING
static void
debug(char *msg, ...)
{
    va_list	ap;
    
    va_start(ap, msg);
    vfprintf(debug_file, msg, ap);
}
#endif
