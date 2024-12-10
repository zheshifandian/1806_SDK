#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
	char *buff, c;
	size_t buff_len = 80;
	char **elem;
	size_t max_elem = 100;
	int i, j, n;
	int err = 0;

	srand(time(NULL));
	if (argc > 1) {
		i = rand() % (argc - 1);
		printf("%s", argv[i + 1]);

		return 0;
	}

	elem = calloc(max_elem, sizeof(char *));
	if (!elem)
		return -1;
	buff = calloc(1, buff_len);
	if (!buff) {
		free(elem);
		return -1;
	}
	i = j = n = 0;
	while ((c = fgetc(stdin)) != EOF) {
		if (c == '\n' || c == ' ') {
			elem[n] = buff;
			n++;
			if (!(buff = calloc(1, buff_len))) {
				err = -1;
				goto out;
			}
			j = 0;
			if (n == max_elem) {
				if (!(elem = realloc(elem, max_elem * sizeof(char *) * 2))) {
					err = -1;
					goto out;
				}
				max_elem *= 2;
			}
		} else {
			buff[j++] = c;
		}
	}
	i = rand() % n;
	printf("%s", elem[i]);

out:
	for (j = 0; j < n; j++)
		free(elem[j]);
	free(elem);
	return err;
}
