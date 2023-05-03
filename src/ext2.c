#include    "ext2.h"

u8int pat_dir[] = {0x02, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x01, 0x02, 0x2E, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x02, 0x02, 0x2E, 0x2E};


void ext2_read_block(ext2_fs_s *fs, u32int block, u8int *buff) {

    vfs_read(fs->disk_dev, fs->block_size * block, fs->block_size, buff);
    return;
}

void ext2_write_block(ext2_fs_s *fs, u32int block, u8int *buff) {

    vfs_write(fs->disk_dev, fs->block_size * block, fs->block_size, buff);
    return;
}
//ext2_init("/dev/hda", "/");
void ext2_init(u8int *dev_path, u8int *mountpoint) {

    ext2_fs_s *fs = kcalloc(sizeof(ext2_fs_s), 1);
    fs->sb = kmalloc(SUPBLOCK_SIZE);

    fs->disk_dev = file_open(dev_path, 0);
    fs->block_size = 1024;

    ext2_read_block(fs, 1, (u8int *)fs->sb);

    fs->block_size = (1024 << fs->sb->log2block_size);
    fs->blocks_per_group = fs->sb->blocks_per_group;
    fs->inodes_per_group = fs->sb->inodes_per_group;

    fs->total_groups = fs->sb->total_blocks / fs->blocks_per_group;
    if(fs->blocks_per_group * fs->total_groups < fs->sb->total_blocks) {
        fs->total_groups++;
    }

    fs->bgd_blocks = (fs->total_groups * sizeof(ext2_bgd_s)) / fs->block_size;
    if(fs->block_size * fs->bgd_blocks < fs->total_groups * sizeof(ext2_bgd_s)) {
        fs->bgd_blocks++;
    }

    fs->bgds = kcalloc(sizeof(ext2_bgd_s), fs->bgd_blocks * fs->block_size);
    for(u32int i = 0; i < fs->bgd_blocks; i++) {
        ext2_read_block(fs, 2 + i, (u8int *)fs->bgds + i * fs->block_size);
    }   

    ext2_inode_s *root_inode = kcalloc(sizeof(ext2_inode_s), 1);
    ext2_read_inode_metadata(fs, root_inode, ROOT_INODE);
    vfs_node_s *node = ext2_get_root(fs, root_inode);
    vfs_mount(mountpoint, node);

    kfree(root_inode);
    return;
}

vfs_node_s *ext2_get_root(ext2_fs_s *fs, ext2_inode_s *inode) {

    vfs_node_s *root = kcalloc(sizeof(vfs_node_s), 1);
    strcpy(root->name, "/");

    root->device = fs;
    root->mask = inode->permission;
    root->flags |= FS_DIRECTORY;
    root->inode_num = ROOT_INODE;
    root->time_create =   inode->ctime;
    root->time_access =   inode->atime;
    root->time_modified = inode->mtime;

    root->read = ext2_read;
    root->write = ext2_write;
    //root->readdir = ;
    root->finddir = ext2_finddir;
    root->create = ext2_mkfile;
    root->mkdir = ext2_mkdir;
    root->unlink = ext2_unlink;
    root->open = ext2_open;
    root->close = ext2_close;
    root->chmod = ext2_chmod;
    root->listdir = ext2_listdir;

    return root;
}

u32int get_disk_block_number(ext2_fs_s *fs, ext2_inode_s *inode, u32int inode_block) {

    s32int a, b, c, d, e, f, g;
    u32int p, ret;
    p = fs->block_size / 4;
    u32int *tmp = kmalloc(fs->block_size);
// How many blocks are left except for direct blocks ?
    a = inode_block - EXT2_DIRECT_BLOCKS;
    if(a < 0) {
        ret = inode->blocks[inode_block];
        goto ext;
    }

    b = a - p;
    if(b < 0) {

        ext2_read_block(fs, inode->blocks[EXT2_DIRECT_BLOCKS], (void *)tmp);
        ret = tmp[a];
        goto ext;
    }

    c = b - p * p;
    if(c < 0) {

        c = b / p;
        d = b - c * p;

        ext2_read_block(fs, inode->blocks[EXT2_DIRECT_BLOCKS + 1], (void *)tmp);
        ext2_read_block(fs, tmp[c], (void *)tmp);

        ret = tmp[d];
        goto ext;
    }

    d = c - p * p * p;
    if(d < 0) {
        e = c / (p * p);
        f = (c - e * p * p) / p;
        g = (c - e * p * p - f * p);

        ext2_read_block(fs, inode->blocks[EXT2_DIRECT_BLOCKS + 2], (void *)tmp);
        ext2_read_block(fs, tmp[e], (void *)tmp);
        ext2_read_block(fs, tmp[f], (void *)tmp);

        ret = tmp[g];
        goto ext;
    }

    ext:
        kfree(tmp);
        return ret;
}

u8int *ext2_read_inode_block(ext2_fs_s *fs, ext2_inode_s *inode, u32int inode_block) {

    u8int *buff = kmalloc(fs->block_size);
    u32int block = get_disk_block_number(fs, inode, inode_block);
    ext2_read_block(fs, block, buff);

    return buff;
}

void ext2_write_inode_block(ext2_fs_s *fs, ext2_inode_s *inode, u32int inode_block, u8int *buff) {

    u32int block = get_disk_block_number(fs, inode, inode_block);
    ext2_write_block(fs, block, buff);

    return;
}

u32int ext2_read_inode_filedata(ext2_fs_s *fs, ext2_inode_s *inode, u32int offset, u32int size, u8int *buff) {

    u32int end_bytes = (inode->size >= offset + size) ? (offset + size) : inode->size;
    u32int start_block = offset / fs->block_size;
    u32int start_offset = offset % fs->block_size;

    u32int end_block = end_bytes / fs->block_size;
     // How much bytes to read for the end block
    u32int end_offset = end_bytes - end_block * fs->block_size;

    u32int i = start_block;
    u32int curr_off = 0;

    while(i <= end_block) {

        u32int left = 0;
        u32int right = fs->block_size - 1;

        u8int *buf_block = ext2_read_inode_block(fs, inode, i);

        if(i == start_block) {
            left = start_offset;
        }
        if(i == end_block) {
            right = end_offset - 1;
        }

        memcpy(buff + curr_off, buf_block + left, (right - left + 1));
        curr_off += (right - left + 1);
        kfree(buf_block);
        i++;
    }
    return end_bytes - offset;
}

