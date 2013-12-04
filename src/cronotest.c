/* ====================================================================
 * Copyright (c) 1995-1999 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */
/*
 * cronotest -- test harness for cronoutils
 *
 * Copyright (c) 1996-1999 by Ford & Mason Ltd
 *
 * This software was submitted by Ford & Mason Ltd to the Apache
 * Software Foundation in December 1999.  Future revisions and
 * derivatives of this source code must acknowledge Ford & Mason Ltd
 * as the original contributor of this module.  All other licensing
 * and usage conditions are those of the Apache Software Foundation.
 *
 * Originally written by Andrew Ford <A.Ford@ford-mason.co.uk>
 *
 * Usage:
 *
 * 	cronotest [OPTIONS] template count
 */

#include "cronoutils.h"
#include "getopt.h"


#define VERSION_MSG 	"%s: test program for " PACKAGE " version " VERSION "\n"

#define USAGE_MSG 	"usage: %s [OPTIONS] template count\n" \
			"\n"\
			"   -a,    --american         American date formats\n" \
			"   -e,    --european         European date formats (default)\n" \
			"   -p PERIOD, --period=PERIOD set the rotation period explicitly\n" \
			"   -d DELAY,  --delay=DELAY   set the rotation period delay\n" \
			"   -s,    --start-time=TIME  starting time\n" \
			"   -z TZ, --time-zone=TZ     use TZ for timezone\n" \
			"   -h,    --help             print this help, then exit\n" \
			"   -v,    --verbose          print verbose messages\n" \
			"   -V,    --version          print version number, then exit\n" \
			"\n" \
			"Starting time can be expressed in one of the following formats:\n" \
			"\n" \
			"   dd month year [HH:MM[:SS]]\n" \
			"   dd mm year    [HH:MM[:SS]]\n" \
			"\n" \
			"If American date formats are selected then the day and month \n" \
			"specifiers are transposed.\n" \
			"\n"


char 		*short_options = "ad:ep:s:z:hVvD";
struct option   long_options[] =
{
    { "american",	no_argument,		NULL, 'a' },
    { "european",	no_argument,		NULL, 'e' },
    { "start-time", 	required_argument,	NULL, 's' },
    { "time-zone",  	required_argument,	NULL, 'z' },
    { "period",		required_argument,	NULL, 'p' },
    { "delay",		required_argument,	NULL, 'd' },
    { "test-subdirs",	required_argument,	NULL, 'D' },
    { "help",       	no_argument,		NULL, 'h' },
    { "version",	no_argument,		NULL, 'V' },
    { "verbose",	no_argument,		NULL, 'v' }
};


/* Test harness for determine_periodicity and start_of_this/next_period
 */
int
main(int argc, char **argv, char **envp)
{
    PERIODICITY periodicity = UNKNOWN;
    PERIODICITY period_delay_units = UNKNOWN;
    int		period_multiple = 1;
    int		period_delay    = 0;
    int		use_american_date_formats = 0;
    time_t	time_now = time(NULL);
    struct tm 	*tm;
    char	*start_time = NULL;
    char	*template;
    int		ch;
    int		n;
    int		i;
    char	buf[BUFSIZE];
    char	filename[MAX_PATH];
    int		test_subdir_creation = 0;

    debug_file = stdout;
    
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
	    
	case 'd':
	    period_delay_units = parse_timespec(optarg, &period_delay);
	    break;

	case 'p':
	    periodicity = parse_timespec(optarg, &period_multiple);
	    if (   (periodicity == INVALID_PERIOD)
		|| (periodicity == PER_SECOND) && (60 % period_multiple)
		|| (periodicity == PER_MINUTE) && (60 % period_multiple)
		|| (periodicity == HOURLY)     && (24 % period_multiple)
		|| (periodicity == DAILY)      && (period_multiple > 365)
		|| (periodicity == WEEKLY)     && (period_multiple > 52)
		|| (periodicity == MONTHLY)    && (12 % period_multiple)) {
		fprintf(stderr, "%s: invalid explicit period specification (%s)\n", argv[0], start_time);
		exit(1);
	    }		
	    break;
	    
	case 's':
	    start_time = optarg;
	    break;

	case 'z':
	    sprintf(buf, "TZ=%s", optarg);
	    putenv(buf);
	    break;

	case 'V':
	    fprintf(stderr, VERSION_MSG, argv[0]);
	    exit(0);
	    
	case 'D':
	    test_subdir_creation++;
	    break;
	    
	case 'v':
	    debug_file = stdout;
	    break;
	    
	case 'h':
	case '?':
	    fprintf(stderr, USAGE_MSG, argv[0]);
	    exit(1);
	}
    }

    if (optind != argc - 2)
    {
	fprintf(stderr, USAGE_MSG, argv[0]);
	exit(1);
    }

    if (start_time)
    {
	time_now = parse_time(start_time, use_american_date_formats);
	if (time_now == -1)
	{
	    fprintf(stderr, "%s: invalid start time (%s)\n", argv[0], start_time);
	    exit(1);
	}
    }
    
    
    template = argv[optind++];
    n    = atoi(argv[optind]);
    if (periodicity == UNKNOWN) {
	periodicity = determine_periodicity(template);
    }


    if (period_delay) {
	if (   (period_delay_units > periodicity)
	    || (   period_delay_units == periodicity
		&& abs(period_delay)  >= period_multiple)) {
	    fprintf(stderr, "%s: period delay cannot be larger than the rollover period\n", argv[0], start_time);
	    exit(1);
	}		
	period_delay *= period_seconds[period_delay_units];
    }

    printf("Rotation period is per %d %s\n", period_multiple, periods[periodicity]);

    tm = localtime(&time_now);
    strftime(buf, sizeof (buf), "%c %Z", tm);
    printf("Start time is %s (%ld)\n", buf, time_now);

    for (i = 1; i <= n; i++)
    {
	tm = localtime(&time_now);
	strftime(buf, sizeof (buf), "%c %Z", tm);
	printf("Period %d starts at %s (%ld):  ", i, buf, time_now);
	time_now = start_of_this_period(time_now, periodicity, period_multiple);
	tm = localtime(&time_now);
	strftime(filename, MAX_PATH, template, tm);
	printf("\"%s\"\n", filename);
	if (test_subdir_creation)
	{
	    create_subdirs(filename);
	}
	time_now = start_of_next_period(time_now, periodicity, period_multiple) + period_delay;
    }

    return 0;
}



