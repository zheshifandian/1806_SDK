#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main(int argc,char *argv[])
{
	char c[] = {
	    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c',
	    'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
	    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C',
	    'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', '-', '.',
	};
	char name[257] = {0};
	int i = 0;
	int len = 0;
	struct timeval tv;

	gettimeofday(&tv, 0);
	srand((unsigned)tv.tv_sec * 1000 + tv.tv_usec);

	//file name's length ranges from 1 to 50
redo:	while(!len)
		len = rand() % 51;

	for (i = 0; i < len; i++)
		name[i] = c[rand() % sizeof(c)];

	//"." is not a valid name
	if ((len == 1) && (name[0] == '.'))
		goto redo;

	printf("%s", name);
	return 0;
}
