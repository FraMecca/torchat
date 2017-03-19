ALL = $(SRC) $(INCLUDE)  
SRC = src/main.c src/socks_helper.c src/util.c src/datastruct.c src/actions.c src/file_upload.c src/fileactions.c src/fileupload_datastructs.c
INCLUDE = include/mongoose.c include/mem.c include/ut_assert.c include/except.c include/base64.c
LDIR := $(PWD)
DEBUG = -Wall -Wextra -g -UNDEBUG
CF = -DMG_ENABLE_THREADS -DMG_ENABLE_HTTP_WEBSOCKET=0 -DMG_ENABLE_HTTP_STREAMING_MULTIPART
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
	gcc -L$(LDIR)/build $(ALL) -I. -ljsonhelper  -lpthread -llogger -ldl -o build/main -Wl,-R$(LDIR)/build  -NDEBUG
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -lcurl -o build/logger.o $I

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
	gcc -L$(LDIR)/build $(ALL) $I   -ljsonhelper -o build/main -lpthread -llogger -ldl -lcurl -DDEBUG -Wl,-R$(LDIR)/build $(DEBUG) $(CF)

etrace:
	mkdir -p build
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o  $(DEBUG) $I
	g++ -shared -o build/liblogger.so build/logger.o   $(DEBUG) $I
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11  -o build/jsonhelper.o $(DEBUG) $I
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o   $(DEBUG) $I
	gcc -L$(LDIR)/build $(ALL) utils/ptrace.c $I   -ljsonhelper -o build/main -lpthread -llogger -lcurl -ldl -DDEBUG -Wl,-R$(LDIR)/build $(DEBUG) $(CF) -finstrument-functions

clang:
	mkdir -p build
	clang -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o  $(DEBUG) $I
	clang -shared -o build/liblogger.so build/logger.o   $(DEBUG) $I
	clang -c -fPIC src/jsonhelper.cpp -std=c++11  -o build/jsonhelper.o $(DEBUG) $I
	clang -shared -o build/libjsonhelper.so build/jsonhelper.o   $(DEBUG) $I
	clang -L$(LDIR)/build $(ALL) $I   -ljsonhelper -o build/main -lpthread -llogger -ldl -lcurl -DDEBUG -Wl,-R$(LDIR)/build $(DEBUG) $(CF)

clean:
	rm -rf build/