void ext2_write_inode_filedata(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx, u32int offset, u32int size, u8int *buff) {

    if(offset + size > inode->size) {
        inode->size = offset + size;
        ext2_write_inode_metadata(fs, inode, ino_idx);
    }
    u32int end_bytes = (inode->size >= offset + size) ? (offset + size) : inode->size;
    u32int start_block = offset / fs->block_size;
    u32int start_offset = offset % fs->block_size;

    u32int end_block = end_bytes / fs->block_size;
    u32int end_offset = end_bytes - end_block * fs->block_size;

    u32int i = start_block;
    u32int curr_off = 0;
    u32int test = 0;

    while(i <= end_block) {

        u32int left = 0;
        u32int right = fs->block_size - 1;
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if(!(test = get_disk_block_number(fs, inode, i))) {           //need test!!!!!!!!!!!
            alloc_inode_block(fs, inode, ino_idx, i);
        }
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!        
        u8int *buf_block = ext2_read_inode_block(fs, inode, i);

        if(i == start_block) {
            left = start_offset;
        }
        if(i == end_block) {
            right = end_offset - 1;
        }

        memcpy(buf_block + left, buff + curr_off, (right - left + 1));
        curr_off += (right - left + 1);
        ext2_write_inode_block(fs, inode, i, buf_block);
        kfree(buf_block);
        i++;
    }
    return;
}

