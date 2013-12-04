/*
 * config.h -- configuration definitions  for the cronolog program
 * $Id: config.h,v 1.1 1998/03/08 11:02:56 andrew Exp $
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
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_


/* Size of read buffer and filename buffer */

#ifndef BUFSIZE
#define BUFSIZE			65536
#endif

#ifndef MAX_PATH
#define MAX_PATH		1024
#endif

/* Default permissions for files and directories that are created */

#ifndef FILE_MODE
#ifndef _WIN32
#define FILE_MODE	( S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH )
#else
#define FILE_MODE       ( _S_IREAD | _S_IWRITE )
#endif
#endif

#ifndef DIR_MODE
#define DIR_MODE	( S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH )
#endif


/* The program creates any new directories that may be needed, unless
 * DONT_CREATE_SUBDIRS is defined, in which case any directories
 * needed must exist or the program will abort. 
 *
 * Uncomment the following definition if you want the alternative functionality:
 */

/* #define DONT_CREATE_SUBDIRS */


/* When creating new directories, common prefixes of the current log
 * file and the last file for which subdirectories were created are
 * not checked with stat(), unless CHECK_ALL_PREFIX_DIRS is defined.
 *
 * Uncomment the following definition if you want the alternative functionality:
 */

/* #define CHECK_ALL_PREFIX_DIRS */




#endif
