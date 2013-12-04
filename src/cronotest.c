/*
 * cronotest -- test harness for cronoutils
 * $Id: cronotest.c,v 1.6 1999/12/16 18:32:06 andrew Exp $
 *
 * Copyright (C) 1997 by Ford & Mason Ltd.
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
 * Usage:
 *
 * 	cronotest [OPTIONS] template count
 */

#include "cronoutils.h"
#include "getopt.h"


#define VERSION_MSG 	"%s: test program for " PACKAGE " version " VERSION "\n" \
			"\n" \
			"Copyright (C) 1997,1998 Ford & Mason Ltd.\n" \
			"This is free software; see the source for copying conditions.\n" \
			"There is NO warranty; not even for MERCHANTABILITY or FITNESS\n" \
			"FOR A PARTICULAR PURPOSE.\n" \
			"\n" \
			"Written by Andrew Ford <andrew@icarus.demon.co.uk>\n"

#define USAGE_MSG 	"usage: %s [OPTIONS] template count\n" \
			"\n"\
			"   -a,    --american         American date formats\n" \
			"   -e,    --european         European date formats (default)\n" \
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


char 		*short_options = "aes:z:hVvd";
struct option   long_options[] =
{
    { "american",	no_argument,		NULL, 'a' },
    { "european",	no_argument,		NULL, 'e' },
    { "start-time", 	required_argument,	NULL, 's' },
    { "time-zone",  	required_argument,	NULL, 'z' },
    { "help",       	no_argument,		NULL, 'h' },
    { "version",	no_argument,		NULL, 'V' },
    { "verbose",	no_argument,		NULL, 'v' }
};


/* Test harness for determine_periodicity and start_of_this/next_period
 */
int
main(int argc, char **argv, char **envp)
{
    PERIODICITY periodicity;
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
	    
	case 'd':
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
    periodicity = determine_periodicity(template);

    printf("Rotation period is per %s\n", periods[periodicity]);

    tm = localtime(&time_now);
    strftime(buf, sizeof (buf), "%c %Z", tm);
    printf("Start time is %s (%ld)\n", buf, time_now);
    time_now = start_of_this_period(time_now, periodicity);

    for (i = 1; i <= n; i++)
    {
	tm = localtime(&time_now);
	strftime(buf, sizeof (buf), "%c %Z", tm);
	printf("Period %d starts at %s (%ld):  ", i, buf, time_now);
	strftime(filename, MAX_PATH, template, tm);
	printf("\"%s\"\n", filename);
	if (test_subdir_creation)
	{
	    create_subdirs(filename);
	}
	time_now = start_of_next_period(time_now, periodicity);
    }

    return 0;
}



