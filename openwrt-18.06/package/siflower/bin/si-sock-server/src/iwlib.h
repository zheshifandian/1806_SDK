/*
 *	Wireless Tools
 *
 *		Jean II - HPLB 97->99 - HPL 99->07
 *
 * Common header for the Wireless Extension library...
 *
 * This file is released under the GPL license.
 *     Copyright (c) 1997-2007 Jean Tourrilhes <jt@hpl.hp.com>
 */

#ifndef IWLIB_H
#define IWLIB_H

/*#include "CHANGELOG.h"*/

/***************************** INCLUDES *****************************/

/* Standard headers */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>		/* gethostbyname, getnetbyname */
#include <net/ethernet.h>	/* struct ether_addr */
#include <sys/time.h>		/* struct timeval */
#include <unistd.h>

/* This is our header selection. Try to hide the mess and the misery :-(
 * Don't look, you would go blind ;-)
 * Note : compatibility with *old* distributions has been removed,
 * you will need Glibc 2.2 and older to compile (which means
 * Mandrake 8.0, Debian 2.3, RH 7.1 or older).
 */

/* Set of headers proposed by Dr. Michael Rietz <rietz@mail.amps.de>, 27.3.2 */
#include <net/if_arp.h>		/* For ARPHRD_ETHER */
#include <sys/socket.h>		/* For AF_INET & struct sockaddr */
#include <netinet/in.h>         /* For struct sockaddr_in */
#include <netinet/if_ether.h>

/* Fixup to be able to include kernel includes in userspace.
 * Basically, kill the sparse annotations... Jean II */
#ifndef __user
#define __user
#endif

#include <linux/types.h>		/* for "caddr_t" et al		*/

/* Glibc systems headers are supposedly less problematic than kernel ones */
#include <sys/socket.h>			/* for "struct sockaddr" et al	*/
#include <net/if.h>			/* for IFNAMSIZ and co... */

/* Private copy of Wireless extensions (in this directoty) */
#include "wireless.h"

/* Make gcc understant that when we say inline, we mean it.
 * I really hate when the compiler is trying to be more clever than me,
 * because in this case gcc is not able to figure out functions with a
 * single call site, so not only I have to tag those functions inline
 * by hand, but then it refuse to inline them properly.
 * Total saving for iwevent : 150B = 0.7%.
 * Fortunately, in gcc 3.4, they now automatically inline static functions
 * with a single call site. Hurrah !
 * Jean II */
#undef IW_GCC_HAS_BROKEN_INLINE
#if __GNUC__ == 3
#if __GNUC_MINOR__ >= 1 && __GNUC_MINOR__ < 4
#define IW_GCC_HAS_BROKEN_INLINE	1
#endif	/* __GNUC_MINOR__ */
#endif	/* __GNUC__ */
/* However, gcc 4.0 has introduce a new "feature", when compiling with
 * '-Os', it does not want to inline iw_ether_cmp() and friends.
 * So, we need to fix inline again !
 * Jean II */
#if __GNUC__ == 4
#define IW_GCC_HAS_BROKEN_INLINE	1
#endif	/* __GNUC__ */
/* Now, really fix the inline */
#ifdef IW_GCC_HAS_BROKEN_INLINE
#ifdef inline
#undef inline
#endif	/* inline */
#define inline		inline		__attribute__((always_inline))
#endif	/* IW_GCC_HAS_BROKEN_INLINE */

#ifdef __cplusplus
extern "C" {
#endif

/****************************** DEBUG ******************************/

//#define DEBUG 1

/************************ CONSTANTS & MACROS ************************/

/* Maximum forward compatibility built in this version of WT */
#define WE_MAX_VERSION	22

/****************************** TYPES ******************************/

/* Shortcuts */
typedef struct iw_range		iwrange;
typedef struct sockaddr		sockaddr;
/* ---------------------- SOCKET SUBROUTINES -----------------------*/
int
	iw_sockets_open(void);
/* --------------------- WIRELESS SUBROUTINES ----------------------*/

int
	iw_get_range_info(int		skfd,
			  const char *	ifname,
			  iwrange *	range);
/************************* INLINE FUNTIONS *************************/
/*
 * Functions that are so simple that it's more efficient inlining them
 */

/*
 * Note : I've defined wrapper for the ioctl request so that
 * it will be easier to migrate to other kernel API if needed
 */

/*------------------------------------------------------------------*/
/*
 * Wrapper to extract some Wireless Parameter out of the driver
 */
static inline int
iw_get_ext(int			skfd,		/* Socket to the kernel */
	   const char *		ifname,		/* Device name */
	   int			request,	/* WE ID */
	   struct iwreq *	pwrq)		/* Fixed part of the request */
{
  /* Set device name */
  strncpy(pwrq->ifr_name, ifname, IFNAMSIZ);
  /* Do the request */
  return(ioctl(skfd, request, pwrq));
}

/*------------------------------------------------------------------*/
/*
 * Close the socket used for ioctl.
 */
static inline void
iw_sockets_close(int	skfd)
{
  close(skfd);
}

#ifdef __cplusplus
}
#endif

#endif	/* IWLIB_H */
