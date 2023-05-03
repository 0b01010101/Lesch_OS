#include "vfs.h"
#include "izo.h"
#include "windows.h"

vfs_node_s *vfs_root;
gtree_s *vfs_tree;

u32int vfs_read(vfs_node_s *node, u32int offset, u32int size, u8int *buff) {

    if(node && node->read) {

        u32int ret = node->read(node, offset, size, buff);
        return ret;
    }
    return -1;
}

u32int vfs_write(vfs_node_s *node, u32int offset, u32int size, u8int *buff) {

    if(node && node->write) {

        u32int ret = node->write(node, offset, size, buff);
        return ret;
    }
    return -1;
}

__attribute__ ((target ("no-sse")))
void vfs_open(vfs_node_s *node, u32int flags) {

    if(!node) {
        return;
    }
    if(node->refcount >= 0) {
        node->refcount++;
    }
    node->open(node, flags);
    return;
}

void vfs_close(vfs_node_s *node) {

    if(!node || node == vfs_root || node->refcount == -1) {
        return;
    }
    node->refcount--;

    if(node->refcount == 0) {
        node->close(node);
    }
    return;
}

void vfs_chmod(vfs_node_s *node, u32int mode) {

    if(node && node->chmod) {
        node->chmod(node, mode);
    }
    return;
}

s32int vfs_ioctl(vfs_node_s *node, s32int req, void *argp) {

    if(node && node->ioctl) {
        return node->ioctl(node, req, argp);
    }
    return -1;
}

vfs_node_s *vfs_finddir(vfs_node_s *node, u8int *name) {

    if(node && (node->flags & FS_DIRECTORY) && node->finddir) {
        return node->finddir(node, name);
    }
    return NULL;
}

vfs_node_s *get_mountpoint_recur(u8int **path, gtreenode_s *subroot) {

//------------------------------------
  //  monitor_str_write("In get_mountpoint_recur(): ");
  //  monitor_str_write(*path);
   // monitor_char_put('\n');
//------------------------------------

    u32int found = 0;
    u8int *curr_token = strsep(path, "/");

    if(curr_token == NULL || !strcmp(curr_token, "")) {

        vfs_entry_s *ent = (vfs_entry_s *)subroot->value;
//--------------------------------------------------------------
     /*   monitor_str_write("take ent->name: ");
        monitor_str_write(ent->name);
        monitor_str_write(" | file name: ");
        monitor_str_write(ent->file->name);
       monitor_char_put('\n');
*///---------------------------------------------------------------
        return ent->file;
    }

    if(subroot->children->first == NULL) {
        goto skip;
    }
    list_item_s *child = subroot->children->first;

    do{
    //foreach(child, subroot->children) {

        gtreenode_s *tchild = (gtreenode_s *)child->data;
        vfs_entry_s *ent = (vfs_entry_s *)tchild->value;
//-----------------------------------------------------------------
 /*       monitor_str_write(ent->name);
        monitor_char_put('\n');

        monitor_str_write("first = ");
        write_hex(subroot->children->first);
        monitor_str_write("| child = ");
        write_hex(child);
        monitor_str_write(" | child->next = ");
        write_hex(child->next);
        monitor_str_write(" +\n");
*///-----------------------------------------------------------------
        if(strcmp(ent->name, curr_token) == 0) {
//----------------------------------------------------
       // monitor_str_write("found: ");
       // monitor_str_write(ent->name);
       // monitor_char_put('\n');
//----------------------------------------------------
            found = 1;
            subroot = tchild;
            break;
        }
        child = child->next;

    } while(child != subroot->children->first);

skip:
    if(!found) {
        *path = curr_token;
//---------------------------------------------------------
      //  monitor_str_write("!found->return token: ");
      //  monitor_str_write(*path);
      //  monitor_char_put('\n');
//---------------------------------------------------------
        return ((vfs_entry_s *)(subroot->value))->file;
    }
    // Recursion
    return get_mountpoint_recur(path, subroot);
}

