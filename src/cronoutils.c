/*
 * cronoutils -- utilities for the cronolog program
 * $Id: cronoutils.c,v 1.8 1999/12/16 18:32:06 andrew Exp $
 *
 * Copyright (C) 1996, 1997 by Ford & Mason Ltd.
 *
 * Written by Andrew Ford
 *	mailto:andrew@icarus.demon.co.uk
 * 	http://www.nhbs.co.uk/aford/
 *
 * The file LICENSE specifies the terms and conditions for redistribution.
 *
 * The latest version of cronolog can be found at:
 *
 *	http://www.nhbs.co.uk/aford/resources/apache/cronolog/
 *
 */

#include "cronoutils.h"
extern char *tzname[2];
extern long int timezone;


/* debug_file is the file to output debug messages to.  No debug
 * messages are output if it is set to NULL. 
 */
FILE	*debug_file = NULL;


/* America and Europe disagree on whether weeks start on Sunday or
 * Monday - weeks_start_on_mondays is set if a %U specifier is encountered.
 */
int	weeks_start_on_mondays = 0;


/* periods[] is an array of the names of the periods.
 */
char	*periods[] = 
{
    "second",
    "minute",
    "hour",
    "day",
    "week",
    "month",
    "year",
    "aeon"	/* i.e. once only */
};

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
void
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

/* Create a hard or symbolic link to a filename according to the type specified.
 *
 * This function could do with more error checking! 
 */
void
create_link(char *pfilename, const char *linkname, mode_t linktype)
{
    struct stat		stat_buf;
    
    if (stat(linkname, &stat_buf) == 0)
    {
	unlink(linkname);
    }

    if (linktype == S_IFLNK)
    {
	symlink(pfilename, linkname);
    }
    else
    {
	link(pfilename, linkname);
    }
}

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
	        if (periodicity > PER_MINUTE)
		{
		    DEBUG(("%%%c -> per minute\n", ch));
		    periodicity = PER_MINUTE;
		}
		break;
		
	    case 'S':		/* second */
	    case 's':		/* seconds since the epoch (GNU non-standard) */
	    case 'c':		/* full time and date spec */
	    case 'T':		/* full time spec */
	    case 'r':		/* full time spec (non-standard) */
	    case 'R':		/* full time spec (non-standard) */
		DEBUG(("%%%c -> per second", ch));
		periodicity = PER_SECOND;

	    default:		/* ignore anything else */
		DEBUG(("ignoring %%%c\n", ch));
	        break;
	    }
	}
    }
    return periodicity;
}

/* To determine the time of the start of the next period add just
 * enough to move beyond the start of the next period and then
 * determine the time of the start of that period.
 *
 * There is a potential problem if the start or end of daylight saving
 * time occurs during the current period. 
 */
time_t
start_of_next_period(time_t time_now, PERIODICITY periodicity)
{
    time_t	start_time;
    
    switch (periodicity)
    {
    case YEARLY:
	start_time = (time_now + 366 * SECS_PER_DAY + DST_ALLOWANCE);
	break;

    case MONTHLY:
	start_time = (time_now + 31 * SECS_PER_DAY + DST_ALLOWANCE);
	break;

    case WEEKLY:
	start_time = (time_now + SECS_PER_WEEK + DST_ALLOWANCE);
	break;
	
    case DAILY:
	start_time = (time_now + SECS_PER_DAY + DST_ALLOWANCE);
	break;
	
    case HOURLY:
	start_time = time_now + SECS_PER_HOUR + LEAP_SECOND_ALLOWANCE;
	break;

    case PER_MINUTE:
	start_time = time_now + SECS_PER_MIN + LEAP_SECOND_ALLOWANCE;
	break;

    case PER_SECOND:
	start_time = time_now + 1;
	break;

    default:
	start_time = FAR_DISTANT_FUTURE;
	break;
    }
    return start_of_this_period(start_time, periodicity);
}

