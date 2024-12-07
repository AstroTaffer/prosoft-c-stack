#include "cstack.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(VAR) (void)(VAR)

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
    int is_used;
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

int handler_valid_range(const hstack_t hstack);
st_node_t* delete_stack_node(st_node_t* node_p);

st_list_t g_stack_list = {.items_p = NULL, .capacity = 0,
                          .items_counter = 0, .first_free_handler = 0};
    // FIXME: Where should structs and corresponding types be defined?
    //        In cstack.c (as implemented) or in cstack.h?

hstack_t stack_new(void)
{
    if (g_stack_list.items_counter == g_stack_list.capacity)
    {
        // List is full, expand the list

        int new_capacity = (g_stack_list.capacity == 0)
                                    ? 4 : (g_stack_list.capacity * 2);
            // TODO: I could check for arithmetic overflow for new_capacity
            // but it's unlikely someone would need more than ~2 billion (INT_MAX)
            // stacks at once (it takes 30 list expansions to overflow)

        st_entry_t* new_items_p = realloc(g_stack_list.items_p,
                            sizeof(st_entry_t) * new_capacity);
        if (new_items_p == NULL)
            // Could not reallocate memory, yet old memory block is not freed
            return -1;
        g_stack_list.items_p = new_items_p;

        // Assign correct values to new StackEntry elements
        for (int i = g_stack_list.capacity; i < new_capacity; i++)
            g_stack_list.items_p[i] = (st_entry_t) {.top_node_p = NULL,
                                                    .size = 0u, .is_used = 0};

        g_stack_list.first_free_handler = g_stack_list.capacity;
        g_stack_list.capacity = new_capacity;
    }
    else
    {
        // Look for first free handler
        while (g_stack_list.items_p[g_stack_list.first_free_handler].is_used)
            g_stack_list.first_free_handler++;
    }

    g_stack_list.items_p[g_stack_list.first_free_handler].is_used = 1;
    g_stack_list.items_counter++;
    return g_stack_list.first_free_handler++;
}

void stack_free(const hstack_t hstack)
{
    if (!handler_valid_range(hstack) || !g_stack_list.items_p[hstack].is_used)
        return;

    while (g_stack_list.items_p[hstack].top_node_p != NULL)
        g_stack_list.items_p[hstack].top_node_p = delete_stack_node(g_stack_list.
                                                                  items_p[hstack].
                                                                  top_node_p);

    g_stack_list.items_p[hstack].size = 0;
    g_stack_list.items_p[hstack].is_used = 0;
    g_stack_list.items_counter--;

    if (g_stack_list.items_counter == 0)
    {
        free(g_stack_list.items_p);
        g_stack_list.items_p = NULL;
        g_stack_list.capacity = 0;
        g_stack_list.first_free_handler = 0;
    }
}

int stack_valid_handler(const hstack_t hstack)
{
    if (handler_valid_range(hstack) && g_stack_list.items_p[hstack].is_used)
        return 0;

    return 1;
}

unsigned int stack_size(const hstack_t hstack)
{
    if (handler_valid_range(hstack))
        // Omitted explicit is_used check, should not cause any trouble
        return g_stack_list.items_p[hstack].size;

    return 0u;
}

void stack_push(const hstack_t hstack, const void* data_in, const unsigned int size)
{
    if (!handler_valid_range(hstack) || !g_stack_list.items_p[hstack].is_used ||
        data_in == NULL || size == 0u)
        return;

    st_node_t* new_node_p = malloc(sizeof(st_node_t) + sizeof(unsigned char) * size);
    if (new_node_p == NULL)
        // Stack overflow, could not allocate any more memory
        return;

    new_node_p->prev_node_p = g_stack_list.items_p[hstack].top_node_p;
    new_node_p->data_length = size;
    memcpy(new_node_p->data, data_in, size);

    g_stack_list.items_p[hstack].top_node_p = new_node_p;
    g_stack_list.items_p[hstack].size++;
}

unsigned int stack_pop(const hstack_t hstack, void* data_out, const unsigned int size)
{
    if (!handler_valid_range(hstack) || !g_stack_list.items_p[hstack].is_used ||
        g_stack_list.items_p[hstack].size == 0 || data_out == NULL ||
        size < g_stack_list.items_p[hstack].top_node_p->data_length)
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

int handler_valid_range(const hstack_t hstack)
{
    if (hstack < 0 || hstack >= g_stack_list.capacity)
        // Handler out of range
        return 0;
    return 1;
}

st_node_t* delete_stack_node(st_node_t* node_p)
{
    st_node_t* prev_node_p = node_p->prev_node_p;
    free(node_p);
    return prev_node_p;
}

