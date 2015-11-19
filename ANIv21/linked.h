#ifndef LINKED_OBJECT_DEP_H
#define LINKED_OBJECT_DEP_H

#include <new>
#include <string.h>
#include <stdint.h>

using namespace std;

struct object
{
    void *p;
    struct object *next;
    struct object *prev;
};

class object_list
{
    struct object *first;
    struct object *current;
    struct object *last;
    uint32_t size_of_object;
    
public:

    object_list ();
    void *add_object ();
    uint32_t get_list_size ();
    int remove_object (const void *ob);
    int free_list ();
    void *get_object ();
    void list_rewind ();
    void set_object_size (uint32_t s);
    uint32_t get_object_size ();
};

#endif /* linked.h */