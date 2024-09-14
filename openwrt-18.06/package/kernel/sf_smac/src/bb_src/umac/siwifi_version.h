#ifndef _SIWIFI_VERSION_H_
#define _SIWIFI_VERSION_H_

#include "siwifi_version_gen.h"

static inline void siwifi_print_version(void)
{
    printk(SIWIFI_VERS_BANNER"\n");
}

#endif /* _SIWIFI_VERSION_H_ */
