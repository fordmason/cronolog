/* Minimal localtime_r implementation (not reentrant though).
 * This file is in the public domain.
 *
 * Call localtime and if it succeeds copy the contents of the
 * localtime's static tm structure into the caller's structure.
 */

#include <errno.h>
#include <time.h>

struct tm *
localtime_r(const time_t *t, struct tm *ptm)
{
    struct tm *ptms = localtime(t); /* pointer to static structure */

    if (ptms)
    {
	*ptm = *ptms;
	return ptm;
    }
    else
    {
	return 0;
    }
}