void ext2_read_inode_metadata(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx) {

    u32int group = ino_idx / fs->inodes_per_group;
    u32int idx_in_group = ino_idx - group * fs->inodes_per_group;
    u32int block_offset = (idx_in_group - 1) * fs->sb->inode_size / fs->block_size;
    u32int offset_in_block = (idx_in_group - 1) - block_offset * (fs->block_size / fs->sb->inode_size);

    u8int *buf_block = kmalloc(fs->block_size);
    u32int inode_table = fs->bgds[group].table_inode;

    ext2_read_block(fs, inode_table + block_offset, buf_block);
    memcpy(inode, buf_block + offset_in_block * fs->sb->inode_size, fs->sb->inode_size);
    kfree(buf_block);
    
    return;
 }

 void ext2_write_inode_metadata(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx) {

    u32int group = ino_idx / fs->inodes_per_group;
    u32int idx_in_group = ino_idx - group * fs->inodes_per_group;
    u32int block_offset = (idx_in_group - 1) * fs->sb->inode_size / fs->block_size;
    u32int offset_in_block = (idx_in_group - 1) - block_offset * (fs->block_size / fs->sb->inode_size);

    u8int *buf_block = kmalloc(fs->block_size);   
    u32int inode_table = fs->bgds[group].table_inode;

    ext2_read_block(fs, inode_table + block_offset, buf_block);
    memcpy(buf_block + offset_in_block * fs->sb->inode_size, inode, fs->sb->inode_size);

    ext2_write_block(fs, inode_table + block_offset, buf_block);
    kfree(buf_block);

    return;
 }

 //Read n bytes to file starting from offset
 u32int ext2_read(vfs_node_s *file, u32int offset, u32int size, u8int *buff) {

    ext2_fs_s *fs = file->device;
    ext2_inode_s *inode = kmalloc(sizeof(ext2_inode_s));

    ext2_read_inode_metadata(fs, inode, file->inode_num);
    ext2_read_inode_filedata(fs, inode, offset, size, buff);

    kfree(inode);
    return size;
 }

 u32int ext2_write(vfs_node_s *file, u32int offset, u32int size, u8int *buff) {

    ext2_fs_s *fs = file->device;
    ext2_inode_s *inode = kmalloc(sizeof(ext2_inode_s));

    ext2_read_inode_metadata(fs, inode, file->inode_num);
    ext2_write_inode_filedata(fs, inode, file->inode_num, offset, size, buff);

    kfree(inode);
    return size;
 }

 u32int alloc_inode_metadata_block(u32int *block_ptr, ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx, u8int *buff, u32int block_overw) {

    if(!(*block_ptr)) {
        
        u32int no = ext2_alloc_block(fs);
        if(!no) {
            monitor_str_write("!!!ERR (set_disk_block_number()->alloc_inode_metad...()! Have not free block!\n");
            return 0;
        }
        *block_ptr = no;

        if(buff) {
            ext2_write_block(fs, block_overw, (void *)buff);
        }
        else {
            ext2_write_inode_metadata(fs, inode, ino_idx);
        }
    /*
        monitor_str_write("alloc in set:");
        write_dec(*block_ptr);
        monitor_char_put('\n');
    */
        return 1;
    }
    /*
    monitor_str_write("exist block:");
    write_dec(*block_ptr);
    monitor_char_put('\n');
*/
    return 0;
 }

 u32int ext2_alloc_block(ext2_fs_s *fs) {

    u32int *buf = kmalloc(fs->block_size);
     // Read the inode bitmap, find free inode, return its index
    for(u32int i = 0; i < fs->total_groups; i++) {

        if(!fs->bgds[i].free_blocks) {
            continue;
        }
        u32int bitmap_blck = fs->bgds[i].bitmap_block;
        ext2_read_block(fs, bitmap_blck, (void *)buf);

        for(u32int j = 0; j < fs->block_size/4; j++) {

            u32int sub_bitmap = buf[j];
            if(sub_bitmap == 0xFFFFFFFF) {
                continue;
            }
            for(u32int n = 0; n < 32; n++) {
                u32int free = !((sub_bitmap >> n) & 0x01);

                if(free) {
                    // Set bitmap and return
                    u32int mask = (0x01 << n);
                    buf[j] = buf[j] | mask;
                    ext2_write_block(fs, bitmap_blck, (void *)buf);
                    // update free_inodes
                    fs->bgds[i].free_blocks--;
                    rewrite_bgds(fs);

                    kfree(buf);
                    return i * fs->blocks_per_group + j * 32 + n;
                }
            }
        }
    }
    kfree(buf);
    return -1;
 }

 u32int ext2_alloc_inode(ext2_fs_s *fs) {

    u32int *buf = kmalloc(fs->block_size);
     // Read the inode bitmap, find free inode, return its index
     for(u32int i = 0; i < fs->total_groups; i++) {

        if(!fs->bgds[i].free_inodes) {
            continue;
        }
        u32int bitmap_block = fs->bgds[i].bitmap_inode;
        ext2_read_block(fs, bitmap_block, (void *)buf);

        for(u32int j = 0; j < fs->block_size/4; j++) {

            u32int sub_bitmap = buf[j];
            if(sub_bitmap == 0xFFFFFFFF) {
                continue;
            }
            for(u32int n = 0; n < 32; n++) {
                u32int free = !((sub_bitmap >> n) & 0x01);

                if(free) {
                    // Set bitmap and return
                    u32int mask = (0x01 << n);
                    buf[j] = buf[j] | mask;
                    ext2_write_block(fs, bitmap_block, (void *)buf);
                    // update free_inodes
                    fs->bgds[i].free_inodes--;
                    rewrite_bgds(fs);

                    kfree(buf);
                    return i * fs->inodes_per_group + j * 32 + n;
                }
            }
        }
    }
    kfree(buf);
    return -1;
 }

 void rewrite_bgds(ext2_fs_s *fs) {

    for(u32int i = 0; i < fs->bgd_blocks; i++) {
        ext2_write_block(fs, 2 + i, (void *)fs->bgds + i * fs->block_size);
    }
    return;
 }

 void rewrite_superblock(ext2_fs_s *fs) {

    ext2_write_block(fs, 1, (void *)fs->sb);
    return;
 }

 void set_disk_block_number(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx, u32int inode_block, u32int disk_block) {

    s32int a, b, c, d, e, f, g;
    s32int iblock = inode_block;

    u32int p = fs->block_size / 4;
    u32int *ret = kmalloc(fs->block_size);

    a = iblock - EXT2_DIRECT_BLOCKS;
    if(a < 0) {
        inode->blocks[inode_block] = disk_block;
        goto ext;
    }
    b = a - p;
    if(b < 0) {

        //monitor_str_write("more then 11\n");
        if(!alloc_inode_metadata_block(&(inode->blocks[EXT2_DIRECT_BLOCKS]), fs, inode, ino_idx, NULL, 0));
        ext2_read_block(fs, inode->blocks[EXT2_DIRECT_BLOCKS], (void *)ret);

        ((u32int *)ret)[a] = disk_block;
        ext2_write_block(fs, inode->blocks[EXT2_DIRECT_BLOCKS], (void *)ret);
        goto ext;
    }
    c = b - p * p;
    if(c < 0) {
        c = b / p;
        d = b - c * p;
        //monitor_str_write("more then 267\n");

        if(!alloc_inode_metadata_block(&(inode->blocks[EXT2_DIRECT_BLOCKS + 1]), fs, inode, ino_idx, NULL, 0));
        ext2_read_block(fs, inode->blocks[EXT2_DIRECT_BLOCKS + 1], (void *)ret);

        if(!alloc_inode_metadata_block(&(ret[c]), fs, inode, ino_idx, (void *)ret, inode->blocks[EXT2_DIRECT_BLOCKS + 1]));
        u32int tmp = ret[c];

        ext2_read_block(fs, tmp, (void *)ret);
        ret[d] = disk_block;
        ext2_write_block(fs, tmp, (void *)ret);
        goto ext;
    }
    d = c - p * p * p;
    if(d < 0) {
        e = c / (p * p);
        f = (c - e * p * p) / p;
        g = (c - e * p * p - f * p);

        if(!alloc_inode_metadata_block(&(inode->blocks[EXT2_DIRECT_BLOCKS + 2]), fs, inode, ino_idx, NULL, 0));
        ext2_read_block(fs, inode->blocks[EXT2_DIRECT_BLOCKS + 2], (void *)ret);

        if(!alloc_inode_metadata_block(&(ret[e]), fs, inode, ino_idx, (void *)ret, inode->blocks[EXT2_DIRECT_BLOCKS + 2]));
        u32int tmp = ret[e];

        ext2_read_block(fs, ret[e], (void *)ret);

        if(!alloc_inode_metadata_block(&(ret[f]), fs, inode, ino_idx, (void *)ret, tmp));
        tmp = ret[f];

        ext2_read_block(fs, ret[f], (void *)ret);
        ret[g] = disk_block;
        ext2_write_block(fs, tmp, (void *)ret);
        goto ext;
    }

    ext:
        ext2_write_inode_metadata(fs, inode, ino_idx);
        kfree(ret);
        return;
 }