vfs_node_s *get_mountpoint(u8int **path) {

    if(strlen(*path) > 1 && (*path)[strlen(*path) - 1] == '/') {
        *(path)[strlen(*path) - 1] = '\0';
    }
    if(! *path || *(path)[0] != '/') {
        return NULL;
    }
    if(strlen(*path) == 1) {

        *path = '\0';
        vfs_entry_s *ent = (vfs_entry_s *)vfs_tree->root->value;
        return ent->file;
    }

    (*path)++;  
//----------------------------------------------------------------
 /*       vfs_entry_s *entr = (vfs_entry_s *)vfs_tree->root->value;
        monitor_str_write("vfs_tree name: ");
        monitor_str_write(entr->name);
        monitor_char_put('\n');

        list_item_s *child = vfs_tree->root->children->first;
        gtreenode_s *tchild = (gtreenode_s *)child->data;
        entr = tchild->value;
        monitor_str_write("child1 name: ");
        monitor_str_write(entr->name);
        monitor_char_put('\n');

        child = child->next;
        tchild = child->data;
        entr = tchild->value;
        monitor_str_write("child2 name: ");
        monitor_str_write(entr->name);
        monitor_char_put('\n');

        child = child->next;
        tchild = child->data;
        entr = tchild->value;
        monitor_str_write("child3 name: ");
        monitor_str_write(entr->name);
        monitor_char_put('\n');

        child = child->next;
        tchild = child->data;
        entr = tchild->value;
        monitor_str_write("child4 name: ");
        monitor_str_write(entr->name);
        monitor_char_put('\n');
        */
//-----------------------------------------------------------------
    return get_mountpoint_recur(path, vfs_tree->root);
}

vfs_node_s *file_open(u8int *file_name, u32int flags) {

  //  monitor_str_write("file_open(): ");
  //  monitor_str_write(file_name);
  //  monitor_str_write("\n");

    u8int *filename = strdup(file_name);
    u8int *save = strdup(filename);
    u8int *original_filename = filename;
    u8int *free_filename = filename;
    u8int *new_start = NULL;
    u8int *curr_token = NULL;
    s32int i = 0;

    vfs_node_s *nextnode = NULL;
    vfs_node_s **files_del = kcalloc(1024, 1);
    
    vfs_node_s *startpoint = get_mountpoint(&filename);

    if(!startpoint) {
        return NULL;
    }
    if(filename) {
        new_start = strstr(save + (filename - original_filename), filename);
    }

    while(filename != NULL && ((curr_token = strsep(&new_start, "/")) != NULL)) {
        nextnode = vfs_finddir(startpoint, curr_token);

        if(!nextnode || (i >= 1024 / 4)) {

            goto clean;
        }
        startpoint = nextnode;
        files_del[i] = nextnode;
        i++;
    }

    if(!nextnode) {
        nextnode = startpoint;
//----------------------------------------------
       // monitor_str_write("havent nextnode\n");
//----------------------------------------------
    }
    //vfs_open(nextnode, flags);
    i--;
    while(i >= 0) {
        i--;
        kfree(files_del[i]);
    }

    kfree(save);
    kfree(free_filename);
    kfree(files_del);

    return nextnode;

    clean:
        while(i >= 0) {
            i--;
            kfree(files_del[i]);
        }

        kfree(files_del);
        return NULL;
}

//=========================================================================

gtree_s *tree_create(void) {

    return (gtree_s *)kcalloc(sizeof(gtree_s), 1);
}

gtreenode_s *treenode_create(void *value) {

    gtreenode_s *ret = kcalloc(sizeof(gtreenode_s), 1);
    ret->children = list_create();
    ret->value = value;

    return ret;
}

gtreenode_s *tree_insert(gtree_s *tree, gtreenode_s *subroot, void *value) {

    gtreenode_s *treenode = kcalloc(sizeof(gtreenode_s), 1);
    treenode->children = list_create();
    treenode->value = value;

    if(!tree->root) {

        tree->root = treenode;
        //list_insert_front(treenode->children, treenode);
        return treenode;
    }
    list_insert_front(subroot->children, treenode);
    return treenode;
}

void tree2array(gtreenode_s *subroot, void **array, u32int *size) {

    if(!subroot) {
        return;
    }

    void *val = (void *)subroot->value;
    array[*size] = val;
    *size += 1;

    if(subroot->children->first == NULL) {
        goto nxt;
    }
    list_item_s *child = subroot->children->first;
    do {
        tree2array(child->data, array, size);
        child = child->next;

    } while(child != subroot->children->first);
nxt:
    return;
}

void vfs_init(void) {

    vfs_entry_s *root = kmalloc(sizeof(vfs_entry_s));
    root->name = strdup("root");
    root->file = NULL;

    vfs_tree = tree_create();
    tree_insert(vfs_tree, NULL, root);

    return;
}

//================================================================================

void vfs_test_create(u8int *path, u8int *filename) {

    vfs_node_s *node = kcalloc(sizeof(vfs_node_s), 1);
    memcpy(node->name, filename, strlen(filename));
    //monitor_str_write("vfs_test_create()\n");
    vfs_mount(path, node);

    monitor_str_write("mount_OK ");
    return;
}

