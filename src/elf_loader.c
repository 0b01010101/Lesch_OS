#include "elf_loader.h"

u32int elf_valid(Elf32_Ehdr_s *elf_head) {

    if(elf_head->e_ident[EI_MAG0] != ELFMAG0) return 0;
    if(elf_head->e_ident[EI_MAG1] != ELFMAG1) return 0;
    if(elf_head->e_ident[EI_MAG2] != ELFMAG2) return 0;
    if(elf_head->e_ident[EI_MAG3] != ELFMAG3) return 0;

    if(elf_head->e_ident[EI_CLASS] != ELFCLASS32) return 0;
    if(elf_head->e_ident[EI_DATA] != ELFDATA2LSB) return 0;
    if(elf_head->e_ident[EI_VERSION] != EV_CURRENT) return 0;

    if(elf_head->e_mashine != EM_386) return 0;
    if(elf_head->e_type != ET_REL && elf_head->e_type != ET_EXEC) return 0;
    
    return 1;
}

elf_sections_s *elf_load(u8int *name) {

    //Allocate ELF file structure
    elf_sections_s *elf = (elf_sections_s*)kcalloc(sizeof(elf_sections_s), 1);
    /* Open ELF file */
    //vfs_node_s *file_elf = 0;
    vfs_node_s *file_elf = file_open(name, 0);
    if(!file_elf) {
        wprint("ERR: file(%s) open\n", name);
        return NULL;
    }
    //Read ELF header
    elf->elf_header = (Elf32_Ehdr_s *)kcalloc(sizeof(Elf32_Ehdr_s), 1);
    vfs_read(file_elf, 0, sizeof(Elf32_Ehdr_s), (u8int *)elf->elf_header);

    if(!elf_valid(elf->elf_header)) {
        monitor_str_write("Invalid/Unsupported elf executable\n");
        return NULL;
    }

    //Read program header
    Elf32_Half proc_entr = elf->elf_header->e_phnum;
    elf->prog_header = (Elf32_Phdr_s *)kcalloc(sizeof(Elf32_Phdr_s), proc_entr);
    vfs_read(file_elf, elf->elf_header->e_phoff, sizeof(Elf32_Ehdr_s) * proc_entr, (u8int *)elf->prog_header);

    //Read ELF sections
    Elf32_Half sect_entr = elf->elf_header->e_shnum;
    elf->sect_header = (Elf32_Shdr_s *)kcalloc(sizeof(Elf32_Shdr_s), sect_entr);
    vfs_read(file_elf, elf->elf_header->e_shoff, sizeof(Elf32_Shdr_s) * sect_entr, (u8int *)elf->sect_header);

    elf->file = file_elf;
    return elf;
}

void elf_close(elf_sections_s *elf) {

    kfree(elf->elf_header);
    kfree(elf->prog_header);
    kfree(elf->sect_header);
    kfree(elf->file);
    kfree(elf);

    return;
}

void elf_log(elf_sections_s *elf) {

    Elf32_Ehdr_s *h = elf->elf_header;
    wprint("+++++++++++++++++++++++++++ Log ELF Header +++++++++++++++++++++++++++++++++\n");
    wprint("mag[%x%x%x%x]|", elf->elf_header->e_ident[0], elf->elf_header->e_ident[1], elf->elf_header->e_ident[2], elf->elf_header->e_ident[3]);
    wprint("type[%d]|entry[%x]|phoff[%x]|shoff[%x]|\nphnum[%d]|shnum[%d]|esize[%d]|psize[%d]|ssize[%d]\n", h->e_type, h->e_entry, h->e_phoff, h->e_shoff, h->e_phnum, h->e_shnum, h->e_ehsize, h->e_phentsize, h->e_shentsize);
    wprint("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

    if(elf->elf_header->e_phnum) {
        Elf32_Phdr_s *p = elf->prog_header;
        //p++;
        wprint("+++++++++++++++++++++++++++ Log ELF Phdr +++++++++++++++++++++++++++++++++\n");
        wprint("type[%d]|off[%x]|vaddr[%x]|paddr[%x]|file_size[%d]|mem_size[%d]\n", p->p_type, p->p_offset, p->p_vaddr, p->p_paddr, p->p_filesize, p->p_memsz);
        wprint("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    }

    if(elf->elf_header->e_shnum) {
        Elf32_Shdr_s *s = elf->sect_header;
        s++;
        wprint("+++++++++++++++++++++++++++ Log ELF Shdr +++++++++++++++++++++++++++++++++\n");
        wprint("name[%d]|type[%d]|vddr[%d]|off[%x]|size[%d]|link[%d]\n", s->sh_name, s->sh_type, s->sh_addr, s->sh_offset, s->sh_size, s->sh_link);
        wprint("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    }
    return;
}