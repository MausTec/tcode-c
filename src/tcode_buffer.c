#include "tcode_buffer.h"
#include <stdlib.h>

void tcode_buffer_remove_idx(tcode_buffer_t* buffer, size_t idx) {
    tcode_buffer_node_t* node = buffer->first;
    while (node != NULL && idx--) {
        node = node->next;
    }

    if (idx != 0 || node == NULL) {
        return;
    }

    if (buffer->first == node) {
        buffer->first = node->next;
    }

    if (buffer->last == node) {
        buffer->last = node->prev;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }

    free(node);
}

void tcode_buffer_remove_duplicate(tcode_buffer_t* buffer, tcode_command_t* command) {
    tcode_buffer_node_t* node = buffer->first;
    size_t idx = 0;
    while (node != NULL && command != NULL) {
        tcode_command_t c = node->command;
        node = node->next;

        if (c.type == command->type && c.channel == command->channel) {
            tcode_buffer_remove_idx(buffer, idx);
        }

        idx++;
    }
}

tcode_command_t tcode_buffer_enqueue(tcode_buffer_t* buffer, tcode_command_t* command) {
    tcode_buffer_remove_duplicate(buffer, command);

    tcode_buffer_node_t* node = malloc(sizeof(tcode_buffer_node_t));
    if (node == NULL) {
        printf("malloc failed!\n");
    }

    node->prev = buffer->last;
    node->next = NULL;
    node->command = *command;

    if (buffer->last != NULL) {
        buffer->last->next = node;
    }

    buffer->last = node;

    if (buffer->first == NULL) {
        buffer->first = buffer->last;
    }

    return node->command;
}

void tcode_buffer_copy(tcode_buffer_t* target, tcode_buffer_t* src) {
    tcode_buffer_node_t* node = src->first;
    while (node != NULL) {
        tcode_buffer_enqueue(target, &node->command);
        node = node->next;
    }
}

void tcode_buffer_empty(tcode_buffer_t* buffer) {
    tcode_buffer_node_t* node = buffer->first;
    while (node != NULL) {
        tcode_buffer_node_t* next = node->next;
        free(node);
        node = next;
    }
    buffer->first = NULL;
    buffer->last = NULL;
}

void tcode_buffer_foreach(tcode_buffer_t* buffer, tcode_buffer_iterator_t fn) {
    tcode_buffer_node_t* node = buffer->first;
    size_t idx = 0;
    while (node != NULL) {
        fn(idx, &node->command);
        node = node->next;
        idx++;
    }
}