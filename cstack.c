#include "cstack.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

typedef struct StackNode
{
    struct StackNode* prev_node_p;
        // Pointer to the previous node in stack
    unsigned int data_length;
    unsigned char data[];
        // Implemented as a "flexible array member" aka "struct hack",
        // can be implemented as a pointer to outer char array as well
} st_node_t;

typedef struct StackEntry
{
    st_node_t* top_node_p;
        // Pointer to the top (last) node in stack
    unsigned int size;
    bool is_used;
} st_entry_t;

typedef struct StackList
{
    st_entry_t* items_p;
        // Implemented as a pointer to outer st_entry_t array,
        // Because I realloc only the array itself rather than the whole struct
    int capacity;
    int items_counter;
    int first_free_handler;
} st_list_t;

st_node_t* delete_stack_node(st_node_t* node_p);

st_list_t g_stack_list = {.items_p = NULL, .capacity = 0,
                          .items_counter = 0, .first_free_handler = 0};

hstack_t stack_new(void)
{
    if (g_stack_list.items_counter == g_stack_list.capacity)
    {
        // List is full, expand the list

        int new_capacity;
        switch (g_stack_list.capacity)
        {
            case INT_MAX:
                // Cannot expand the list any longer
                return -1;
            case INT_MAX / 2:
                // Simple x2 would get us INT_MAX + 1
                new_capacity = INT_MAX;
                break;
            case 0:
                new_capacity = 4;
                break;
            default:
                new_capacity = g_stack_list.capacity * 2;
                break;
        }

        st_entry_t* new_items_p = realloc(g_stack_list.items_p,
                            sizeof(st_entry_t) * new_capacity);
        if (new_items_p == NULL)
            // Could not reallocate memory, yet old memory block is not freed
            return -1;
        g_stack_list.items_p = new_items_p;

        // Assign correct values to new StackEntry elements
        for (int i = g_stack_list.capacity; i < new_capacity; i++)
            g_stack_list.items_p[i] = (st_entry_t) {.top_node_p = NULL,
                                                    .size = 0u, .is_used = false};

        g_stack_list.first_free_handler = g_stack_list.capacity;
        g_stack_list.capacity = new_capacity;
    }
    else
    {
        // Look for first free handler
        while (g_stack_list.items_p[g_stack_list.first_free_handler].is_used)
            g_stack_list.first_free_handler++;
    }

    g_stack_list.items_p[g_stack_list.first_free_handler].is_used = true;
    g_stack_list.items_counter++;
    return g_stack_list.first_free_handler++;
}

void stack_free(const hstack_t hstack)
{
    if (stack_valid_handler(hstack))
        return;

    while (g_stack_list.items_p[hstack].top_node_p != NULL)
        g_stack_list.items_p[hstack].top_node_p = delete_stack_node(g_stack_list.
                                                                  items_p[hstack].
                                                                  top_node_p);

    g_stack_list.items_p[hstack].size = 0;
    g_stack_list.items_p[hstack].is_used = false;
    g_stack_list.items_counter--;

    if (g_stack_list.items_counter == 0)
    {
        free(g_stack_list.items_p);
        g_stack_list.items_p = NULL;
        g_stack_list.capacity = 0;
        g_stack_list.first_free_handler = 0;
    }
    else if (hstack < g_stack_list.first_free_handler)
        g_stack_list.first_free_handler = hstack;
}

int stack_valid_handler(const hstack_t hstack)
{
    // NB! 1 and 0 return codes are used in in contrast to the usual sense
    if (hstack < 0 || hstack >= g_stack_list.capacity ||
        !g_stack_list.items_p[hstack].is_used)
        return 1;
    return 0;
}

unsigned int stack_size(const hstack_t hstack)
{
    if (!stack_valid_handler(hstack))
        return g_stack_list.items_p[hstack].size;

    return 0u;
}

void stack_push(const hstack_t hstack, const void* data_in, const unsigned int size)
{
    if (stack_valid_handler(hstack) || data_in == NULL || size == 0u)
        return;

    st_node_t* new_node_p = malloc(sizeof(st_node_t) + sizeof(unsigned char) * size);
    if (new_node_p == NULL)
        // OOM, could not allocate any more memory
        return;

    new_node_p->prev_node_p = g_stack_list.items_p[hstack].top_node_p;
    new_node_p->data_length = size;
    memcpy(new_node_p->data, data_in, size);

    g_stack_list.items_p[hstack].top_node_p = new_node_p;
    g_stack_list.items_p[hstack].size++;
}

unsigned int stack_pop(const hstack_t hstack, void* data_out, const unsigned int size)
{
    if (stack_valid_handler(hstack) || g_stack_list.items_p[hstack].size == 0 ||
        data_out == NULL || size < g_stack_list.items_p[hstack].top_node_p->data_length)
        return 0u;

    unsigned int data_length = g_stack_list.items_p[hstack].
                               top_node_p->data_length;
    memcpy(data_out, g_stack_list.items_p[hstack].top_node_p->data, data_length);

    g_stack_list.items_p[hstack].top_node_p = delete_stack_node(g_stack_list.
                                                                items_p[hstack].
                                                                top_node_p);
    g_stack_list.items_p[hstack].size--;

    return data_length;
}

st_node_t* delete_stack_node(st_node_t* node_p)
{
    st_node_t* prev_node_p = node_p->prev_node_p;
    free(node_p);
    return prev_node_p;
}

