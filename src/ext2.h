#ifndef      EXT2_H
#define      EXT2_H

#include    "common.h"
#include    "help_func.h"
#include    "monitor.h"
#include    "ata.h"

extern ata_s first_master;
extern ata_s first_slave;
extern ata_s second_master;
extern ata_s second_slave;

#define     EXT2_SIGN           0xEf53

#define     EXT2_S_IFIFO        0x1000
#define     EXT2_S_IFCHR        0x2000
#define     EXT2_S_IFDIR        0x4000
#define     EXT2_S_IFBLK        0x6000
#define     EXT2_S_IFREG        0x8000
#define     EXT2_S_IFLINK       0xA000
#define     EXT2_S_IFSOCK       0xC000

#define     EXT2_DIRECT_BLOCKS  12
#define     SUPBLOCK_SIZE       1024
#define     ROOT_INODE          0x02

typedef struct ext2_supblock ext2_supblock_s;
typedef struct ext2_inode ext2_inode_s;
typedef struct ext2_bgd ext2_bgd_s;
typedef struct ext2_fs ext2_fs_s;
typedef struct ext2_dir ext2_dir_s;

struct ext2_supblock {

u32int      total_inodes;
u32int      total_blocks;
u32int      su_blocks;
u32int      free_blocks;
u32int      free_inodes;
u32int      supblock_idx;
u32int      log2block_size;
u32int      log2frag_size;
u32int      blocks_per_group;
u32int      frags_per_group;
u32int      inodes_per_group;

u32int      mtime;
u32int      wtime;

u16int      mount_count;
u16int      mount_allowed_count;
u16int      ext2_magic;
u16int      fs_state;
u16int      err;
u16int      minor;

u32int      last_check;
u32int      interval;
u32int      os_id;
u32int      major;

u16int      r_userid;
u16int      r_groupid;

// Extended features (not used for now since we're doing ext2 only)
u32int      first_inode;
u16int      inode_size;
u16int      superblock_group;
u32int      optional_feature;
u32int      required_feature;
u32int      readonly_feature;

u8int       fs_id[16];
u8int       vol_name[16];
u8int       last_mount_path[64];

u32int      compression_method;
u8int       file_pre_alloc_blocks;
u8int       dir_per_alloc_blocks;
u16int      unsd1;
u8int       journal_id[16];
u32int      journal_inode;
u32int      journal_device;
u32int      orphan_head;

u8int       unsd2[1024 - 236];
}__attribute__((packed));

struct ext2_inode {

u16int      permission;
u16int      userid;
u32int      size;
u32int      atime;
u32int      ctime;
u32int      mtime;
u32int      dtime;
u16int      gid;
u16int      hard_links;
u32int      num_sectors;
u32int      flags;
u32int      os_spec1;
u32int      blocks[EXT2_DIRECT_BLOCKS + 3];
u32int      gener;
u32int      file_acl;

union {
    u32int  dir_acl;
    u32int  size_high;
};
u32int      f_block_addr;
u8int       os_spec2[12];
}__attribute__((packed));

struct ext2_bgd {

u32int      bitmap_block;
u32int      bitmap_inode;
u32int      table_inode;
u16int      free_blocks;
u16int      free_inodes;
u16int      num_dirs;
u32int      unsd1;
u32int      unsd2;
u32int      unsd3;
}__attribute__((packed));

struct ext2_fs {

    vfs_node_s      *disk_dev;
    ext2_supblock_s *sb;
    ext2_bgd_s      *bgds;
    u32int          block_size;
    u32int          blocks_per_group;
    u32int          inodes_per_group;
    u32int          total_groups;
    u32int          bgd_blocks;
};

struct ext2_dir {

    u32int  inode;
    u16int  size;
    u8int   name_len;
    u8int   type;
    u8int   name[];
}__attribute__((packed));

void ext2_init(u8int *dev_path, u8int *mountpoint);
void ext2_read_block(ext2_fs_s *fs, u32int block, u8int *buff);
void ext2_write_block(ext2_fs_s *fs, u32int block, u8int *buff);

u32int get_disk_block_number(ext2_fs_s *fs, ext2_inode_s *inode, u32int inode_block);
void set_disk_block_number(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx, u32int inode_block, u32int disk_block);

void ext2_write_inode_block(ext2_fs_s *fs, ext2_inode_s *inode, u32int inode_block, u8int *buff);
u8int *ext2_read_inode_block(ext2_fs_s *fs, ext2_inode_s *inode, u32int inode_block);

void ext2_write_inode_filedata(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx, u32int offset, u32int size, u8int *buff);
u32int ext2_read_inode_filedata(ext2_fs_s *fs, ext2_inode_s *inode, u32int offset, u32int size, u8int *buff);

void ext2_write_inode_metadata(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx);
void ext2_read_inode_metadata(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx);

void ext2_open(vfs_node_s *file, u32int flags);
void ext2_close(vfs_node_s *file);
void ext2_chmod(vfs_node_s *file, u32int mode);

u32int ext2_read(vfs_node_s *file, u32int offset, u32int size, u8int *buff);
u32int ext2_write(vfs_node_s *file, u32int offset, u32int size, u8int *buff);

u32int alloc_inode_metadata_block(u32int *block_ptr, ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx, u8int *buff, u32int block_overw);

u32int ext2_alloc_block(ext2_fs_s *fs);
u32int ext2_alloc_inode(ext2_fs_s *fs);
void alloc_inode_block(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx, u32int block);
void free_inode_block(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx, u32int block);
void rewrite_bgds(ext2_fs_s *fs);
void rewrite_superblock(ext2_fs_s *fs);

vfs_node_s *ext2_get_root(ext2_fs_s *fs, ext2_inode_s *inode);
vfs_node_s *vfsnode_from_direntry(ext2_fs_s *fs, ext2_dir_s *dir, ext2_inode_s *inode);
vfs_node_s *ext2_finddir(vfs_node_s *parent, u8int *name);

u32int ext2_create_entry(vfs_node_s *parent, u8int *name, u32int inode_entry);
void ext2_mkfile(vfs_node_s *parent, u8int *name, u16int permis);
void ext2_mkdir(vfs_node_s *parent, u8int *name, u16int permis);
void ext2_unlink(vfs_node_s *parent, u8int *name);
u32int ext2_file_size(vfs_node_s *file);
u8int **ext2_listdir(vfs_node_s *parent);

void ext2_remov_entry(vfs_node_s *parent, u8int *entry_name);
void ext2_free_block(ext2_fs_s *fs, u32int block);
void ext2_free_inode(ext2_fs_s *fs, u32int inode);
void free_all_inode_blocks(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_num);
void ext2_del_inode(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_num);

void ext2_show_inode(ext2_inode_s *inode);
void ext2_show_fs(vfs_node_s *node);
void ext2_show_sb(ext2_fs_s *fs);
void ext2_test(vfs_node_s *file);

#endif