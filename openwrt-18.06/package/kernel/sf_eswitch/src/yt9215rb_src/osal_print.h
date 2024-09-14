#ifndef __OSAL_PRINT_H__
#define __OSAL_PRINT_H__

#if defined(__KERNEL__)
#include <linux/string.h>
#endif

/*add other space later */
#if defined(__KERNEL__)
#define osal_printf                    printk
#else
#define osal_printf                    printf
#endif
#define osal_sprintf                  sprintf
#define osal_strncmp              strncmp
#define osal_strlen                  strlen
#define osal_strcpy                  strcpy


#endif
