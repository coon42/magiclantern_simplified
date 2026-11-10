#ifndef _edmac_util_h_
#define _edmac_util_h_

#include <edmac.h>

/* Describe the transfer geometry in readable form: "1920x1080" for
 * a plain image, "(1920, skip 32) x 1080" if bytes are skipped, and so
 * on for the more complex ones.  Returns a static buffer. */
char * edmac_format_size(struct edmac_info * info);

#endif
