#invoke make inside following directories and in this order: loader, launch, fib
all : 
	cd loader && $(MAKE)
	cd launcher && $(MAKE)
	cd test && $(MAKE)
	make install
	
	clear #remove this if you don't want a clean terminal while running

	cd bin && ./launch ../test/fib
	
#move the lib_simpleloader.so and launch binaries inside bin directory
install:
	
	mkdir -p bin
	
	cp launcher/launch bin/
	cp loader/lib_simpleloader.so bin/
	
	
#Provide the command for cleanup
clean:
	rm -rf bin
	cd loader && $(MAKE) clean
	cd launcher && $(MAKE) clean
	cd test && $(MAKE) clean
	mkdir -p bin
	clear #remove this if you don't want a clean terminal while running
	
