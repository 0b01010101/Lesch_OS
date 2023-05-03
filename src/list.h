
#ifndef     LIST_H
#define     LIST_H

#include    "common.h"
#include    "memory.h"

typedef struct list_str    list_s;
typedef struct list_item_str    list_item_s;

struct list_item_str {

    list_item_s   *prev;
    list_item_s   *next;
    list_s        *list;
    void          *data;
};

struct list_str {

    list_item_s    *first;
    u32int         count;
    mutex_s        mtx;
};

list_item_s *list_insert_front(list_s *list, void *data);
list_item_s *list_insert_back(list_s *list, void *data);
list_s *list_create(void);
void list_init(list_s *list);
void list_add(list_s *list, list_item_s *item);
void list_remove(list_item_s *item);
s32int list_contain(list_s *list, void *data);
u32int list_size(list_s *list);

#endif