/* Determine the time of the start of the period containing a given time.
 * Break down the time with localtime and subtract the number of
 * seconds since the start of the period.  If the length of period is
 * equal or longer than a day then we have to check tht the
 * calculation is not thrown out by the start or end of daylight
 * saving time.
 */
time_t
start_of_this_period(time_t start_time, PERIODICITY periodicity)
{
    struct tm	tm_initial;
    struct tm	tm_adjusted;
    int		expected_mday;
    
    localtime_r(&start_time, &tm_initial);

    switch (periodicity)
    {
    case YEARLY:
    case MONTHLY:
    case WEEKLY:
    case DAILY:
	switch (periodicity)
	{
	case YEARLY:
	    start_time -= (  (tm_initial.tm_yday * SECS_PER_DAY)
			   + (tm_initial.tm_hour * SECS_PER_HOUR)
			   + (tm_initial.tm_min  * SECS_PER_MIN)
			   + (tm_initial.tm_sec));
	    expected_mday = 1;
	    break;

	case MONTHLY:
	    start_time -= (  ((tm_initial.tm_mday - 1) * SECS_PER_DAY)
			   + ( tm_initial.tm_hour      * SECS_PER_HOUR)
			   + ( tm_initial.tm_min       * SECS_PER_MIN)
			   + ( tm_initial.tm_sec));
	    expected_mday = 1;
	    break;
	    
	case WEEKLY:
	    if (weeks_start_on_mondays)
	    {
		tm_initial.tm_wday = (6 + tm_initial.tm_wday) % 7;
	    }
	    start_time -= (  (tm_initial.tm_wday * SECS_PER_DAY)
			   + (tm_initial.tm_hour * SECS_PER_HOUR)
			   + (tm_initial.tm_min  * SECS_PER_MIN)
			   + (tm_initial.tm_sec));
	    expected_mday = tm_initial.tm_mday;
	    break;
	
	case DAILY:
	    start_time -= (  (tm_initial.tm_hour * SECS_PER_HOUR)
			   + (tm_initial.tm_min  * SECS_PER_MIN )
			   +  tm_initial.tm_sec);
	    expected_mday = tm_initial.tm_mday;
	    break;

	default:
	    fprintf(stderr, "software fault in start_of_this_period()\n");
	    exit(1);
	}

	/* If the time of day is not equal to midnight then we need to
	 * adjust for daylight saving time.  Adjust the time backwards
	 * by the value of the hour, minute and second fields.  If the
	 * day of the month is not as expected one then we must have
	 * adjusted back to the previous day so add 24 hours worth of
	 * seconds.  
	 */

	localtime_r(&start_time, &tm_adjusted);    
	if (   (tm_adjusted.tm_hour != 0)
	    || (tm_adjusted.tm_min  != 0)
	    || (tm_adjusted.tm_sec  != 0))
	{
	    char	sign   = '-';
	    time_t      adjust = - (  (tm_adjusted.tm_hour * SECS_PER_HOUR)
				    + (tm_adjusted.tm_min  * SECS_PER_MIN)
				    + (tm_adjusted.tm_sec));

	    if (tm_adjusted.tm_mday != expected_mday)
	    {
		adjust += SECS_PER_DAY;
		sign = '+';
	    }
	    start_time += adjust;

	    if (adjust < 0)
	    {
		adjust = -adjust;
	    }
	    
	    DEBUG(("Adjust for dst: %02d/%02d/%04d %02d:%02d:%02d -- %c%0d:%02d:%02d\n",
		   tm_initial.tm_mday, tm_initial.tm_mon+1, tm_initial.tm_year+1900,
		   tm_initial.tm_hour, tm_initial.tm_min,   tm_initial.tm_sec, sign,
		   adjust / SECS_PER_HOUR, (adjust / 60) % 60, adjust % SECS_PER_HOUR));
	}
	break;

    case HOURLY:
	start_time -= (tm_initial.tm_sec + tm_initial.tm_min * SECS_PER_MIN);
	break;

    case PER_MINUTE:
	start_time -= tm_initial.tm_sec;
	break;

    case PER_SECOND:	/* No adjustment needed */
    default:
	break;
    }
    return start_time;
}

