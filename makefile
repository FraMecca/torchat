ALL = src/main.c include/mongoose.c src/socks_helper.c src/util.c src/list.c
LDIR := $(PWD)


default: init build/logger.o build/jsonhelper.o build/main

init:
	echo $(LDIR)
	mkdir -p build
	echo 'Remember to execute export ( DOLLAR ) LD_LIBRARY_PATH= ( DOLLAR ) (pwd)/build'

build/logger.o: src/logger.cpp
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl 	-o build/logger.o

build/jsonhelper.o: src/jsonhelper.cpp
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11 -o build/jsonhelper.o #<-- removed the -g flag since this is not debug

build/main: build/logger.o build/jsonhelper.o $(ALL)
	g++ -shared -o build/liblogger.so build/logger.o
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o
	gcc -L$(LDIR)/build $(ALL) -I. -ljsonhelper  -lpthread -llogger -ldl -o build/main 

asan:
	mkdir -p build
	echo 'Remember to execute export ( DOLLAR ) LD_LIBRARY_PATH= (pwd)/build'
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o	
	g++ -shared -o build/liblogger.so build/logger.o
	g++ -c -Wall -fPIC src/jsonhelper.cpp -std=c++11 -g -o build/jsonhelper.o -fsanitize=address
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o  -g -fsanitize=address
	gcc -L$(LDIR)/build $(ALL) -I. -g  -ljsonhelper -o build/main -lpthread -fsanitize=address

debug:
	mkdir -p build
	echo 'Remember to execute export ( DOLLAR ) LD_LIBRARY_PATH= (pwd)/build'
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o -g -DDEBUG
	g++ -shared -o build/liblogger.so build/logger.o  -g -DDEBUG
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11 -g -o build/jsonhelper.o -DDEBUG
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o  -g -DDEBUG
	gcc -L$(LDIR)/build $(ALL) -I. -g  -ljsonhelper -o build/main -lpthread -llogger -ldl -DDEBUG 
