#Create lib_simpleloader.so from loader.c
all:
	gcc -m32 -c loader.c
	gcc -m32 -o loader loader.c
	gcc -m32 -shared -fPIC -O3 -o lib_simpleloader.so loader.c
	#m-32 converts it all to 32 bit architecture stuff
	#shared and o is for creating a shared object file 
	#fPIC is for position independent code (works regardless of absolute memory address)
	#O3 just improves optimization levels

#Provide the command for cleanup
clean:
	rm -f loader.o loader lib_simpleloader.so 