/* Converts struct tm to time_t, assuming the data in tm is UTC rather
 * than local timezone (as mktime assumes).
 *  
 * Contributed by Roger Beeman <beeman@cisco.com>. 
 */
time_t
mktime_from_utc(struct tm *t)
{
   time_t tl, tb;

   tl = mktime(t);
   tb = mktime(gmtime(&tl));                               
   return (tl <= tb ? (tl + (tl - tb)) : (tl - (tb - tl)));
}

/* Check whether the string is processed well.  It is processed if the
 * pointer is non-NULL, and it is either at the `GMT', or at the end
 * of the string.
 */
static int
check_end(char *p)
{
   if (!p)
      return 0;
   while (isspace(*p))
      ++p;
   if (!*p || (p[0] == 'G' && p[1] == 'M' && p[2] == 'T'))
      return 1;
   else
      return 0;
}

/* NOTE: We don't use `%n' for white space, as OSF's strptime uses
   it to eat all white space up to (and including) a newline, and
   the function fails (!) if there is no newline.

   Let's hope all strptime-s use ` ' to skipp *all* whitespace
   instead of just one (it works that way on all the systems I've
   tested it on). */

static char *european_date_formats[] = 
{	
    "%d %b %Y %T",       	/* dd mmm yyyy HH:MM:SS */
    "%d %b %Y %H:%M",       	/* dd mmm yyyy HH:MM    */
    "%d %b %Y",       		/* dd mmm yyyy          */
    "%d-%b-%Y %T",       	/* dd-mmm-yyyy HH:MM:SS */
    "%d-%b-%Y %H:%M",       	/* dd-mmm-yyyy HH:MM    */
    "%d-%b-%y %T",       	/* dd-mmm-yy   HH:MM:SS */
    "%d-%b-%y %H:%M",  		/* dd-mmm-yy   HH:MM    */
    "%d-%b-%Y",
    "%b %d %T %Y",
    "%b %d %Y",
    NULL
};

static char *american_date_formats[] = 
{	
    "%b %d %Y %T",       	/* dd mmm yyyy HH:MM:SS */
    "%b %d %Y %H:%M",       	/* dd mmm yyyy HH:MM    */
    "%b %d %Y",       		/* dd mmm yyyy          */
    "%b-%d-%Y %T",       	/* dd-mmm-yyyy HH:MM:SS */
    "%b-%d-%Y %H:%M",       	/* dd-mmm-yyyy HH:MM    */
    "%b-%d-%Y",
    "%b/%d/%Y %T",
    "%b/%d/%Y %H:%M",
    "%b/%d/%Y",
    NULL
};



time_t
parse_time(char *time_str, int use_american_date_formats)
{
   struct tm    tm;
   char		**date_formats;
   
   memset(&tm, 0, sizeof (tm));
   tm.tm_isdst = -1;
   
   
   for (date_formats = (use_american_date_formats
			? american_date_formats
			: european_date_formats);
	*date_formats;
	date_formats++)
   {
       if (check_end(strptime(time_str, *date_formats, &tm)))
	   return mktime_from_utc(&tm);
   }
   
   return -1;
}
  


/* Simple debugging print function.
 */
void
print_debug_msg(char *msg, ...)
{
    va_list	ap;
    
    va_start(ap, msg);
    vfprintf(debug_file, msg, ap);
}


/* Build a timestamp and return a pointer to it.
 * (has a number of static buffers that are rotated).
 */ 
char *
timestamp(time_t thetime)
{
    static int	index = 0;
    static char	buffer[4][80];
    struct tm	*tm;
    char	*retval;

    retval = buffer[index++];
    index %= 4;
    
    tm = localtime(&thetime);
    strftime(retval, 80, "%Y/%m/%d-%H:%M:%S %Z", tm);
    return retval;
}

    
