#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  free(ehdr);
  free(phdr);
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
  fd = open(argv[1], O_RDONLY);
  //printf("File descriptor (should maybe be 3): %d\n",fd);

  off_t size_to_reserve = lseek(fd, 0, SEEK_END);

  lseek(fd, 0, SEEK_SET) //offset becomes 0 for future of execution
  //printf("size_to_reserve has value : %ld\n", size_to_reserve);

  int* reserved_memory = (int*)malloc(size_to_reserve); //char* maybe? ======

  if (!reserved_memory){
    perror('memory reservation problem maybe');
    exit(1);
  }

  ehdr = (Elf32_Ehdr*) reserved_memory;
  phdr = (Elf32_Phdr*) reserved_memory + ehdr.e_phoff;

  int entry = ehdr.e_entry;

  for (int i = 0; i < ehdr.e_phnum; i++)
  {
    Elf32_Phdr* sector = phdr + i;
    if (sector.p_type != PT_LOAD){
      continue;
    }
    else{
      virtual_mem = mmap(NULL, sector.p_memsz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
      memcpy(virtual_mem, heap_mem + sector.p_offset, sector.p_memsz);

      if ((entry > sector.p_vaddr) && (virtual_mem + sector.p_offset > entry)){
        break;
      }
    }
  }

  if (entry != NULL){
    int (*_start)(void) = (int (*)(void)) entry;
    int result = _start();
    printf("User _start return value = %d\n",result);
  }
  






  // 1. Load entire binary content into the memory from the ELF file.
  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c
  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  // 6. Call the "_start" method and print the value returned from the "_start"

  //int result = _start();
  //printf("User _start return value = %d\n",result);
}

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv[1]);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}
