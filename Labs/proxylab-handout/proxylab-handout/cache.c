#include <stddef.h>
#include "csapp.h"
#include "cache.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
static int find_empty(void);

typedef struct {
    char uri[MAXLINE];
    char *data;
    size_t size; 
    int time; 

} cache_block_t; 

static cache_block_t cache[MAX_OBJECTS];
static size_t object_cnt; 
static int tick;

static int readcnt;
static sem_t mutex, w;

void cache_init(void) {
    readcnt = 0;
    sem_init(&mutex, 0, 1);
    sem_init(&w, 0, 1);
}

int  cache_find(char *uri, char *buf_out, size_t *size_out) {
    P(&mutex);
    readcnt++;
    if (readcnt == 1) {
        P(&w);
    }
    V(&mutex);

    // critical section
    
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (cache[i].data == NULL) continue;
        if (!strcmp(cache[i].uri, uri)) {
            P(&mutex);
            tick++;
            cache[i].time = tick;  // when find, set MRU 
            V(&mutex);
            memcpy(buf_out, cache[i].data, cache[i].size);
            *size_out = cache[i].size;
            
            P(&mutex);
            readcnt--;
            if (readcnt == 0) {
                V(&w);
            }
            V(&mutex);

            return 0;
        }
    }
    P(&mutex);
    readcnt--;
    if (readcnt == 0) {
        V(&w);
    }
    V(&mutex);
    return -1; 
}

void cache_insert(char *uri, char *buf, size_t size) {
    P(&w);
    tick++;
    if (object_cnt >= MAX_OBJECTS) {
        cache_evict();
    }
    int i = find_empty();
    if (i == -1) {
        V(&w);
        return; 
    }
    cache[i].data = malloc(size);
    memcpy(cache[i].data, buf, size);

    cache[i].size = size; 
    cache[i].time = tick; 
    strcpy(cache[i].uri, uri);
    object_cnt++;
    V(&w);

}

void cache_evict(void) {
    // find the least time
    int min_tick = cache[0].time; 
    int min_index = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (cache[i].time < min_tick) {
            min_tick = cache[i].time; 
            min_index = i; 
        }
    }
    
    cache[min_index].size = 0; 
    free(cache[min_index].data); 
    cache[min_index].data = NULL;
    cache[min_index].uri[0] = '\0';
    cache[min_index].time = 0;
    object_cnt--;
}

static int find_empty(void) {
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (cache[i].data == NULL) {
            return i; 
        }
    }
    return -1;
}