void vfs_test(void) {

    u8int *file1 = "one";
    u8int *file2 = "two";
    u8int *file3 = "three";
    u8int *file4 = "foure";
    u8int *file5 = "five";

    u8int *path1 = "/drivers/mobile";
    u8int *path2 = "/pages/book";
    u8int *path3 = "/main/problems";
    u8int *path4 = "/home/user1";
    u8int *path5 = "/home/user2";

    u8int *path_op1 = strdup(path1);
    u8int *path_op2 = strdup(path2);
    u8int *path_op3 = strdup(path3);
    u8int *path_op4 = strdup(path4);
    u8int *path_op5 = strdup(path5);

    monitor_clear();

    vfs_test_create(path1, file1);
    monitor_str_write("| ");
    vfs_test_create(path2, file2);
    monitor_str_write("| ");
    vfs_test_create(path3, file3);
    monitor_str_write("| ");
    vfs_test_create(path4, file4);
    monitor_str_write("| ");
    vfs_test_create(path5, file5);
    monitor_char_put('\n');

    vfs_node_s *node1 = file_open(path_op1, 0);
    vfs_node_s *node2 = file_open(path_op2, 0);
    vfs_node_s *node3 = file_open(path_op3, 0);
    vfs_node_s *node4 = file_open(path_op4, 0);
    vfs_node_s *node5 = file_open(path_op5, 0);

    vfs_tree_print(vfs_tree->root, 0);

    return;
}

void vfs_tree_print(gtreenode_s *node, u32int offset) {

    u32int len = 0;
    u8int *tmp = kcalloc(512, 1);

    if(!node) {
        return;
    }

    for(u32int i = 0; i < offset; ++i) {
        strcat(tmp, " ");
    }
    u8int *curr = tmp + strlen(tmp);
    vfs_entry_s *entr = (vfs_entry_s *)node->value;

    if(entr->file) {
        monitor_str_write(curr);
        monitor_str_write(entr->name);
        monitor_char_put('(');
        write_hex((u32int)entr->file);
        monitor_str_write(", ");
        monitor_str_write(entr->file->name);
        monitor_char_put(')');

    } else {
        monitor_str_write(curr);
        monitor_str_write(entr->name);
        monitor_str_write("(empty)");
    }
    monitor_str_write(tmp);
    monitor_char_put('\n');

    len = strlen(entr->name);
    kfree(tmp);

    if(node->children->first == NULL) {
        goto ex_jmp;
    }
    list_item_s *child = node->children->first;
    do {

        vfs_tree_print(child->data, offset + len + 1);
        child = child->next;

    } while(child != node->children->first);

ex_jmp:
    return;
}

void vfs_mount(u8int *path, vfs_node_s *fs) {

    //monitor_str_write("in vfs_mount\n");

    fs->refcount = -1;
    fs->fs_type = 0;

    if(path[0] == '/' && strlen(path) == 1) {

        vfs_entry_s *entr = (vfs_entry_s *)vfs_tree->root->value;

        if(entr->file) {
            monitor_str_write("ERROR(vfs_mount()): path is already mounted\n");
            return;
        }

        vfs_root = fs;
        entr->file = fs;
        return;
    }
//------------------------------------
    //monitor_str_write("in vfs_mount(): ");
    //monitor_str_write(path + 1);
    //monitor_char_put('\n');
//-------------------------------------
    vfs_mount_recur(path + 1, vfs_tree->root, fs);
}

