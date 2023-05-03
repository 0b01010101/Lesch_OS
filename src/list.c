
#include "list.h"

void list_init(list_s *list) {

    list->first = NULL;
    list->count = 0;

    return;
}

list_s *list_create(void) {
    
    list_s *list = (list_s *)kmalloc(sizeof(list_s));
    list_init(list);

    return list;
}

list_item_s *list_insert_front(list_s *list, void *data) {

    if(!list) {
        return 0;
    }

    get_mtx(&list->mtx, true);
    list_item_s *ret = (list_item_s *)kcalloc(sizeof(list_item_s), 1);

    if(!list->first) {

        ret->next = ret;
        ret->prev = ret;
    }
    else {

        list->first->prev->next = ret;          //last block;
        ret->prev = list->first->prev;          //new block;
        ret->next = list->first;                //new block;
        list->first->prev = ret;                //first block;
    }

    ret->data = data;
    list->first = ret;
    list->count++;
    clean_mtx(&list->mtx);

    return ret;
}

list_item_s *list_insert_back(list_s *list, void *data) {

    get_mtx(&list->mtx, true);
    
    list_item_s *ret = (list_item_s *)kcalloc(sizeof(list_item_s), 1);
    list_item_s *first = list->first;

    if(!list->first) {

        ret->next = ret;
        ret->prev = ret;
    }
    else {
        first->prev->next = ret;
        ret->prev = first->prev;
        ret->next = first;
        first->prev = ret;
    }

    ret->data = data;
    list->count++;
    clean_mtx(&list->mtx);

    return ret;
}

void list_add(list_s *list, list_item_s *item) {

    if(item->list == NULL) {

        get_mtx(&list->mtx, true);       //{
        if(list->first) {

            item->list = list;
            item->next = list->first;
            item->prev = list->first->prev;

            item->prev->next = item;
            item->next->prev = item;
        }
        else {
            list->first = item;
            item->list = list;
            item->next = item;
            item->prev = item;
        }
        list->count++;
        clean_mtx(&list->mtx);           //}
    }
    return;
}

void list_remove(list_item_s *item) {

    get_mtx(&item->list->mtx, true);    //{

    if(item->list->first == item) {

        item->list->first = item->next;

        if(item->list->first == item) {

            item->list->first = NULL;
        }
    }
    item->prev->next = item->next;
    item->next->prev = item->prev;

    item->list->count--;
    //kfree(item);
    clean_mtx(&item->list->mtx);       //}
    return;
}

s32int list_contain(list_s *list, void *data) {

    s32int idx = 0;

    if(list->first == NULL) {
        goto ext;
    }
    list_item_s *step = list->first;
    
    do {
        if(step->data == data) {
            return idx;
        }
        idx++;
        step = step->next;

    } while(step != list->first);

ext:
    return -1;
}

u32int list_size(list_s *list) {

    if(!list) {
        return 0;
    }
    return list->count;
}