vfs_node_s *vfsnode_from_direntry(ext2_fs_s *fs, ext2_dir_s *dir, ext2_inode_s *inode) {

    vfs_node_s* ret = kcalloc(sizeof(vfs_node_s), 1);

    ret->device = (void *)fs;
    ret->inode_num = dir->inode;
    memcpy(ret->name, dir->name, dir->name_len);

    ret->uid = inode->userid;
    ret->gid = inode->gid;
    ret->size = inode->size;
    ret->mask = inode->permission & 0xFFF;
    ret->nlink = inode->hard_links;
    ret->flags = 0;

    if((inode->permission & EXT2_S_IFREG) == EXT2_S_IFREG) {

        ret->flags |= FS_FILE;
        ret->read = ext2_read;
        ret->write = ext2_write;
        ret->unlink = ext2_unlink;
        ret->get_file_size = ext2_file_size;
    }
    else if((inode->permission & EXT2_S_IFDIR) == EXT2_S_IFDIR) {

        ret->flags |= FS_DIRECTORY;
        ret->read = ext2_read;
        ret->write = ext2_write;
        ret->finddir = ext2_finddir;
        ret->mkdir = ext2_mkdir;
        ret->create = ext2_mkfile;
        ret->unlink = ext2_unlink;
        ret->listdir = ext2_listdir;
    }
    else if((inode->permission & EXT2_S_IFIFO) == EXT2_S_IFIFO) {
        ret->flags |= FS_PIPE;
    }
    else if((inode->permission & EXT2_S_IFBLK) == EXT2_S_IFBLK) {
        ret->flags |= FS_BLOCKDEV;
    }
    else if((inode->permission & EXT2_S_IFLINK) == EXT2_S_IFLINK) {
        ret->flags |= FS_SYMLINK;
    }
    else if((inode->permission & EXT2_S_IFCHR) == EXT2_S_IFCHR) {
        ret->flags |= FS_CHARDEV;
    }

    ret->time_create = inode->ctime;
    ret->time_access = inode->atime;
    ret->time_modified = inode->mtime;

    ret->open = ext2_open;
    ret->close = ext2_close;
    ret->chmod = ext2_chmod;
   
    return ret;
}

vfs_node_s *ext2_finddir(vfs_node_s *parent, u8int *name) {

    ext2_inode_s *p_inode = kmalloc(sizeof(ext2_inode_s));
    ext2_fs_s * fs = parent->device;
    vfs_node_s *ret;

    u32int curr_offset = 0;
    u32int block_offset = 0;
    u32int in_block_offset = 0;
    u32int real_size;
    u32int expect_size;

    ext2_read_inode_metadata(fs, p_inode, parent->inode_num);
    u8int *block_buf = ext2_read_inode_block(fs, p_inode, block_offset);

    while(curr_offset < p_inode->size) {

        if(in_block_offset >= fs->block_size) {

            if(curr_offset > p_inode->size) {
                kfree(block_buf);
                kfree(p_inode);
                return NULL;
            }
            block_offset++;
            in_block_offset = 0;
            kfree(block_buf);
            block_buf = ext2_read_inode_block(fs, p_inode, block_offset);
        }

        ext2_dir_s *curr_dir = (ext2_dir_s *) (block_buf + in_block_offset);
        u8int *tmp = kcalloc(curr_dir->name_len + 1, 1);
        memcpy(tmp, curr_dir->name, curr_dir->name_len);

        if(curr_dir->inode != 0 && !strcmp(tmp, name)) {
            /*                                                                      //since the memory is blocked during allocation, we copy
            ext2_dir_s *rez_dir = kmalloc(sizeof(ext2_dir_s) + curr_dir->name_len);
            memcpy(rez_dir, curr_dir, curr_dir->name_len + sizeof(ext2_dir_s));
            curr_dir = rez_dir;
            */
            ext2_inode_s *inode = kmalloc(sizeof(ext2_inode_s));
            ext2_read_inode_metadata(fs, inode, curr_dir->inode);
            ret = vfsnode_from_direntry(fs, curr_dir, inode);

            vfs_node_s *rez_ret = kmalloc(sizeof(vfs_node_s));      //since the memory is blocked during allocation, we copy
            memcpy(rez_ret, ret, sizeof(vfs_node_s));

            kfree(block_buf);
            //kfree(rez_ret);
            kfree(p_inode);
            kfree(inode);
            kfree(ret);
            kfree(tmp);

            return rez_ret;
        }

        if(((sizeof(ext2_dir_s) + curr_dir->name_len) & 0x00000003) != 0) {
            expect_size = ((sizeof(ext2_dir_s) + curr_dir->name_len) & 0xFFFFFFFC) + 0x04;
        }
        else {
            expect_size = ((sizeof(ext2_dir_s) + curr_dir->name_len) & 0xFFFFFFFC);
        }                                                              
        real_size = curr_dir->size;

        if(real_size != expect_size) {
            break;
        }

        in_block_offset += curr_dir->size;
        curr_offset += curr_dir->size;
        kfree(tmp);
    }
    kfree(block_buf);
    kfree(p_inode);

    return NULL;
}

void ext2_close(vfs_node_s *file) {

    kfree(file);
    return;
}

void ext2_open(vfs_node_s *file, u32int flags) {

    ext2_fs_s *fs = file->device;   

    if(flags & O_TRUNC) {

        ext2_inode_s *inode = kmalloc(sizeof(ext2_inode_s));
        ext2_read_inode_metadata(fs, inode, file->inode_num);
        inode->size = 0;
        ext2_write_inode_metadata(fs, inode, file->inode_num);
        kfree(inode);
    }
    return;
}

void ext2_chmod(vfs_node_s *file, u32int mode) {

    ext2_fs_s *fs = file->device;
    ext2_inode_s *inode = kmalloc(sizeof(ext2_inode_s));

    ext2_read_inode_metadata(fs, inode, file->inode_num);
    inode->permission = (inode->permission & 0xFFFFF000) | mode;
    ext2_write_inode_metadata(fs, inode, file->inode_num);
    return;
}

void alloc_inode_block(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx, u32int ino_block) {

    u32int ret = ext2_alloc_block(fs);
    set_disk_block_number(fs, inode, ino_idx, ino_block, ret);
    inode->num_sectors += (ino_block + 1) * (fs->block_size / 512);
    ext2_write_inode_metadata(fs, inode, ino_idx);    //the same function can be removed in set_disk_blcok_number();

    return;
}

void free_inode_block(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_idx, u32int ino_block) {

    u32int ret = get_disk_block_number(fs, inode, ino_block);
    ext2_free_block(fs, ret);
    set_disk_block_number(fs, inode, ino_idx, ret, 0);
    ext2_write_inode_metadata(fs, inode, ino_idx);    //the same function can be removed in set_disk_blcok_number();

    return;
}

