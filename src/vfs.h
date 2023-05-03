#ifndef     VFS_H
#define     VFS_H

#include    "common.h"
#include    "list.h"
#include    "help_func.h"

#define     FS_FILE         0x01
#define     FS_DIRECTORY    0x02
#define     FS_CHARDEV      0x04
#define     FS_BLOCKDEV     0x08
#define     FS_PIPE         0x10
#define     FS_SYMLINK      0x20
#define     FS_MOUNTPOINT   0x40

#define     O_TRUNC         0x0400

//#define foreach(t, list) for(list_item_s *t = list->first; t != NULL; t = t->next)
#define foreach(t, list) for(list_item_s *t = ((list_s *)list)->first; t->next != list->first; t = t->next)

typedef struct vfs_node vfs_node_s;
typedef struct dirent dirent_s;
typedef struct vfs_entry vfs_entry_s;
typedef struct gtreenode gtreenode_s;
typedef struct gtree gtree_s;

typedef vfs_node_s   FILE;

typedef void (*close_cllback) (vfs_node_s *file);
typedef void (*open_cllback) (vfs_node_s *, u32int flags);
typedef void (*chmod_cllback) (vfs_node_s *, u32int mode);
typedef void (*unlink_cllback) (vfs_node_s *, u8int *name);
typedef void (*mkdir_cllback) (vfs_node_s *, u8int *name, u16int perm);
typedef void (*create_cllback) (vfs_node_s *, u8int *name, u16int perm);

typedef u32int (*get_size_cllback) (vfs_node_s *);
typedef u32int (*get_file_size_cllback) (vfs_node_s *);
typedef u32int (*ioctl_cllback) (vfs_node_s *, u32int req, void *argp);
typedef u32int (*read_cllback) (vfs_node_s *node, u32int offset, u32int size, u8int *buff);
typedef u32int (*write_cllback) (vfs_node_s *node, u32int offset, u32int size, u8int *buff);

typedef u8int **(*listdir_cllback) (vfs_node_s *);
typedef dirent_s *(*readdir_cllback) (vfs_node_s *, u32int);
typedef vfs_node_s *(*finddir_cllback) (vfs_node_s *, u8int *name);

struct vfs_node {

    u8int   name[256];
    void    *device;
    u32int  mask;
    u32int  uid;
    u32int  gid;
    u32int  flags;
    u32int  inode_num;
    u32int  size;
    u32int  fs_type;
    u32int  open_flags;

    u32int  time_create;
    u32int  time_access;
    u32int  time_modified;

    u32int  offset;
    u32int  nlink;
    u32int  refcount;
    // File operations
    read_cllback     read;
    write_cllback    write;
    readdir_cllback  readdir;
    finddir_cllback  finddir;
    create_cllback   create;
    mkdir_cllback    mkdir;
    unlink_cllback   unlink;
    open_cllback     open;
    close_cllback    close;

    ioctl_cllback         ioctl;
    get_size_cllback      get_size;
    chmod_cllback         chmod;
    get_file_size_cllback get_file_size;
    listdir_cllback       listdir;
};

struct dirent {

    u8int  name[256];
    u32int inode_num;
};

struct vfs_entry {

    u8int       *name;
    vfs_node_s  *file;
};

struct gtreenode {

    list_s    *children;
    void      *value;
};

struct gtree {

    gtreenode_s *root;
};


void vfs_open(vfs_node_s *node, u32int flags);
void vfs_close(vfs_node_s *node);
void vfs_chmod(vfs_node_s *node, u32int mode);
s32int vfs_ioctl(vfs_node_s *node, s32int req, void *argp);

u32int vfs_read(vfs_node_s *node, u32int offset, u32int size, u8int *buff);
u32int vfs_write(vfs_node_s *node, u32int offset, u32int size, u8int *buff);
u32int vfs_get_file_size(vfs_node_s *file);

vfs_node_s *get_mountpoint(u8int **path);
vfs_node_s *get_mountpoint_recur(u8int **path, gtreenode_s *subroot);

vfs_node_s *file_open(u8int *file_name, u32int flags);
vfs_node_s *vfs_finddir(vfs_node_s *node, u8int *name);

gtree_s *tree_create(void);
gtreenode_s *tree_insert(gtree_s *tree, gtreenode_s *subroot, void *value);
gtreenode_s *treenode_create(void *value);
void tree2array(gtreenode_s *subroot, void **array, u32int *size);

void vfs_init(void);
void vfs_mount(u8int *path, vfs_node_s *fs);
void vfs_mount_dev(u8int *path, vfs_node_s *node);
void vfs_mount_recur(u8int *path, gtreenode_s *subroot, vfs_node_s *fs);

s32int vfs_mkfile(u8int *name, u16int permission);
s32int vfs_mkdir(u8int *name, u16int permission);
s32int vfs_unlink(u8int *name);

void vfs_test(void);
void vfs_test_create(u8int *path, u8int *filename);
void vfs_tree_print(gtreenode_s *node, u32int offset);
u8int vfs_listdir(u8int *name);

#endif