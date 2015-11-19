#include "linked.h"

object_list::object_list()
{
    first = NULL;
    current = NULL;
    last = NULL;
    size_of_object = 0;
}

void object_list::set_object_size (uint32_t s)
{
    size_of_object = s;
}

uint32_t object_list::get_object_size ()
{
    return size_of_object;
}

void *object_list::add_object ()
{
    if (first == NULL)
    {
        first = new(nothrow) struct object;
        if (first == NULL) return NULL;
        first->p = new(nothrow) char[size_of_object];
        if (first->p == NULL) return NULL;
        first->prev = NULL;
        first->next = NULL;
        last = first;
        return first->p;
    }
    else
    {
        struct object *tmp = NULL;
        tmp = new(nothrow) struct object;
        if (tmp == NULL) return NULL;
        tmp->p = new(nothrow) char[size_of_object];
        if (tmp->p == NULL) return NULL;
        tmp->prev = last;
        tmp->next = NULL;
        last->next = tmp;
        last = tmp;
        return tmp->p;
    }
    return 0;
}

uint32_t object_list::get_list_size ()
{
    uint32_t s = 0;
    struct object *tmp = first;
    if (tmp == NULL) return -1;
    while (tmp != NULL)
    {
        s++;
        tmp = tmp->next;
    }
    return s;
}

int object_list::free_list ()
{
    register int count = 0;
    struct object *tmp = last;
    if (tmp == NULL) return -1;
    while (tmp != NULL)
    {
        struct object *prev = tmp->prev;
        tmp->prev = NULL;
        tmp->next = NULL;
        delete [] (char *)tmp->p;
        delete tmp;
        tmp = prev;
        count++;
    }
    last = NULL;
    first = NULL;
    return count;
}

int object_list::remove_object (const void *ob)
{
    struct object *tmp = first;
    if (tmp == NULL) return -1;
    while (tmp != NULL)
    {
        if (memcmp (tmp->p, ob, size_of_object) == 0)
        {
            struct object *next = tmp->next;
            struct object *prev = tmp->prev;
            if ((tmp == first) && (tmp == last))
            {
                first = NULL;
                last = NULL;
                tmp->next = NULL;
                tmp->prev = NULL;
                delete [] (char *)tmp->p;
                delete tmp;
                break;
            } 
            else if (tmp == first)
            {
                first = next;
                first->prev = NULL;
                tmp->next = NULL;
                tmp->prev = NULL;
                delete [] (char *)tmp->p;
                delete tmp;
                break;
            }
            else if (tmp == last)
            {
                last = prev;
                last->next = NULL;
                tmp->next = NULL;
                tmp->prev = NULL;
                delete [] (char *)tmp->p;
                delete tmp;
                break;
            }
            else
            {
                next->prev = prev;
                prev->next = next;
                tmp->next = NULL;
                tmp->prev = NULL;
                delete [] (char *) tmp->p;
                delete tmp;
                break;
            }
        }
        tmp = tmp->next;
    }
    return 0;
}

void *object_list::get_object ()
{
    if (current == NULL)
    {
        current = first;
        if (current == NULL) return NULL;
        return current->p;
    }
    else
    {
        current = current->next;
        if (current == NULL) return NULL;
        return current->p;
    }
}

void object_list::list_rewind ()
{
    current = NULL;
}