u32int ext2_create_entry(vfs_node_s *parent, u8int *entry_name, u32int entry_inode) {

    ext2_fs_s *fs = parent->device;

    u32int entry_name_len = strlen(entry_name);
    u32int in_block_offset = 0;
    u32int block_offset = 0;
    u32int curr_offset = 0;
    u32int found = 0;

    ext2_inode_s *p_inode = kmalloc(sizeof(ext2_inode_s));
    u8int *check = kcalloc(entry_name_len + 1, 1);
    ext2_read_inode_metadata(fs, p_inode, parent->inode_num);
    u8int *block_buf = ext2_read_inode_block(fs, p_inode, block_offset);


    while(curr_offset < p_inode->size) {

        if(in_block_offset >= fs->block_size) {

            block_offset++;
/*
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            u32int test = 0;
            if(!(test = get_disk_block_number(fs, p_inode, block_offset))) {
                alloc_inode_block(fs, p_inode, parent->inode_num, block_offset);
            }
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/
            in_block_offset = 0;
            kfree(block_buf);
            block_buf = ext2_read_inode_block(fs, p_inode, block_offset);
        }
        ext2_dir_s *curr_dir = (ext2_dir_s *)(block_buf + in_block_offset);

        if(curr_dir->name_len == entry_name_len) {
            memcpy(check, curr_dir->name, entry_name_len);

            if(curr_dir->inode != 0 && !strcmp(entry_name, check)) {
                monitor_str_write("Entry by the same name '");
                monitor_str_write(check);
                monitor_str_write("' already exist\n");

                kfree(p_inode);
                kfree(block_buf);
                kfree(check);
                return 1;
            }
        }

        if(found) {                    // Found the last entry
        // Overwrite this last entry with our new entry
            curr_dir->inode = entry_inode;
            curr_dir->size = (u32int)block_buf + fs->block_size - (u32int)curr_dir;
            curr_dir->name_len = entry_name_len;
            curr_dir->type = 0;
            // Must use memcpy instead of strcpy, because name in direntry does not contain ending '\0'
            memcpy(curr_dir->name, entry_name, entry_name_len);
            ext2_write_inode_block(fs, p_inode, block_offset, block_buf);

            in_block_offset += curr_dir->size;
            curr_offset += curr_dir->size;

            if(in_block_offset >= fs->block_size) {

                if(curr_offset > p_inode->size) {
                    kfree(p_inode);
                    kfree(block_buf);
                    kfree(check);
                    return 3;
                }
                block_offset++;
/*
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                u32int test = 0;                                   
                if(!(test = get_disk_block_number(fs, p_inode, block_offset))) {
                    alloc_inode_block(fs, p_inode, parent->inode_num, block_offset);
                }
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/
                in_block_offset = 0;
                kfree(block_buf);
                block_buf = ext2_read_inode_block(fs, p_inode, block_offset);
            }

            curr_dir = (ext2_dir_s *) (block_buf + in_block_offset);
            memset(curr_dir, 0, sizeof(ext2_dir_s));
            ext2_write_inode_block(fs, p_inode, block_offset, block_buf);

            kfree(p_inode);
            kfree(block_buf);
            kfree(check);
            return 0;
        }
        u32int expect_size = ((sizeof(ext2_dir_s) + curr_dir->name_len) & 0xFFFFFFFC) + 0x04;
        //u32int expect_size = ((sizeof(ext2_dir_s) + curr_dir->name_len + 1) & 0xFFFFFFFC) + 0x04;
        u32int real_size = curr_dir->size;

        if(!curr_dir->inode) {
            if( curr_dir->name_len == entry_name_len) {

                curr_dir->inode = entry_inode;
                curr_dir->size = expect_size;
                curr_dir->type = 0;
                //curr_dir->name_len = entry_name_len;

                memcpy(curr_dir->name, entry_name, entry_name_len);
                ext2_write_inode_block(fs, p_inode, block_offset, block_buf);

                kfree(p_inode);
                kfree(block_buf);
                kfree(check);
                return 0;
            }
            /*
            if(curr_dir->name_len > entry_name_len) {

                u32int of1 = curr_dir->name[curr_dir->name_len];
                u32int raz = curr_dir->name_len - entry_name_len;
                memset(curr_dir->name, 0, curr_dir->name_len + 1);
                memcpy(curr_dir->name, entry_name, entry_name_len);
                curr_dir->name_len = entry_name_len;
                curr_dir->name[curr_dir->name_len] = of1 + raz;
                curr_dir->inode = entry_inode;
                curr_dir->size = expect_size;
                curr_dir->type = 0;
                ext2_write_inode_block(fs, p_inode, block_offset, block_buf);

                kfree(p_inode);
                kfree(block_buf);
                kfree(check);
                return;
            }
              */  
        }
        if(real_size != expect_size) {

            found = 1;
            curr_dir->size = expect_size;
            in_block_offset += expect_size;
            curr_offset += expect_size;
            //curr_dir->name[curr_dir->name_len] = 0;
            continue;
        }
        //in_block_offset += curr_dir->size + curr_dir->name[curr_dir->name_len];
        in_block_offset += curr_dir->size;
        //curr_offset += curr_dir->size + curr_dir->name[curr_dir->name_len];
        curr_offset += curr_dir->size;
    }   
    kfree(p_inode);
    kfree(block_buf);
    kfree(check);

    return 2;
}

