#ifndef LINKED_OBJECT_DEP_H
#define LINKED_OBJECT_DEP_H

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
    long size_of_object;
    
public:

    object_list ();
    void *add_object ();
    long get_list_size ();
    int remove_object (const void *ob);
    int free_list ();
    void *get_object ();
    void list_rewind ();
    void set_object_size (long s);
    long get_object_size ();
};

#endif /* linked.h */