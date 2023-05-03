#ifndef      ELF_H
#define      ELF_H

#include    "common.h"
#include    "memory.h"
#include    "vfs.h"

#define     EI_NIDENT      16

#define     ELFMAG0         0x7f    
#define     ELFMAG1         'E'
#define     ELFMAG2         'L'
#define     ELFMAG3         'F'
#define     ELFDATA2LSB     (1)     //Little Endian
#define     ELFCLASS32      (1)     //32-bit Architecture
#define     EV_CURRENT      (1)     //ELF Current Version

#define     SHF_WRITE      0x01
#define     SHF_ALLOC      0x02
#define     SHF_EXECINSTR  0x04
#define     SHF_MASKPROC   0xF0000000

//-----------  Types of object files----------------------------------
#define		ET_NONE			0x00			/* No file type */
#define		ET_REL			0x01			/* Relocatable file */
#define		ET_EXEC		    0x02			/* Executable file */
#define		ET_DYN			0x03			/* Shared object file */
#define	    ET_CORE			0x04			/* Core file */
#define		ET_LOPROC		0xFF00		    /* Processor-specific */
#define		ET_HIPROC		0xFFFF		    /* Processor-specific */

//------------ Types of architecture ---------------------------------
#define     EM_NONE         0x00
#define     EM_M32          0x01
#define     EM_SPARC        0x02
#define     EM_386          0x03
#define     EM_68K          0x04
#define     EM_88K          0x05
#define     EM_860          0x07
#define     EM_MIPS         0x08

//-------------- Data types ------------------------------------------
typedef     unsigned short int  Elf32_Half;
typedef     unsigned int        Elf32_Addr;
typedef     unsigned int        Elf32_Off;
typedef     unsigned int        Elf32_Word;
typedef     int                 Elf32_Sword;

typedef struct Elf32_Ehdr Elf32_Ehdr_s;
typedef struct Elf32_Phdr Elf32_Phdr_s;
typedef struct Elf32_Shdr Elf32_Shdr_s;
typedef struct elf_sections elf_sections_s;
//---------------------------------------------------------------------
enum Elf_Ident {
    EI_MAG0         = 0,
    EI_MAG1         = 1,
    EI_MAG2         = 2,
    EI_MAG3         = 3,
    EI_CLASS        = 4,
    EI_DATA         = 5,
    EI_VERSION      = 6,
    EI_OSABI        = 7,
    EI_ABIVERSION   = 8,
    EI_PAD          = 9
};
//--------------- ELF header ------------------------------------------
struct Elf32_Ehdr {

    u8int           e_ident[EI_NIDENT];     //ELF identification data
    Elf32_Half      e_type;                 //Type of object file
    Elf32_Half      e_mashine;              // Architecture type
    Elf32_Word      e_version;              //Object file version
    Elf32_Addr      e_entry;                //Entry point to process 
    Elf32_Off       e_phoff;                //Program header offset
    Elf32_Off       e_shoff;                //Section table header offset
    Elf32_Word      e_flags;
    Elf32_Half      e_ehsize;               //ELF header size
    Elf32_Half      e_phentsize;            //Size of program header entry
    Elf32_Half      e_phnum;                //Number of program header entries
    Elf32_Half      e_shentsize;            //Section header entry size
    Elf32_Half      e_shnum;                //Section header entries number
    Elf32_Half      e_shstrndx;             //index of ".shstrtab" section            
};
//---------------------------------------------------------------------
//--------------- Program header --------------------------------------
struct Elf32_Phdr {

    Elf32_Word      p_type;                 //Program header type
    Elf32_Off       p_offset;               //Program header offset
    Elf32_Addr      p_vaddr;                //Program header virtual address
    Elf32_Addr      p_paddr;                //
    Elf32_Word      p_filesize;             //Number bytes in file image segment
    Elf32_Word      p_memsz;                //Size of segment in memory
    Elf32_Word      p_flags;                //
    Elf32_Word      p_align;                //alignment requirements for a section in memory (if it contains values 0x00 or 0x01, then alignment is not required)
};
//---------------------------------------------------------------------
//--------------- Section header --------------------------------------
struct Elf32_Shdr {

    Elf32_Word      sh_name;                //Index into section header stirng table
    Elf32_Word      sh_type;                //Section type
    Elf32_Word      sh_flags;               //
    Elf32_Addr      sh_addr;                //Section virtual address in process image
    Elf32_Off       sh_offset;              //Section offset in file
    Elf32_Word      sh_size;                //Section size
    Elf32_Word      sh_link;                //contains the index (in the table of section headers) of the section that this section is associated with
    Elf32_Word      sh_info;                // Extra information
    Elf32_Word      sh_addralign;           //Section address alingment
    Elf32_Word      sh_entsize;             //Size of one specific struct in this section
};
//---------------------------------------------------------------------
//---------------- ELF_loader struct ----------------------------------
struct elf_sections {

    Elf32_Ehdr_s      *elf_header;
    Elf32_Shdr_s      *sect_header;
    Elf32_Phdr_s      *prog_header;
    vfs_node_s        *file;
};
//---------------------------------------------------------------------
u32int elf_valid(Elf32_Ehdr_s *elf_header);
elf_sections_s *elf_load(u8int *name);
void elf_close(elf_sections_s *elf_sections);

#endif