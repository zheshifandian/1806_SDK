/* (C) 2007 by folkert@vanheusden.com
 * The GPL (GNU public license) applies to this sourcecode.
 */
void * mymalloc(int size, char *what);
void * myrealloc(void *oldp, int newsize, char *what);
void myfree(void *p);
char * mystrdup(char *in, char *what);