void ext2_mkdir(vfs_node_s *parent, u8int *name, u16int permis) {

    ext2_fs_s *fs = parent->device;
    ext2_inode_s *inode = kmalloc(sizeof(ext2_inode_s));
    u32int ino_idx = ext2_alloc_inode(fs);
    ext2_read_inode_metadata(fs, inode, ino_idx);

    inode->permission = EXT2_S_IFDIR;
    inode->permission |= 0x0FFF & permis;
    inode->size = fs->block_size;
    inode->userid = 0;
    inode->gid = 0;
    inode->atime = 0;
    inode->ctime = 0;
    inode->dtime = 0;
    inode->f_block_addr = 0;
    inode->num_sectors = 0;
    inode->hard_links = 2;
    inode->flags = 0;
    inode->file_acl = 0;
    inode->dir_acl = 0;
    inode->os_spec1 = 0;
    inode->gener = 0;
    memset(inode->blocks, 0, sizeof(inode->blocks));
    memset(inode->os_spec2, 0, 12);

    alloc_inode_block(fs, inode, ino_idx, 0);
/*
monitor_str_write("IN MKDIR | inode(");
write_dec(ino_idx);
monitor_str_write(") | block(");
write_dec(inode->blocks[0]);
monitor_str_write(")\n");
*/
    ext2_write_inode_metadata(fs, inode, ino_idx);
    u8int *empty = kcalloc(fs->block_size, 1);
    memcpy(empty, pat_dir, sizeof(pat_dir));
    ext2_write_block(fs, inode->blocks[0], empty);
    kfree(empty);

    ext2_read_inode_metadata(fs, inode, parent->inode_num);
/*
monitor_str_write("|parent->name");
monitor_str_write(parent->name);
monitor_str_write("| parent->block[0](");
write_dec(inode->blocks[0]);
monitor_str_write(")\n");
*/
    u32int tmp = ext2_create_entry(parent, name, ino_idx);

    if(tmp != 0) {
        if(tmp == 1) {
            
        }
        else if(tmp == 2) {
            monitor_str_write("!DIR NOT CREATE!\n");
        }
        ext2_del_inode(fs, inode, ino_idx);
        kfree(inode);
        return;
    }

    ext2_inode_s *p_inode = kmalloc(sizeof(ext2_inode_s));
    ext2_read_inode_metadata(fs, p_inode, parent->inode_num);
    p_inode->hard_links++;
    ext2_write_inode_metadata(fs, p_inode, parent->inode_num);
    rewrite_bgds(fs);                   //write free_block;

    kfree(inode);
    kfree(p_inode);
    return;
}

void ext2_mkfile(vfs_node_s *parent, u8int *name, u16int permis) {

    ext2_fs_s *fs = parent->device;
    ext2_inode_s *inode = kmalloc(sizeof(ext2_inode_s));
    u32int ino_idx = ext2_alloc_inode(fs);
    ext2_read_inode_metadata(fs, inode, ino_idx);

    inode->permission = EXT2_S_IFREG;
    inode->permission |= 0x0FFF & permis;
    inode->size = fs->block_size;
    inode->userid = 0;
    inode->gid = 0;
    inode->atime = 0;
    inode->ctime = 0;
    inode->dtime = 0;
    inode->f_block_addr = 0;
    inode->num_sectors = 0;
    inode->hard_links = 2;
    inode->flags = 0;
    inode->file_acl = 0;
    inode->dir_acl = 0;
    inode->os_spec1 = 0;
    inode->gener = 0;
    memset(inode->blocks, 0, sizeof(inode->blocks));
    memset(inode->os_spec2, 0, 12);

    alloc_inode_block(fs, inode, ino_idx, 0);
    ext2_write_inode_metadata(fs, inode, ino_idx);

    u8int *empty = kcalloc(fs->block_size, 1);
    memcpy(empty, pat_dir, sizeof(pat_dir));
    ext2_write_block(fs, inode->blocks[0], empty);
    kfree(empty);

    u32int tmp = ext2_create_entry(parent, name, ino_idx);

    if(tmp != 0) {

        if(tmp == 2) {
            monitor_str_write("!FILE NOT CREATE!");
        }
        else if(tmp == 1) {

        }
        ext2_del_inode(fs, inode, ino_idx);
        kfree(inode);
        return;
    }

    ext2_inode_s *p_inode = kmalloc(sizeof(ext2_inode_s));
    ext2_read_inode_metadata(fs, p_inode, parent->inode_num);
    p_inode->hard_links++;
    ext2_write_inode_metadata(fs, p_inode, parent->inode_num);
    rewrite_bgds(fs);                   //write free_block

    kfree(inode);
    kfree(p_inode);
    return;
}

void ext2_unlink(vfs_node_s *parent, u8int *name) {     //Delete the file

    ext2_fs_s *fs = parent->device;
    ext2_inode_s *p_inode = kmalloc(sizeof(ext2_inode_s));

    ext2_remov_entry(parent, name);
    ext2_read_inode_metadata(fs, p_inode, parent->inode_num);
    p_inode->hard_links--;

    if(!p_inode->hard_links) {              //need test!!!!!!!!!
        
        free_all_inode_blocks(fs, p_inode, parent->inode_num);
    }
    ext2_write_inode_metadata(fs, p_inode, parent->inode_num);
    rewrite_bgds(fs);

    kfree(p_inode);
    return;
}

u32int ext2_file_size(vfs_node_s *file) {

    ext2_fs_s *fs = file->device;
    ext2_inode_s *inode = kmalloc(sizeof(ext2_inode_s));

    ext2_read_inode_metadata(fs, inode, file->inode_num);
    u32int a = inode->size;
    kfree(inode);

    return a;
}

