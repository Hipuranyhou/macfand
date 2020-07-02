/**
 * macfand - hipuranyhou - 21.06.2020
 * 
 * Daemon for controlling fans on Linux systems using
 * applesmc and coretemp.
 * 
 * https://github.com/Hipuranyhou/macfand
 */

#include <string.h>

#include "linked.h"


int list_push_front(t_node **head, const void *data, const size_t data_size) {
    t_node *new_node = (t_node*)malloc(sizeof(*new_node));
    if (!new_node)
        return 0;
    
    new_node->next = *head;
    new_node->data = malloc(data_size);

    if (!new_node->data && data_size > 0) {
        free(new_node);
        return 0;
    }

    memcpy(new_node->data, data, data_size);
    *head = new_node;
    return 1;
}


void list_free(t_node *head, void (*node_free)(void *)) {
    t_node *next = NULL;
    while (head) {
        next = head->next;
        node_free(head->data);
        free(head);
        head = next;
    }
}


void list_print(const t_node *head, FILE *stream, void (*node_print)(const void *, FILE *)) {
    while (head) {
        node_print(head->data, stream);
        fprintf(stream, "\n");
        head = head->next;
    }
}