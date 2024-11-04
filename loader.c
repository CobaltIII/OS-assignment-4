#include "loader.h"
#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <elf.h>
#include <setjmp.h>
#include <errno.h>
#include <stdint.h>

#define PAGE 4096

Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd;

int total_number_of_page_faults = 0;
int total_number_of_pages_allocated = 0;
int memory_wasted = 0;

typedef struct node{
	struct node* next;
	void* content;
} node;

typedef struct{
	node* head;
} linked_list;

linked_list Pages_History;

void initialize_History(){
	Pages_History.head = NULL;
}

void add_to_History(void* Content){
	node* new_node = (node*)malloc(sizeof(node));
	if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
	new_node->content = Content;
	new_node->next = NULL;
	if (Pages_History.head == NULL){
		Pages_History.head = new_node; 
	}
	else{
		node* current = Pages_History.head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
	}
}

void print_History() {
    node* current = Pages_History.head;
    int index = 0;
    while (current != NULL) {
        printf("%d:   %p\n", index++, current->content);
        current = current->next;
    }
}

typedef struct {
    uintptr_t start_addr;
    uintptr_t end_addr;
    void *mem_ptr;
    off_t file_offset;
} segment_data;

void segmentation_fault_handler(int signum, siginfo_t* information, void* content){
	
	if (signum == SIGSEGV){
	
		total_number_of_page_faults++;
		total_number_of_pages_allocated++;
		
		void* fault_addr = information->si_addr;
		//address that caused the fault
		
		for (int i = 0; i < ehdr->e_phnum; i++){
			void* start_address = (void*)(uintptr_t)phdr[i].p_vaddr;
			
			if ((fault_addr < (void*)(uintptr_t)(phdr[i].p_vaddr)) || (fault_addr >= (void*)(uintptr_t)(phdr[i].p_vaddr + phdr[i].p_memsz))){
				continue;
			}
			
			printf("start_addr: %p\n", (void*)(uintptr_t)phdr[i].p_vaddr);
			printf("fault_addr: %p\n", fault_addr);
        	printf("end_addr: %p\n", (void*)(uintptr_t)(phdr[i].p_vaddr + phdr[i].p_memsz));
        	
        	int pages_for_segment = 0;
        	for(int i = 0; i < phdr[i].p_memsz; i += PAGE){
        		pages_for_segment++; 
        	}//calculates number of pages needed for the entire segment
        	
        	int page_index = ((uintptr_t)fault_addr - ((uintptr_t)phdr[i].p_vaddr)) / PAGE;
        	//index value of the page with segfault-ing address
        	
        	off_t offset = (off_t)phdr[i].p_offset + page_index * PAGE;
        	
        	void* page = mmap((void*)(uintptr_t)phdr[i].p_vaddr + page_index * PAGE, PAGE, PROT_READ|PROT_WRITE|PROT_EXEC , MAP_ANONYMOUS|MAP_PRIVATE , fd, offset);
        	
        	if (page == MAP_FAILED){
        		printf("Error: mmap fail, check line - 110\n"); //check line 110, couldn't do mmap
    			exit(1);
        	}
        	
        	if (lseek(fd, offset , SEEK_SET) == -1){
        		printf("Error: error at lseek, check line - 110\n"); //check line 119, couldn't do lseek
    			exit(1);
        	}
        	
        	//reading all the remainder of the page
        	int all_data = phdr[i].p_memsz - (page_index * PAGE);
        	int data_on_page;
        	if (all_data < PAGE){
        		data_on_page = all_data; //FRAGMENTATION CASE
        		memory_wasted = memory_wasted + (PAGE - all_data);
        	}
        	else{
        		data_on_page = PAGE;
        	}
        	
        	if(read(fd, page, data_on_page) == -1){
        		printf("Error: error at while reading page data, check line - 110\n"); //check line 135
    			exit(1);
        	}
        	
        	add_to_History(page);
        	return;
        	
		}
		
	}
	
}

/*
 * release memory and other cleanups
 */