void ext2_remov_entry(vfs_node_s *parent, u8int *entry_name) {      //Remove an entry

    ext2_fs_s *fs = parent->device;

    u32int entry_name_len = strlen(entry_name);
    u32int in_block_offset = 0;
    u32int block_offset = 0;
    u32int curr_offset = 0;

    ext2_inode_s *p_inode = kmalloc(sizeof(ext2_inode_s));
    ext2_read_inode_metadata(fs, p_inode, parent->inode_num);
    u8int *check = kcalloc(entry_name_len + 1, 1);
    u8int *block_buf = ext2_read_inode_block(fs, p_inode, block_offset);

    while(curr_offset < p_inode->size) {

        if(in_block_offset >= fs->block_size) {

            block_offset++;
            in_block_offset = 0;
            kfree(block_buf);
            block_buf = ext2_read_inode_block(fs, p_inode, block_offset);
        }
        ext2_dir_s *curr_dir = (ext2_dir_s *) (block_buf + in_block_offset);

        if(curr_dir->name_len == entry_name_len) {
            memcpy(check, curr_dir->name, entry_name_len);

            if(curr_dir->inode != 0 && !strcmp(entry_name, check)) {

                curr_dir->inode = 0;
                ext2_write_inode_block(fs, p_inode, block_offset, block_buf);

                kfree(block_buf);
                kfree(p_inode);
                kfree(check);
                return;
            }
        }

        u32int expect_size = ((sizeof(ext2_dir_s) + curr_dir->name_len) & 0xFFFFFFFC) + 0x04;
        u32int real_size = curr_dir->size;

        if(real_size != expect_size) {

            kfree(block_buf);
            kfree(p_inode);
            kfree(check);
            return;
        }

        in_block_offset += curr_dir->size;
        curr_offset += curr_dir->size;
    }
    kfree(block_buf);
    kfree(p_inode);
    kfree(check);

    return;
}

void ext2_free_block(ext2_fs_s *fs, u32int block) {

    u32int *buf = kcalloc(fs->block_size, 1);

    u32int group = block / fs->blocks_per_group;
    u32int sub_bitmap = (block - (fs->blocks_per_group * group)) / 4;
    u32int idx = (block - (fs->blocks_per_group * group)) % 4;

    u32int bitmap_block = fs->bgds[group].bitmap_block;
    ext2_read_block(fs, bitmap_block, (void *)buf);

    u32int mask = ~(0x01 << idx);
    buf[sub_bitmap] = buf[sub_bitmap] & mask;
    ext2_write_block(fs, bitmap_block, (void *)buf);

    fs->bgds[group].free_blocks++;
    rewrite_bgds(fs);
    kfree(buf);

    return;
}

void ext2_free_inode(ext2_fs_s *fs, u32int inode) {

    u32int *buf = kcalloc(fs->block_size, 1);

    u32int group = inode / fs->inodes_per_group;
    u32int sub_bitmap = (inode - (fs->inodes_per_group * group)) / 4;
    u32int idx = (inode - (fs->inodes_per_group * group)) % 4;

    u32int bitmap_block = fs->bgds[group].bitmap_inode;
    ext2_read_block(fs, bitmap_block, (void *)buf);

    u32int mask = ~(0x01 << idx);
    buf[sub_bitmap] = buf[sub_bitmap] & mask;
    ext2_write_block(fs, bitmap_block, (void *)buf);

    fs->bgds[group].free_inodes++;
    rewrite_bgds(fs);
    kfree(buf);

    return;
}

u8int **ext2_listdir(vfs_node_s *parent) {

    s32int size = 0;
    s32int i = 10;
    
    u32int in_block_offset = 0;
    u32int block_offset = 0;
    u32int curr_offset = 0;

    ext2_fs_s *fs = parent->device;
    ext2_inode_s *p_inode = kmalloc(sizeof(ext2_inode_s));
    ext2_read_inode_metadata(fs, p_inode, parent->inode_num);

    u8int **ret = kmalloc(sizeof(u8int *) * i);
    u8int *block_buf = ext2_read_inode_block(fs, p_inode, block_offset);

    while(curr_offset < p_inode->size) {

        if(in_block_offset >= fs->block_size) {

            if(curr_offset > p_inode->size) {
                kfree(block_buf);
                kfree(p_inode);
                return ret;
            }
            block_offset++;
            in_block_offset = 0;
            kfree(block_buf);
            block_buf = ext2_read_inode_block(fs, p_inode, block_offset);
        }
        if(size + 1 == i) {

            u8int *rez = kmalloc(sizeof(u8int) * i * 2);
            memcpy(rez, ret, i);
            kfree(ret);
            ret = (u8int **)rez;
            i = i * 2;
        }

        ext2_dir_s *curr_dir = (ext2_dir_s *)(block_buf + in_block_offset);
        if(curr_dir->inode ) {

            u8int *temp = kcalloc(curr_dir->name_len + 1, 1);
            memcpy(temp, curr_dir->name, curr_dir->name_len);
            ret[size++] = temp;

        }
        u32int expect_size = ((sizeof(ext2_dir_s) + curr_dir->name_len) & 0xFFFFFFFC) + 0x04;
        u32int real_size = curr_dir->size;

        if(real_size != expect_size) {
            break;
        }
        in_block_offset += curr_dir->size;
        curr_offset += curr_dir->size;
    }
    kfree(block_buf);
    kfree(p_inode);
    ret[size] = NULL;

    return ret;
}

void ext2_del_inode(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_num) {

    free_all_inode_blocks(fs, inode, ino_num);
    ext2_free_inode(fs, ino_num);

    return;
}

void free_all_inode_blocks(ext2_fs_s *fs, ext2_inode_s *inode, u32int ino_num) {

    u32int count_blocks = inode->num_sectors * 512 / fs->block_size;
    for(u32int i = 0; i <= count_blocks; i++) {

        free_inode_block(fs, inode, ino_num, i);
    }
    return;
}

//--------------------------------------------------------------------------------------
void ext2_show_sb(ext2_fs_s *fs) {

    monitor_str_write("SB\n");
    monitor_str_write("total_inodes: ");
    write_dec(fs->sb->total_inodes);
    monitor_str_write(" | total_blocks: ");
    write_dec(fs->sb->total_blocks);
    monitor_str_write("| ext2_magic: ");
    write_hex(fs->sb->ext2_magic);
    monitor_str_write(" | block_size: ");
    write_dec(1024 << fs->sb->log2block_size);
    monitor_char_put('\n');

    monitor_str_write("inodes_per_group:");
    write_dec(fs->sb->inodes_per_group);
    monitor_str_write(" | blocks_per_group: ");
    write_dec(fs->sb->blocks_per_group);
    monitor_str_write(" | supblock_idx: ");
    write_dec(fs->sb->supblock_idx);
    monitor_char_put('\n');

    monitor_str_write("free_inodes: ");
    write_dec(fs->sb->free_inodes);
    monitor_str_write(" | free_blocks: ");
    write_dec(fs->sb->free_blocks);
    monitor_str_write(" | fs_state: ");
    write_dec(fs->sb->fs_state);
    monitor_str_write(" | frag_size: ");
    write_dec(1024 << fs->sb->log2frag_size);
    monitor_char_put('\n');

    monitor_str_write("mtime: ");
    write_hex(fs->sb->mtime);
    monitor_str_write(" | wtime: ");
    write_hex(fs->sb->wtime);
    monitor_char_put('\n');

    return;
}

