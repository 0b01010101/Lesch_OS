
OUTPUT(kernel.tmp)

ENTRY(_go)
STARTUP(kernel.o)

INPUT(main.o tab_descr_asm.o isr_asm.o switch_task.o user.o syscall.o bios_asm.o)



SECTIONS
 {
   . = 0x200000;
   
   .text : { *(.text) }
   .rdata : { *(.rdata) }
   .data : { *(.data) }  
  
   .bss : {*(.bss)
			}
 }