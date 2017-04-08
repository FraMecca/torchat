ALL = $(SRC) $(INCLUDE)  
SRC = src/main.c src/socks_helper.c src/util.c src/datastruct.c src/actions.c src/torchatproto.c
INCLUDE = include/mem.c include/ut_assert.c include/except.c include/base64.c  include/proxysocket.c include/fd.c include/iol.c include/utils.c
LDIR := $(PWD)
DEBUG = -Wall -Wextra -g -UNDEBUG
CF =  -Wall
I = -I. -Iinclude -Ilib 


default: init build/logger.o build/jsonhelper.o build/main

init:
	echo $(LDIR)
	mkdir -p build

build/logger.o: src/logger.cpp
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl 	-o build/logger.o -DNDEBUG $(I)

build/jsonhelper.o: src/jsonhelper.cpp
	echo $(CF)
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11 -o build/jsonhelper.o -DNDEBUG

build/main: build/logger.o build/jsonhelper.o $(ALL)
	g++ -shared -o build/liblogger.so build/logger.o
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o
	gcc -L$(LDIR)/build $(ALL) -I. -ljsonhelper  -lpthread -llogger -ldill -ldl -o build/main -Wl,-R$(LDIR)/build  #-NDEBUG
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl  -o build/logger.o $I

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
	gcc -L$(LDIR)/build $(ALL) $I   -ljsonhelper -o build/main -lpthread -llogger -ldl -ldill -DDEBUG -Wl,-R$(LDIR)/build $(DEBUG) $(CF) -O0

callgrind:
	echo "like make debug but without loguru"
	mkdir -p build
	#g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o  $(DEBUG) $I
	g++ -c -fPIC src/logger_dummy.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o  $(DEBUG) $I
	g++ -shared -o build/liblogger.so build/logger.o   $(DEBUG) $I
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11  -o build/jsonhelper.o $(DEBUG) $I
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o   $(DEBUG) $I
	gcc -L$(LDIR)/build $(ALL) $I   -ljsonhelper -o build/main -lpthread -llogger -ldl -ldill -DDEBUG -Wl,-R$(LDIR)/build $(DEBUG) $(CF) -O0


clang:
	mkdir -p build
	clang -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o  $(DEBUG) $I
	clang -shared -o build/liblogger.so build/logger.o   $(DEBUG) $I
	clang -c -fPIC src/jsonhelper.cpp -std=c++11  -o build/jsonhelper.o $(DEBUG) $I
	clang -shared -o build/libjsonhelper.so build/jsonhelper.o   $(DEBUG) $I
	clang -L$(LDIR)/build $(ALL) $I   -ljsonhelper -o build/main -lpthread -llogger -ldl -ldill -DDEBUG -Wl,-R$(LDIR)/build $(DEBUG) $(CF)

clean:
	rm -rf build/
