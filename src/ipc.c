#include "ipc.h"

list_s  mpfile_list;

void mpfile_init(void) {

    list_init(&mpfile_list);
    return;
}

void mpfile_reset(void) {

    if(mpfile_list.count > IPC_MPFILE_MAX) {

        mpfile_s *tmp = NULL;

        while(mpfile_list.count > 0) {

            tmp = (mpfile_s *) mpfile_list.first;
            list_remove(&tmp->list_item);
            kfree(tmp);
        }
        list_init(&mpfile_list);
    }
    return;
}

void mpfile_set(u8int *name, u32int *vaddr) {

    mpfile_s *mpf1 = kmalloc(sizeof(mpfile_s));
    mpfile_s *mpf  = kmalloc(sizeof(mpfile_s));
    kfree(mpf1);

    if(!mpf) {
        wprint("ERR: mpf addr(%x)\n", mpf);
        return;
    }
    memset(mpf, 0, sizeof(mpfile_s));
    process_s *proc = get_curr_proc();
    physaddr *paddr = virt2phys(proc->page_dir, vaddr);

    //wprint("pcurr(%x)| pkern(%x)\n cname(%s), kername(%s)", proc, p, proc->name, get_kern_proc()->name);
    mpf->pid_src = proc->pid;
    mpf->paddr = paddr;
    strcpy(mpf->name, name);
    wprint("pid_src(%d)| vaddr(%x)/paddr(%x)| vmpfad(%x)/pmpfad(%x)\n", mpf->pid_src, vaddr, mpf->paddr, mpf, virt2phys(proc->page_dir, mpf));

    list_add(&mpfile_list, &mpf->list_item);

    return;
}

u8int mpfile_get(u8int *name, u32int *vaddr) {

    wprint("in mp get...");

    u32int i;
    u8int err;
    mpfile_s *mpf = (mpfile_s *)mpfile_list.first;
    process_s *proc = get_curr_proc();

    if(!mpf) {
        wprint("mpf == 0!\n");
        return 1;
    }
    for(i = 0; i < mpfile_list.count; i++) {

        wprint("i=%d, name(%s) ", i, mpf->name);
        if(!(strcmp(name, mpf->name))) {
            wprint("FIND!!\n");
            break;
        }
        mpf = (mpfile_s *)mpf->list_item.next;
    }
    if(i == mpfile_list.count) {
        wprint("i==count\n");
        return 2;
    }
    else {
        wprint("err=%d| paddr(%x)/(%x)| vaddr(%x)/paddr(%x)\n", err, virt2phys(proc->page_dir, mpf), mpf->paddr, vaddr, virt2phys(proc->page_dir, vaddr));
       
        err = phys2virt(proc->page_dir, vaddr, (physaddr)mpf->paddr, 1, 0x07);
        wprint("err=%d| paddr(%x)/(%x)| vaddr(%x)/paddr(%x)\n", err, virt2phys(proc->page_dir, mpf), mpf->paddr, vaddr, virt2phys(proc->page_dir, vaddr));
        if(!err) {
            return 3;
        }
    }
    wprint("end mpf_get\n");
    return 0;
}

u8int mpfile_remove(mpfile_s *vaddr) {

    u32int i;
    process_s *proc = get_curr_proc();
    mpfile_s *mpf = (mpfile_s *)mpfile_list.first;
    physaddr *paddr = virt2phys(proc->page_dir, vaddr);

    for(i = 0; i < mpfile_list.count; i++) {

        if(paddr == mpf->paddr) {
            break;
        }
        mpf = (mpfile_s *)mpf->list_item.next;
    }
    if(i == mpfile_list.count) {
        return 2;
    }
    else {
        list_remove(&mpf->list_item);
        kfree(mpf);
    }
    return 0;
}