void vfs_mount_recur(u8int *path, gtreenode_s *subroot, vfs_node_s *fs) {

    u32int found = 0;
    u8int *curr_token = strsep(&path, "/");

   //monitor_str_write("in vfs_mount_recur\n");

    if(curr_token == NULL || !strcmp(curr_token, "")) {

       // monitor_str_write("mount point\n");
        //mount point
        vfs_entry_s *entr = (vfs_entry_s *)subroot->value;

        if(entr->file) {
            monitor_str_write("ERROR(vfs_mount_recur()): path is already mounted");
            return;
        }
        if(!strcmp(entr->name, "/")) {
            vfs_root = fs;
        }

        entr->file = fs;
        return;
    }

    if(subroot->children->first == NULL) {
            goto miss;
        }

    list_item_s *child = subroot->children->first;
    do{
    //foreach(child, subroot->children) {
//--------------------------------------
    /*    monitor_str_write("first = ");
        write_hex(subroot->children->first);
        monitor_str_write("| child = ");
        write_hex(child);
        monitor_str_write(" | child->next = ");
        write_hex(child->next);
        monitor_str_write(" +\n");
*///--------------------------------------        
        gtreenode_s *tchild = (gtreenode_s *)child->data;
        vfs_entry_s *entr = (vfs_entry_s *)tchild->value;
//--------------------------------------------
    /*    monitor_str_write("entr->name:");
        monitor_str_write(entr->name);
        monitor_char_put('\n');
*///-------------------------------------------------
        if(strcmp(entr->name, curr_token) == 0) {
            found = 1;
            subroot = tchild;

           // monitor_str_write("found\n");
            break;
        }
        child = child->next;

    } while(child != subroot->children->first);

miss:
    if(!found) {
//-------------------------------------
       // monitor_str_write("!found\n");
//--------------------------------------
        vfs_entry_s *entr = kcalloc(sizeof(vfs_entry_s), 1);
        entr->name = strdup(curr_token);
//--------------------------------------------------
       // monitor_str_write(entr->name);
       // monitor_char_put('\n');
//----------------------------------------------------
        subroot = tree_insert(vfs_tree, subroot, entr);
    }
    vfs_mount_recur(path, subroot, fs);

    return;
}

void vfs_mount_dev(u8int *path, vfs_node_s *node) {

    vfs_mount(path, node);
    return;
}

s32int vfs_mkdir(u8int *name, u16int permission) {

    u32int i = strlen(name);
    u8int *dir_name = strdup(name);
    u8int *s_dirname = dir_name;
    u8int *path_par = "/";

    while(i >= 0) {

        if(dir_name[i] == '/') {

            if(i != 0) {
                dir_name[i] = '\0';
                path_par = dir_name;
            }
            dir_name = &dir_name[i+1];
            break;
        }
        i--;
    }
    vfs_node_s *parent_node = file_open(path_par, 0);

    if(!parent_node){

        kfree(s_dirname);
        //vfs_close(parent_node);
        return -1;
    }

    if(parent_node->mkdir) {
        parent_node->mkdir(parent_node, dir_name, permission);
    }
    kfree(s_dirname);
    vfs_close(parent_node);

    return 0;
}

s32int vfs_mkfile(u8int *name, u16int permission) {

    u32int i = strlen(name);
    char *dir_name = strdup(name);
    char *s_dirname = dir_name;
    char *path_par = "/";

    while(i >= 0) {

        if(dir_name[i] == '/') {

            if(i != 0) {
                dir_name[i] = '\0';
                path_par = dir_name;
            }
            dir_name = &dir_name[i+1];
            break;
        }
        i--;
    }
    vfs_node_s *parent_node = file_open(path_par, 0);

    if(!parent_node) {

        kfree(s_dirname);
        //vfs_close(parent_node);
        return -1;
    }

    if(parent_node->create) {
        parent_node->create(parent_node, dir_name, permission);
    }
    kfree(s_dirname);
    vfs_close(parent_node);

    return 0;
}

s32int vfs_unlink(u8int *name) {

    u32int i = strlen(name);
    u8int *dir_name = strdup(name);
    u8int *s_dirname = dir_name;
    u8int *path_par = "/";

    while(i >= 0) {

        if(dir_name[i] == '/') {

            if(i != 0) {
                dir_name[i] = '\0';
                path_par = dir_name;
            }
            dir_name = &dir_name[i+1];
            break;
        }
        i--;
    }
    vfs_node_s *parent_node = file_open(path_par, 0);

    if(!parent_node) {
        kfree(s_dirname);
        //vfs_close(parent_node);
        return -1;
    }

    if(parent_node->unlink) {
        parent_node->unlink(parent_node, dir_name);
    }
    kfree(s_dirname);
    vfs_close(parent_node);

    return 0;
}

u32int vfs_get_file_size(vfs_node_s *file) {

    if(file && file->get_file_size) {
        return file->get_file_size(file);
    }
    return 0;
}

u8int vfs_listdir(u8int *name) {

    vfs_node_s *file = file_open(name, 0);

    if(!file) {
        return 0;
    }
    if(!file->listdir) {
        return 0;
    }
    u8int **nams = file->listdir(file);
    u8int **save = nams;

    while(*nams) {
        monitor_str_write(*nams);
        monitor_char_put('+');
        kfree(*nams);
        nams++;
    }
    kfree(save);
    monitor_char_put('\n');
    
    return 1;
}
