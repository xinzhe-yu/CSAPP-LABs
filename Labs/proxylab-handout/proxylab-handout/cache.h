#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_OBJECTS 10

void cache_init(void);
int  cache_find(char *uri, char *buf_out, size_t *size_out);
void cache_insert(char *uri, char *buf, size_t size);
void cache_evict(void);


#endif