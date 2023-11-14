dnl Template file for GNU Autoconf
dnl
dnl Process this file with autoconf to produce a configure script.
dnl
dnl Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

dnl
dnl $Id: configure.ac 1671 2003-07-05 07:19:51Z andrew $
dnl


AC_REVISION($Id: configure.ac 1671 2003-07-05 07:19:51Z andrew $)

AC_INIT([cronolog],[1.6.3],[A.Ford@ford-mason.co.uk])
AC_SUBST([PACKAGE_NAME])dnl
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE


dnl Checks for programs.

AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_PATH_PROG(PERL, perl, /usr/local/bin/perl, /usr/bin:/usr/local/bin:/opt/bin:/usr/contrib/bin)


dnl Checks for libraries.

dnl Checks for header files.

AC_HEADER_STDC
AC_HEADER_STAT
AC_HEADER_TIME
AC_STRUCT_TM
AC_STRUCT_TIMEZONE
AC_CHECK_HEADERS([fcntl.h langinfo.h libintl.h limits.h locale.h stdlib.h string.h sys/time.h unistd.h])


dnl Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Checks for library functions.

AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([localtime_r memset mkdir nl_langinfo putenv strcasecmp strchr strncasecmp])
AC_REPLACE_FUNCS(strptime localtime_r)

AC_CONFIG_FILES([Makefile
		 cronolog.spec
                 doc/Makefile
                 lib/Makefile
                 src/Makefile
                 testsuite/Makefile])
AC_OUTPUT
