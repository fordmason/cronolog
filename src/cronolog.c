/*
 * cronolog -- simple program to rotate Apache logs without having to kill the server.
 * $Id: cronolog.c,v 1.6 1998/03/10 10:58:39 andrew Exp $
 *
 * Copyright (C) 1996-1998 by Andrew Ford and Ford & Mason Ltd
 *	mailto:andrew@icarus.demon.co.uk
 * 	http://www.nhbs.co.uk/aford/
 *
 * The file LICENSE specifies the terms and conditions for redistribution.
 *
 * The latest version of cronolog can be found at:
 *
 *	http://www.nhbs.co.uk/aford/resources/apache/cronolog/
 *
 * cronolog is loosly based on the rotatelogs program, which is part of the
 * Apache package written by Ben Laurie <ben@algroup.co.uk>
 *
 * The argument to this program is the log file name template as an
 * strftime format string.  For example to generate new error and
 * access logs each day stored in subdirectories by year, month and day add
 * the following lines to the httpd.conf:
 *
 *	TransferLog "|/www/etc/cronolog /www/logs/%Y/%m/%d/access.log"
 *	ErrorLog    "|/www/etc/cronolog /www/logs/%Y/%m/%d/error.log"
 *
 * The option "-x file" specifies that debugging messages should be
 * written to "file" (e.g. /dev/console) or to stderr if "file" is "-".
 */

#include "cronoutils.h"
#include "getopt.h"


/* Forward function declaration */

int	new_log_file(const char *, const char *, mode_t, 
		     PERIODICITY, char *, size_t, time_t, time_t *);


/* Definition of version and usage messages */

#define VERSION_MSG   	PACKAGE " version " VERSION "\n" \
			"\n" \
			"Copyright (C) 1997, 1998 Ford & Mason Ltd.\n" \
			"This is free software; see the source for copying conditions.\n" \
			"There is NO warranty; not even for MERCHANTABILITY or FITNESS\n" \
			"FOR A PARTICULAR PURPOSE.\n" \
			"\n" \
			"Written by Andrew Ford <A.Ford@ford-mason.co.uk>\n" \
			"\n" \
			"The latest version can be found at:\n" \
			"\n" \
			"    http://www.ford-mason.co.uk/resources/cronolog/\n"


#define USAGE_MSG 	"usage: %s [OPTIONS] logfile-spec\n" \
			"\n" \
			"   -H NAME, --hardlink=NAME maintain a hard link from NAME to current log\n" \
			"   -S NAME, --symlink=NAME  maintain a symbolic link from NAME to current log\n" \
			"   -l NAME, --link=NAME     same as -S/--symlink\n" \
			"   -h,      --help          print this help, then exit\n" \
			"   -o,      --once-only     create single output log from template (not rotated)\n" \
			"   -x FILE, --debug=FILE    write debug messages to FILE\n" \
			"                            ( or to standard error if FILE is \"-\")\n" \
			"   -a,    --american         American date formats\n" \
			"   -e,    --european         European date formats (default)\n" \
			"   -s,    --start-time=TIME  starting time\n" \
			"   -z TZ, --time-zone=TZ     use TZ for timezone\n" \
			"   -V,      --version       print version number, then exit\n"


/* Definition of the short and long program options */

char          *short_options = "aes:z:oH:S:l:hVx:";
struct option long_options[] =
{
    { "american",	no_argument,		NULL, 'a' },
    { "european",	no_argument,		NULL, 'e' },
    { "start-time", 	required_argument,	NULL, 's' },
    { "time-zone",  	required_argument,	NULL, 'z' },
    { "hardlink",  	required_argument, 	NULL, 'H' },
    { "symlink",   	required_argument, 	NULL, 'S' },
    { "link",      	required_argument, 	NULL, 'l' },
    { "once-only", 	no_argument,       	NULL, 'o' },
    { "help",      	no_argument,       	NULL, 'h' },
    { "version",   	no_argument,       	NULL, 'V' }
};

/* Main function.
 */
