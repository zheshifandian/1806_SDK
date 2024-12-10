#ifndef __mtd_h
#define __mtd_h

#include <stdbool.h>

typedef enum _MTD_CMD{
	CMD_READ,
	CMD_WRITE
}MTD_CMD;

int mtd_operation(int cmd, char* data, int len, size_t offset);

#endif /* __mtd_h */
