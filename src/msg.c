#include "msg.h"

list_s  msg_list;
u32int  msg_count;

void msg_init(void) {

    list_init(&msg_list);
    return;
}

void msg_reset(void) {

    if(msg_list.count > MSG_MAX_CNT) {
        
        msg_s *tmp = NULL;

        while(msg_list.count > 0) {
            
            tmp = (msg_s *)msg_list.first;
            list_remove(&tmp->list_item);
            kfree(tmp);
        }
        list_init(&msg_list);
    }
    return;
}

void msg_send(msg_s *msg) {

    process_s *proc = get_curr_proc();
    msg_s *tmp = kcalloc(sizeof(msg_s), 1);
    memcpy(tmp, msg, sizeof(msg_s));

    msg->count = msg_count++;
    tmp->pid_src = proc->pid;
    list_add(&msg_list, &tmp->list_item);

    return;
}

u8int msg_recv(msg_s *msg) {

    u8int err = 0;
    u32int idx = 0;
    msg_s *tmp = (msg_s *)msg_list.first;
    process_s *proc = get_curr_proc();

    if(!tmp) {
        return 0;
    }
    do{
        if(proc->pid == tmp->pid_dst) {
            break;
        }
        tmp = (msg_s *)tmp->list_item.next;
        idx++;

    } while(idx < msg_list.count);

    if(idx == msg_list.count) {
        err = 0;
    }
    else {
        memcpy(msg, tmp, sizeof(msg_s));
        list_remove(&tmp->list_item);
        kfree(tmp);
        err = 1;
    }
    return err;
}

msg_s *get_msg(u32int pid) {

    msg_s *tmp = (msg_s *)msg_list.first;
    u32int idx = 0;

    if(!tmp) {
        return NULL;
    }
    do{
        if(tmp->pid_dst == pid) {
            break;
        }
        tmp = (msg_s *)tmp->list_item.next;
    } while(idx < msg_list.count);

    if(idx == msg_list.count) {
        return NULL;
    }
    return tmp;
}