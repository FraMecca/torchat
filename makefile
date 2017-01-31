ALL = $(SRC) $(INCLUDE)
SRC = src/main.c src/socks_helper.c src/util.c src/list.c src/actions.c
INCLUDE = include/mongoose.c include/mem.c include/ut_assert.c include/except.c
LDIR := $(PWD)
ND = -DNDEBUG
DEBUG = -Wall -Wextra -DDEBUG -g
CF = -DMG_ENABLE_THREADS -DMG_ENABLE_HTTP_WEBSOCKET=0
I = -I. -Iinclude -Ilib


default: init build/logger.o build/jsonhelper.o build/main

init:
	echo $(LDIR)
	mkdir -p build

build/logger.o: src/logger.cpp
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl 	-o build/logger.o $(ND)

build/jsonhelper.o: src/jsonhelper.cpp
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11 -o build/jsonhelper.o  $(ND)

build/main: build/logger.o build/jsonhelper.o $(ALL)
	g++ -shared -o build/liblogger.so build/logger.o
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o
	gcc -L$(LDIR)/build $(ALL) -I. -ljsonhelper  -lpthread -llogger -ldl -o build/main -Wl,-R$(LDIR)/build $(ND)
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl 	-o build/logger.o $I

build/jsonhelper.o: src/jsonhelper.cpp
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11 -o build/jsonhelper.o  $I 

build/main: build/logger.o build/jsonhelper.o $(ALL)
	g++ -shared -o build/liblogger.so build/logger.o 
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o $I
	gcc -L$(LDIR)/build $(ALL) $I -ljsonhelper  -lpthread -llogger -ldl -o build/main -Wl,-R$(LDIR)/build $(CF)

asan:
	mkdir -p build
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o $I $(DEBUG)
	g++ -shared -o build/liblogger.so build/logger.o $I $(DEBUG)
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11  -o build/jsonhelper.o -fsanitize=address $I $(DEBUG)
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o   -fsanitize=address $I $(DEBUG)
	gcc -L$(LDIR)/build $(ALL) $I $(DEBUG)   -ljsonhelper -o build/main -lpthread -fsanitize=address -Wl,-R$(LDIR)/build $(CF)

debug:
	mkdir -p build
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o  $(DEBUG) $I
	g++ -shared -o build/liblogger.so build/logger.o   $(DEBUG) $I
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11  -o build/jsonhelper.o $(DEBUG) $I
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o   $(DEBUG) $I
	gcc -L$(LDIR)/build $(ALL) $I   -ljsonhelper -o build/main -lpthread -llogger -ldl -DDEBUG -Wl,-R$(LDIR)/build $(DEBUG) $(CF)

etrace:
	mkdir -p build
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o  $(DEBUG) $I
	g++ -shared -o build/liblogger.so build/logger.o   $(DEBUG) $I
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11  -o build/jsonhelper.o $(DEBUG) $I
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o   $(DEBUG) $I
	gcc -L$(LDIR)/build $(ALL) utils/ptrace.c $I   -ljsonhelper -o build/main -lpthread -llogger -ldl -DDEBUG -Wl,-R$(LDIR)/build $(DEBUG) $(CF) -finstrument-functions

clean:
	rm -rf build/
