                                                                "Lesch_OS"
*A simple educational operating system.
The operating system works with TCP/IP protocols (and with several application layer protocols), ext2 file system, user applications in multithreaded mode 
(it also creates processes). Tested, debugged and run only in Bochs. "Simple" and "educational" primarily because of the old hardware (drivers for it), 
memory allocation managers and threads (simply removes the "next" from the list). "Sleeping" streams are removed from the list), in principle, if you pump 
them, it will be very good, although everything works that way. It is loaded from a 1.44 Mb floppy disk.
================================================================================================================================================================
$Loader // consists of: boot.asm, kern_boot.asm, kernel.asm.
#(boot.asm)The bios loads the code from our floppy disk into RAM at 0x7C00. We are installing the stack 
->call 3function 10preservation (clean the screen and set text mode 80x25)->display the disk id via the "BIOS_printf" function
-> copy (read)code from ROM (starting from 2 sectors and reading 61 sectors in total) in RAM at the address "kernel_buffer" 
-> install the data segment and make a long-distance transition to "kernel_buffer".
	#(kern_boot.asm)we do a small check and if we didn't pass it, then we output a message and loop 
-> create a memory card (500b in size) and data about it in the mboot_struct structure, which we will then transfer to the kernel
-> switch to protected mode (open the "A20" line->disable non-masked interrupts->load the gdt_str address (a structure with the gdt address and its size)
->set the bit to cr0->make a long-distance transition to 0x08:Kern_32)
->set the segment registers to 0x10->output the message 
->we unload the main code (remaining) of the kernel from ROM to RAM at the address of 2mb in parts through the buffer "kernel_buffer", switching from 
protected mode to rail mode
->load the address of the "mboot_struct" structure into ebx, jump into the kernel (KERN_OFFSET(0x2mb)), into the _go function in the kernel.asm
#(kernel.asm) file, set esp to 2mb - 4 (slightly lower than we are now)->put the stack address (esp) on the stack and "mboot_struct" (ebx)->jump into 
the main kernel function "_kernel" in the main.c file
->exiting the kernel, go into sleep mode (hlt) and get stuck.
================================================================================================================================================================
$The table of descriptors and interrupts In the KERNEL (not the one in the loader) // consists of: table_descript.c, tab_descr_asm.asm, asm_asm.asm, isr.c, isr_handl, syscall.c
#(#(tables_descript.c)form gdt and tss->load gdt structures and tss to special registers(asm:lgdt/gdtr)
->we reassign interrupt numbers through the PIC ports of the controller
->->form the interrupt table->load the idt structure into a special register (asm:list)
->->register interrupt handlers (write function addresses in an array (index == interrupt number))
->register system call handler (register as interrupt handler 0x30)
================================================================================================================================================================
$Memory // consists of: memory.c
#(memory.c)iterate over the memory card in blocks from "mboot_struct" and write to the global variable "size_phymemory" the size of the entire memory
in bytes "check_memory_map(memory_map_s *mmap_addr, u32int len)"
->in "init_memory_manager()" we turn on the page memory addressing "on_page_mode()"(create a directory of tables (table of tables) by physical and virtual
address(5mb), and the page tables themselves are located immediately behind the directory of tables->display physical memory to virtual from 0x00 
to 0x1200000(physical == virtual)->write the physical address of the directory (5mb) in cr3 and slightly change cr0
->iterate over the memory card and mark free blocks with an address greater than 1mb as free using the function "free_phys_pages(physaddr base, u32int count)"
and set all available memory to the variable "memory_size"
->configure the kernel heap(display pages for the service information of the heap "phys2virt()"->display pages"phys2virt()" for a bunch of cores(which we 
will allocate with functions such as:kmalloc(), kcalloc()...)
->fill in the "heap_s" structure of the kernel.

	(all addresses below are virtual, although they should correspond to physical ones)
*the kernel code is located at 0x200000 to 0x4FFFFF
*directory of tables and page tables at 0x500000 - 0x8FFFFF
*core heap size = 0x2000000
*memory size for information about heap objects (aka "displaying pages for heap service information "phys2virt()"") = 0x400000
*the service information about the kernel heap is located at 0x1000000
*the address of the beginning of the kernel heap is located at 0x7FFFFFFF - 2000000
================================================================================================================================================================
$Processes and threads // consists of: schedule.c, process.c, shell.c, elf_loader.c, switch_task.asm, user.asm
#(schedule.c) in "init_task_manager()" prohibit interrupts and switching between threads with the "stop" function (just increases the semaphore 
counter"multi_task")
->creating lists: proc_list, thread_list, thread_wait, tty16_list(for GUI/text output), msg_list, mpfile_list(shared memory list (something like 
named channels))
->configure USER HEAP in the kernel (we display pages for the service information of the heap "phys2virt()"->display pages "phys2virt()" under USER HEAP 
in the kernel (which we will allocate with functions such as:umalloc(), ucalloc()...)
->fill in the structure "heap_s" USER HEAP in the core
->create a process and a kernel thread ->add them to the corresponding lists (proc_list/thread_list)
->allow switching between threads with the "start()" function (reduces the "multi_task" semaphore counter) and allow 
interrupts (masked)
*service information about USER HEAP blocks is always located at 0x80000000
*the allocated memory for the USER HEAP heap is located at 0x80010000
*size for service information about USER HEAP blocks = 0x10000
*size for memory under the USER HEAP heap = 0x100000
--------------------------------------------------------------------------------------------------------------------------------------------------------------
@Creating executable processes:
#(process.c)call the function "exec_proc(u8int *name, bool kern, u8int *cmd_line)" ->load the elf file from the disk by its name and fill in the structure
"elf_sections_s"(function "elf_load(name)")
->allocate free pages from the kernel and we display the physical address to the desired (take from "elf_sections_s")
virtual address 
->create a copy of the directory of the kernel process tables ->mark in the directory the table descriptors responsible for the USER_HEAP address of the 
application code as NO PRESENT(0 bit to 0)
->create a process and a thread
->forming a stack of a new thread
->then, when the kernel switched to our new thread, the elf_start function is called from the stack (we previously put the address to the "elf_start" 
function and the necessary parameters into it)"
->we read the application code from ROM to the necessary virtual addresses (specified in "elf_sections_s")
->creating a user stack(before that, we formed the thread stack in kernel mode, and now we are preparing a new stack for switching from kernel mode to user mode
->filling in the heap_s structure for USER_HEAP
->switch to user mode by calling the "switch_user_mode()" function by passing parameters:the entry point to the program and our new stack
----------------------------------------------------------------------------------------------------------------------------------------------------------------
@Shell:
#(shell.c) with the "shell_init(void)" function, we block masked interrupts->prohibit switching between threads(the "stop()" function)->create a process and
a thread(the "proc_create" function, which works almost the same as "exec_proc())
->select from open the free pages and display the physical address to the desired (0x08049000) virtual address 
->we do the same for the application stack, only the virtual address is different(SHELL_VADDR(0x08049000) + the size of the application code)
->we form the thread stack (we put the address of our new process in it, the address for the "shell_start" function and flags
->allow switching between threads and masked interrupts
->exit the "shell_init(void)" function and wait for switching to our new thread and process
->enter the "shell_start" function
->form the user stack(before that, we formed the thread stack in kernel mode, and now we are preparing a new stack for switching from kernel mode to user mode
->filling in the heap_s structure for USER_HEAP
->copy the application code from RAM to RAM, but to the desired virtual address (0x08049000)
->switch to user mode by calling the "switch_user_mode()" function by passing the parameters:the entry point to the program and our new stack

*the shell application intercepts keyboard keystrokes through the system call, puts the key values (the system call immediately converts them to asc2) in 
the buffer, if there is a key '\n', then the system call "shell_handler(u8int *shell_buff)" is called. 
*In shell_handler():
->prohibit switching between processes
->create a new buffer from the kernel heap (in order to pass parameters to another process) and copy data from the input buffer into it ("shell_buff")
->calls the "exec_proc" function with the necessary flag.
If there is an "exit" in shell_buff, then !!!!the process is destroyed by the function "proc_destroy()" THIS function terminates any process!!!!!!
-> we release the previously created buffer from the kernel heap by the function "kfree"
->allow switching between processes and exit
================================================================================================================================================================
$IPC // consists of: ipc.c, socket.c
#(ipc.c)calling the function (usually as a system call) "mpfile_set(u8int *name, u32int *vaddr)", we create the structure "mpfile_s"->add it to the 
list(mpfile_list)->fill in the fields "name","pid_src"(id of the current process) and paddr(physical address of the input parameter"vaddr")->exit.
 !!! the displayed memory NEEDS TO BE ALLOCATED ALIGNED to one page (0x1000bytes) by the "umalloc_align" function, naturally via syscall!!!
->from another process, we call the function (syscall) "mpfile_get(u8int *name, u32int *vaddr)"->we search in the list"mpfile_list" for the 
structure"mpfile_s" with name == input parameter"name"-> if found, we display it to the desired virtual address(this is the input parameter"vaddr") physical address from the field "*mpfie_s->paddr"
->done!->exit.

	#(socket.c)calling the "socket(u8int space_addr, u8int sock_type, u8int protocol)" function, we look for a free socket in the "socket_pool" array and 
fill in its fields (from the interesting: we enter the virtual address of the current thread in the "socket.thread" field)->if successful, we return 
the ordinal socket number in the array (socket id), otherwise return 0 (I agree, the function can return 0 even if successful (if the index in the 
array is == 0, but this is necessary);
->by calling the function "sock_listen(u16int id, u16int max_ps)", we check whether the port already exists in the kernel (bound or not), which we specified
as the source in the socket with the desired id(input parameter) and if it does not exist, then we reserve it. Reserved ports are entered in the 
"ports_binded" array as part of the "bind_port_s" structure (in net.h).
->sock_close(u16int id)- we clear the socket in the "socket_pool" array, and the port structure in the "ports_binded" array.
->sock_bind(u16int id, void *ip_src, u16int port_src) - just add additional parameters to the socket (a little useless function).
->sock_connect(u16int id, void *ip_src, u16int port_src, void *ip_dst, u16int port_dst) - establish a connection
(send a connection request to the desired node).
->sock_accept(u16int id) - waits in the connection cycle to the desired port (it doesn't seem to have been used)
->sock_recv(socket_s *sock, u8int *recv_buff) - adds a buffer(recv_buff) to the socket(sock) to receive bytes and sends the stream to the "sleep" standby 
mode with the "slp_thrd" function when the TCP manager loads bytes into recv_buff(switches cr3 to the desired directory of page tables 
"socket->thread->proc->page_dir" copies bytes and returns cr3 to its previous state) the function will return the number of bytes received.
->sock_send(socket_s *sock, u8int *data, u32int size, u8int flag) - copies data from data to a buffer created from the kernel heap, sends them as a tcp 
packet via the "tcp_xmit" function with the necessary flags -> returns the number of bytes sent.
->ssock_send(socket_s *sock, u8int flags) - the scheme of operation is identical to sock_send(), you just need to manually fill in the fields 
"sock->send_bytes" (number of bytes to be sent) and "sock->buf_send" (buffer with data to be sent). IT IS USED FOR SYSCALL, due to the lack of processor 
registers for transmitting parameters.

	#(msg.c) there are still "messages", BUT they have not been tested and are not written!!!!!!!(they are not used anywhere in the code).
==============================================================================================================================================================
$NET // consists of: net.c, rtl8029.c, socket.c
   #(net.c)Receiving the packet: an interrupt is triggered and the driver(rtl8029.c) is running in the current stream->from the network card driver(rtl8029.c)
we get the buffer and its size in the function "Ethernet_Recv(ether_s *eth, u32int len)" 
->pass it to "Arp_Recv(arp_s *arp_pack, u32int len)" or to "Ip_Recv(ip_s *pack, u32int len)" (the "len" parameter is the packet size from the same buffer 
without the size of the ethernet header, data is the same buffer from the driver, but without the ethernet header)
->from the "Ip_Recv" function, we pass the packet to "Udp_Recv(ip_s *ip, u32int len)" or to "Tcp_Recv(ip_s *ip, u32int len)"
ATTENTION!!! we transmit a packet with the header "ip_s", BUT the length is considered without it!!!
->in the functions "Udp_Recv()" and "Tcp_Recv()", the search for the desired socket goes on and on. When it comes to sending a packet to the application 
buffer, the functions "udp_to_app" and "tcp_to_app" are called, which, in order to copy the received packet to the application buffer, switch cr3 to the 
table directory of the desired process (socket->thread->proc->page_dir)->copy->switch cr3 back.

	Sending a packet: we call sock_send()/ssock_send() (Work only for TCP) and in it we create a new buffer from the kernel heap, into which we copy the data 
being sent and transfer the buffer to the next level of Internet protocols.
!!!At each new level, a new buffer is created and data is copied into it (taking into account the offset of the new packet header), yes, it is better to 
rewrite without creating new buffers, immediately allocating the desired buffer size taking into account the length of all protocol headers.

	*Protocols are also implemented: Tftp, Ftp, Dhcp (something is wrong with it)
==============================================================================================================================================================
$File system // consists of: vfs.c, ext2.c, ata.c
virtual file system(vfs.c) - interface for working with ext2.c. ata.c - driver for communicating with hd.
==============================================================================================================================================================
$Драйвера // pci.c, rtl8029.c rtl8139.c(не тестировался), ata.c, keyboard.c, timer.c, bios.c(переход в реальный режим и вызов нужных функций биос), vesa.c 



































