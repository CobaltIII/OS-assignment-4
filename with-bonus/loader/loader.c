#include "loader.h"

Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd;

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
  }
  
  loader_cleanup();
  close(fd);
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

  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(&argv[1]);

  // 3. invoke the cleanup routine inside the loader
  loader_cleanup();

  return 0;
}
