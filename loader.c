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
			
			if ((fault_addr < start_address) || (fault_addr >= (void*)(uintptr_t)(phdr[i].p_vaddr + phdr[i].p_memsz))){
				continue;
			}
			
			printf("start_addr: %p\n", start_address);
			printf("fault_addr: %p\n", fault_addr);
        	printf("end_addr: %p\n", (void*)(uintptr_t)(phdr[i].p_vaddr + phdr[i].p_memsz));
        	
        	int pages_for_segment = 0;
        	for(int i = 0; i < phdr[i].p_memsz; i += PAGE){
        		pages_for_segment++; 
        	}//calculates number of pages needed for the entire segment
        	
        	int page_index = ((uintptr_t)fault_addr - ((uintptr_t)phdr[i].p_vaddr)) / PAGE;
        	//index value of the page with segfault-ing address
        	
        	off_t offset = (off_t)phdr[i].p_offset + page_index * PAGE;
        	
        	void* page = mmap(start_address + page_index * PAGE, PAGE, PROT_READ|PROT_WRITE|PROT_EXEC , MAP_ANONYMOUS|MAP_PRIVATE , fd, offset);
        	
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
        	
        	//add_to_History(page);
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
  
  
  
  char* h_memory;
  //h_memory = (char *)malloc(lseek(fd, 0, SEEK_END)); 

  
  off_t size_to_reserve = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET); //offset becomes 0 for future of execution

  //printf("size_to_reserve has value : %ld\n", size_to_reserve);

  h_memory = (char *)malloc(size_to_reserve); //char* maybe? ======
  

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
  
  /*
  if (ehdr->e_type != ET_EXEC)
  {
    printf("Unsupported elf file, check elf file again maybe?\n"); //problem with ELF file... idk what to do here @Akshat pls check
    free(h_memory);
    exit(1);
  }
  */
  
  read(fd, ehdr, sizeof(Elf32_Ehdr));
  
  lseek(fd, ehdr->e_phoff, SEEK_SET);
  phdr = (Elf32_Phdr *)malloc(ehdr->e_phnum * sizeof(Elf32_Phdr));
  read(fd, phdr, ehdr->e_phnum * sizeof(Elf32_Phdr));

  
  struct sigaction signal;
  signal.sa_sigaction = segmentation_fault_handler;
  signal.sa_flags = SA_SIGINFO;
  
  int (*_start)(void) = (int (*)(void)) ehdr->e_entry;
  int result = _start();
  printf("User _start return value = %d\n",result);

  
  printf("****************************RESULTS****************************\n \n");
  printf("Total number of page faults = %d \n" , total_number_of_page_faults);
  printf("Total number of pages allocated = %d \n" , total_number_of_pages_allocated);
  printf("Total KBs of internal fragmentation = %d \n" , memory_wasted);
  printf("***************************************************************\n \n");
  
  loader_cleanup();
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Usage: %s <ELF Executable> \n", argv[0]);
    exit(1);
  }
  
  int file_descriptor = open(argv[1], O_RDONLY);
    if (file_descriptor < 0) {
        printf("Unable to open ELF file.\n");
        exit(1);
    }
    close(file_descriptor);

  load_and_run_elf(argv);
  loader_cleanup();
  return 0;
}
