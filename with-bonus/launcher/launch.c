#include "../loader/loader.h"

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
    
    setup();

  load_and_run_elf(argv);
  loader_cleanup();
  return 0;
}
