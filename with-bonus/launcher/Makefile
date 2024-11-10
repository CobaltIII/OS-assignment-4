#Compile the launch.c by linking it with the lib_simpleloader.so
all:
	gcc -m32 -c launch.c
	gcc -m32 -o launch launch.c -L../loader -l_simpleloader -Wl,-rpath,'$$ORIGIN/../loader'
	# -L../bin tells you to look in the ../bin directory
	# -l_simpleloader links against the simpleloader library=> it finds lib_simpleloader.so
	# -Wl,-rpath tells the linker to search this path at runtime for the shared libraries 
	# "$$ORIGIN/../bin" where $$ORIGIN == the directory of the executable
	
#Provide the command for cleanup
clean:
	-@rm -f launch.o launch
