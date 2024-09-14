#ifndef __OSAL_MATH_H__
#define __OSAL_MATH_H__
/* TODO 32bits check */
#if defined(__KERNEL__)
#include<asm/div64.h>
#endif
/*
 * Include Files
 */

/*
 * Symbol Definition
 */

/*
 * Macro Definition
 */
#if defined(__KERNEL__)
/* b must be uint32_t */
#define do_div64(a, b) do_div((a), (b))
#else
#define do_div64(a, b) (a) = ((a) / (b))
#endif

/*
 * Data Type Declaration
 */


/*
 * Function Declaration
 */

#endif
