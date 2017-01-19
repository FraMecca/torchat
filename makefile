ALL = src/main.c include/mongoose.c src/socks_helper.c src/util.c src/list.c src/actions.c
LDIR := $(PWD)
DEBUG = -Wall -DDEBUG -Wextra
CF = -DMG_ENABLE_THREADS
I = -I. -Iinclude -Ilib


default: init build/logger.o build/jsonhelper.o build/main

init:
	echo $(LDIR)
	mkdir -p build

build/logger.o: src/logger.cpp
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl 	-o build/logger.o $I

build/jsonhelper.o: src/jsonhelper.cpp
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11 -o build/jsonhelper.o $I

build/main: build/logger.o build/jsonhelper.o $(ALL)
	g++ -shared -o build/liblogger.so build/logger.o $I
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o
	gcc -L$(LDIR)/build $(ALL) $I -ljsonhelper  -lpthread -llogger -ldl -o build/main -Wl,-R$(LDIR)/build $(CF)

asan:
	mkdir -p build
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o	
	g++ -shared -o build/liblogger.so build/logger.o $I
	g++ -c -Wall -fPIC src/jsonhelper.cpp -std=c++11 -g -o build/jsonhelper.o -fsanitize=address $I
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o  -g -fsanitize=address
	gcc -L$(LDIR)/build $(ALL) $I -g  -ljsonhelper -o build/main -lpthread -fsanitize=address -Wl,-R$(LDIR)/build $(CF)

debug:
	mkdir -p build
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o -g $(DEBUG) $I
	g++ -shared -o build/liblogger.so build/logger.o  -g $(DEBUG) $I
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11 -g -o build/jsonhelper.o $(DEBUG) $I
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o  -g $(DEBUG) $I
	gcc -L$(LDIR)/build $(ALL) $I -g  -ljsonhelper -o build/main -lpthread -llogger -ldl -DDEBUG -Wl,-R$(LDIR)/build $(DEBUG) $(CF)

etrace:
	mkdir -p build
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o -g $(DEBUG)
	g++ -shared -o build/liblogger.so build/logger.o  -g $(DEBUG) $I
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11 -g -o build/jsonhelper.o $(DEBUG) $I
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o  -g $(DEBUG) $I
	gcc -L$(LDIR)/build $(ALL) utils/ptrace.c $I -g  -ljsonhelper -o build/main -lpthread -llogger -ldl -DDEBUG -Wl,-R$(LDIR)/build $(DEBUG) $(CF) -finstrument-functions

clean:
	rm -rf build/