void ext2_show_inode(ext2_inode_s *inode) {

    monitor_str_write("INODE show \n");
    monitor_str_write("permiss:");
    write_hex(inode->permission);
    monitor_str_write("|size:");
    write_dec(inode->size);
    monitor_str_write("|links:");
    write_dec(inode->hard_links);
    monitor_str_write("|sectors:");
    write_dec(inode->num_sectors);
    monitor_str_write("\nctime:");
    write_hex(inode->ctime);
    monitor_str_write("|atime:");
    write_hex(inode->atime);
    monitor_str_write("|blocks[0]:");
    write_dec(inode->blocks[0]);
    monitor_char_put('\n');

    return;
}

void ext2_show_fs(vfs_node_s *node) {

    ext2_fs_s *fs = node->device;
    monitor_str_write("FS show \n");
    
    monitor_str_write("size_bks:");
    write_dec(fs->block_size);
    monitor_str_write("|b_per_gr:");
    write_dec(fs->blocks_per_group);
    monitor_str_write("|i_per_gr:");
    write_dec(fs->inodes_per_group);
    monitor_str_write("|total_gr:");
    write_dec(fs->total_groups);
    monitor_str_write("|bgd_bks:");
    write_dec(fs->bgd_blocks);

    monitor_str_write("\nbgds:");
    write_hex((u32int)fs->bgds);
    monitor_str_write("|inode_bitmap:");
    write_dec(fs->bgds[0].bitmap_inode);
    monitor_char_put('\n');
    return;
}

void ext2_show_bgd(ext2_fs_s *fs, u32int group) {

    ext2_bgd_s *bgd = (ext2_bgd_s *)&fs->bgds[group];
    monitor_str_write("GROUP ");
    write_dec(group);

    monitor_str_write("\nbitmap_bl:");
    write_dec(bgd->bitmap_block);
    monitor_str_write("|bitmap_i:");
    write_dec(bgd->bitmap_inode);
    monitor_str_write("|tab_i:");
    write_dec(bgd->table_inode);
    monitor_str_write("|free_bl:");
    write_dec(bgd->free_blocks);
    monitor_str_write("|free_i:");
    write_dec(bgd->free_inodes);
    monitor_str_write("|num_dirs:");
    write_dec(bgd->num_dirs);
    monitor_char_put('\n');

    return;
}

void ext2_test(vfs_node_s *file) {

    monitor_clear();

    ext2_create_entry(file, "hiJeck", 14);
    ext2_create_entry(file, "hiJeck", 15);
    ext2_create_entry(file, "hiNick", 16);
    ext2_create_entry(file, "hiAnna", 89);
    ext2_create_entry(file, "hiSam", 44);

    ext2_remov_entry(file, "hiNick");
    ext2_remov_entry(file, "hiAnna");

    ext2_mkdir(file, "hiDan", EXT2_S_IFREG);
    ext2_create_entry(file, "hiMiti", 34);
 
    ext2_mkfile(file, "testFunFile", 0xFFFF);
    ext2_mkdir(file, "funFF", 0xFFFF);

    vfs_mkdir("/hiDan/inDan_1", 0);
    vfs_mkdir("/hiDan/inDan_1/inDan_2", 0);
    vfs_mkdir("/hiDan/inDan_1/inDan_2/inDan_3", 0);
    vfs_mkdir("/hiDan/inDan_1/inDan_2/inDan_3/inDan_4", 0);
    vfs_mkdir("/hiDan/inDan_1/inDan_2/inDan_3/inDan_4/inDan_5", 0);
    vfs_mkfile("/hiDan/inDan_1/inDan_2/inDan_3/inDan_4/inDan_5/HI_PARF!", 0);

    vfs_mkdir("/funFF/fun_1", 0xAAAA);
    vfs_mkdir("/funFF/fun_1/fun_2", 0xAAAA);
    vfs_mkdir("/funFF/fun_1/fun_2/fun_3", 0xAAAA);
    vfs_mkdir("/funFF/fun_1/fun_2/fun_3/fun_4", 0xAAAA);
    vfs_mkdir("/funFF/fun_1/fun_2/fun_3/fun_4/fun_5", 0xAAAA);
    vfs_mkfile("/funFF/fun_1/fun_2/fun_3/fun_4/fun_5/HI WORLD!", 0);

    vfs_node_s *hi = file_open("/hiDan/inDan_1/inDan_2/inDan_3/inDan_4/inDan_5/HI_PARF!", 0);
    monitor_str_write("addr 'hi':");
    write_hex((u32int)hi);
    vfs_node_s *hi_1 = file_open("/funFF/fun_1/fun_2/fun_3/fun_4/fun_5/HI WORLD!", 0);
    monitor_str_write("/addr 'hi_1':");
    write_hex((u32int)hi_1);
    monitor_str_write("\nopen files... (");
    monitor_str_write(hi->name);
    monitor_str_write(") OK / (");
    monitor_str_write(hi_1->name);
    monitor_str_write(") OK\n");

    kfree(hi);
    kfree(hi_1);
    return;
}

void ext2_test_pr(u32int idx_inode, u32int block) {
    monitor_str_write("get block(");
    write_dec(block);
    monitor_str_write(") from inode(");
    write_dec(idx_inode);
    monitor_str_write(")\n");
    return;
}
//-------------------------------------------------------------------------------------