void loader_cleanup()
{
  // freeing the memory for globally defined variables
  if (ehdr){
  	ehdr = NULL;
  	free(ehdr);
  }
  
  if (phdr){
  	phdr = NULL;
  	free(phdr);
  }
  
  if (fd < 0){
  	close(fd);
  }
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** argv)
{

  // 1. Load entire binary content into the memory from the ELF file.
  fd = open(*argv , O_RDONLY);
  /*
  char* h_memory;
  h_memory = (char *)malloc(lseek(fd, 0, SEEK_END)); 

  
  off_t size_to_reserve = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET); //offset becomes 0 for future of execution

  //printf("size_to_reserve has value : %ld\n", size_to_reserve);


  //char* h_memory;
  //h_memory = (char *)malloc(size_to_reserve); //char* maybe? ======
  

  if (h_memory == 0){
    printf("Error: memory reservation, check line - 34\n"); //check line 34, couldn't reserve memory
    exit(1);
  }
  
  ssize_t load_file = read(fd, h_memory, size_to_reserve);
  
  if ((size_t)load_file != size_to_reserve){
  	perror("Error: problem in File read operation, check line 42\n"); //reading memory problem, check line 42
  	free(h_memory);
  	exit(1);
  }

  if (load_file < 0)
  {
    perror("Error: File read operation failed, check line 42\n"); //couldn't read memory properly, check line 42
    free(h_memory);
    exit(1);
  }
  
  ehdr = (Elf32_Ehdr *)h_memory;
  
  if (ehdr->e_type != ET_EXEC)
  {
    printf("Unsupported elf file, check elf file again maybe?\n"); //problem with ELF file... idk what to do here @Akshat pls check
    free(h_memory);
    exit(1);
  }
  
  phdr = (Elf32_Phdr *)(h_memory + (*ehdr).e_phoff);
  unsigned int e_entry = (*ehdr).e_entry;
  void *entrypoint;
  void *v_memory;
  
  //printf("break 2");
  
  
  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c
  
  for (int i = 0; i < (*ehdr).e_phnum; i++)
  {
    //printf("value of i = %d\n", i); //indexing of the phdr table
    if (phdr[i].p_type != PT_LOAD){
      continue;
    }
    
    else{
      // 3. Allocate memory of the size "p_memsz" using mmap function 
      //    and then copy the segment content
      v_memory = mmap(NULL, phdr[i].p_memsz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
      memcpy(v_memory, h_memory + phdr[i].p_offset, phdr[i].p_memsz); //i think this is to copy the memory segment?
      if (v_memory == MAP_FAILED)
      {
        printf("Error: Memory mapping failed\n"); //the mmap function bhai yahan kya karna hai-
        exit(1);
      }
      
      // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
      entrypoint = v_memory + (e_entry - phdr[i].p_vaddr);
      if ((entrypoint >= v_memory) && (v_memory + phdr[i].p_offset >= entrypoint)){
        break;
      }
    }
    
  }
  
  //printf("%p,%p",v_memory,entrypoint);
  
  if (entrypoint != NULL){
    //5. Typecast the address to that of function pointer matching "_start" method in fib.c.
    // 6. Call the "_start" method and print the value returned from the "_start"
    int (*_start)(void) = (int (*)(void)) entrypoint;
    int result = _start();
    printf("User _start return value = %d\n",result);
  }*/
  
  ehdr = mmap(NULL, sizeof(Elf32_Ehdr), PROT_READ, MAP_PRIVATE, fd, 0);
  phdr = (Elf32_Phdr*)((char*)ehdr + ehdr->e_phoff);
  int (*_start)() = (int (*)())(uintptr_t)(ehdr->e_entry);
  printf("HEREEEEEEEEEEEEE\n");
  //loader_cleanup();
  //close(fd);
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Usage: %s <ELF Executable> \n", argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file

  // checking if ELF file exists in the provided path
  
  int file_descriptor = open(argv[1], O_RDONLY);
    // Check if the file exists and can be opened
    if (file_descriptor < 0) {
        printf("Unable to open ELF file.\n");
        exit(1);
    }
    close(file_descriptor);
    
    struct sigaction signal;
    memset(&signal, 0 , sizeof(signal));
    signal.sa_sigaction = segmentation_fault_handler;
    signal.sa_flags = SA_SIGINFO; //INFO OF SEGFAULTING ADDRESS
    
    if (sigaction(SIGSEGV, &signal, NULL) == -1) {
    	perror("Error in SIGSEGV handler");
  }


  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(&argv[1]);
  
  printf("****************************RESULTS****************************\n \n");
  printf("Total number of page faults = %d \n" , total_number_of_page_faults);
  printf("Total number of pages allocated = %d \n" , total_number_of_pages_allocated);
  printf("Total KBs of internal fragmentation = %d \n" , memory_wasted);
  printf("***************************************************************\n \n");
  
  
  // 3. invoke the cleanup routine inside the loader
  loader_cleanup();

  return 0;
}
