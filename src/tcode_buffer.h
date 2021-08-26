#ifndef __tcode_buffer_h
#define __tcode_buffer_h

#ifdef __cplusplus
extern "C" {
#endif

#include "tcode.h"

typedef void (tcode_buffer_iterator_t)(size_t idx, tcode_command_t *command);

typedef struct tcode_buffer_node {
    tcode_command_t command;
    struct tcode_buffer_node* next;
    struct tcode_buffer_node* prev;
} tcode_buffer_node_t;

typedef struct tcode_buffer {
    tcode_buffer_node_t* first;
    tcode_buffer_node_t* last;
} tcode_buffer_t;

#define TCODE_BUFFER_DEFAULT { \
    .first = NULL, \
    .last = NULL, \
}

tcode_command_t tcode_buffer_enqueue(tcode_buffer_t *buffer, tcode_command_t *command);
void tcode_buffer_foreach(tcode_buffer_t* buffer, tcode_buffer_iterator_t fn);
void tcode_buffer_copy(tcode_buffer_t* target, tcode_buffer_t *src);
void tcode_buffer_empty(tcode_buffer_t* buffer);
void tcode_buffer_remove_idx(tcode_buffer_t* buffer, size_t idx);

#ifdef __cplusplus
}
#endif

#endif