int
main(int argc, char **argv)
{
    PERIODICITY	periodicity = UNKNOWN;
    int		use_american_date_formats = 0;
    char 	read_buf[BUFSIZE];
    char 	tzbuf[BUFSIZE];
    char	filename[MAX_PATH];
    char	*start_time = NULL;
    char	*template;
    char	*linkname = NULL;
    mode_t	linktype = 0;
    int 	n_bytes_read;
    int		ch;
    time_t	time_now;
    time_t	time_offset = 0;
    time_t	next_period = 0;
    int 	log_fd = -1;

    while ((ch = getopt_long(argc, argv, short_options, long_options, NULL)) != EOF)
    {
	switch (ch)
	{
	case 'a':
	    use_american_date_formats = 1;
	    break;
	    
	case 'e':
	    use_american_date_formats = 0;
	    break;
	    
	case 's':
	    start_time = optarg;
	    break;

	case 'z':
	    sprintf(tzbuf, "TZ=%s", optarg);
	    putenv(tzbuf);
	    break;

	case 'H':
	    linkname = optarg;
	    linktype = S_IFREG;
	    break;

	case 'l':
	case 'S':
	    linkname = optarg;
	    linktype = S_IFLNK;
	    break;
	    
	case 'o':
	    periodicity = ONCE_ONLY;
	    break;
	    
	case 'x':
	    if (strcmp(optarg, "-") == 0)
	    {
		debug_file = stderr;
	    }
	    else
	    {
		debug_file = fopen(optarg, "a+");
	    }
	    break;
	    
	case 'V':
	    fprintf(stderr, VERSION_MSG);
	    exit(0);
	    
	case 'h':
	case '?':
	    fprintf(stderr, USAGE_MSG, argv[0]);
	    exit(1);
	}
    }

    if ((argc - optind) != 1)
    {
	fprintf(stderr, USAGE_MSG, argv[0]);
	exit(1);
    }

    DEBUG((VERSION_MSG "\n"));

    if (start_time)
    {
	time_now = parse_time(start_time, use_american_date_formats);
	if (time_now == -1)
	{
	    fprintf(stderr, "%s: invalid start time (%s)\n", argv[0], start_time);
	    exit(1);
	}
	time_offset = time_now - time(NULL);
	DEBUG(("Using offset of %d seconds from real time\n", time_offset));
    }

    /* The template should be the only argument.
     * Unless the -o option was specified, determine the periodicity.
     */
    
    template = argv[optind];
    if (periodicity == UNKNOWN)
    {
	periodicity = determine_periodicity(template);
    }


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
	if (errno == EINTR)
	{
	    continue;
	}
	else if (n_bytes_read < 0)
	{
	    exit(4);
	}

	time_now = time(NULL) + time_offset;
	
	/* If the current period has finished and there is a log file
	 * open, close the log file
	 */
	if ((time_now >= next_period) && (log_fd >= 0))
	{
	    close(log_fd);
	    log_fd = -1;
	}
	
	/* If there is no log file open then open a new one.
	 */
	if (log_fd < 0)
	{
	    log_fd = new_log_file(template, linkname, linktype, periodicity,
				  filename, sizeof (filename), time_now, &next_period);
	}

	DEBUG(("%s (%d): wrote message; next period starts at %s (%d) in %d secs\n",
	       timestamp(time_now), time_now, 
	       timestamp(next_period), next_period,
	       next_period - time_now));

	/* Write out the log data to the current log file.
	 */
	if (write(log_fd, read_buf, n_bytes_read) != n_bytes_read)
	{
	    perror(filename);
	    exit(5);
	}
    }

    /* NOTREACHED */
    return 1;
}

/* Open a new log file: determine the start of the current
 * period, generate the log file name from the template,
 * determine the end of the period and open the new log file.
 *
 * Returns the file descriptor of the new log file and also sets the
 * name of the file and the start time of the next period via pointers
 * supplied.
 */
int
new_log_file(const char *template, const char *linkname, mode_t linktype,
	     PERIODICITY periodicity, char *pfilename, size_t pfilename_len,
	     time_t time_now, time_t *pnext_period)
{
    time_t 	start_of_period;
    struct tm 	*tm;
    int 	log_fd;

    start_of_period = start_of_this_period(time_now, periodicity);
    tm = localtime(&start_of_period);
    strftime(pfilename, BUFSIZE, template, tm);
    *pnext_period = start_of_next_period(time_now, periodicity);
    
    DEBUG(("%s (%d): using log file \"%s\" until %s (%d) (for %d secs)\n",
	   timestamp(time_now), time_now, pfilename, 
	   timestamp(*pnext_period), *pnext_period,
	   *pnext_period - time_now));
    
    log_fd = open(pfilename, O_WRONLY|O_CREAT|O_APPEND, FILE_MODE);
    
#ifndef DONT_CREATE_SUBDIRS
    if ((log_fd < 0) && (errno == ENOENT))
    {
	create_subdirs(pfilename);
	log_fd = open(pfilename, O_WRONLY|O_CREAT|O_APPEND, FILE_MODE);
    }
#endif	    

    if (log_fd < 0)
    {
	perror(pfilename);
	exit(2);
    }

    if (linkname)
    {
	create_link(pfilename, linkname, linktype);
    }
    return log_fd;
}
