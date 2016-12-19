ALL = src/main.c include/mongoose.c src/socks_helper.c src/util.c src/list.c

default:
	mkdir -p build
	echo 'Remember to execute export ( DOLLAR ) LD_LIBRARY_PATH= current directory'
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o
	g++ -shared -o build/liblogger.so build/logger.o  
	g++ -c -Wall -fPIC src/jsonhelper.cpp -std=c++11 -g -o build/jsonhelper.o
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o  
	gcc -L/home/user/git_mio/torchat/build $(ALL) -I. -ljsonhelper  -lpthread -llogger -o build/main -ldl

clang:
	mkdir -p build
	cd build
	echo 'Remember to execute export ( DOLLAR ) LD_LIBRARY_PATH= current directory'
	g++ -c -Wall -fPIC src/jsonhelper.cpp -std=c++11 -g -o build/jsonhelper.o
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o  -g
	clang -L/home/user/git_mio/torchat $(ALL) -I. -g  -ljsonhelper -o main -lpthread

asan:
	mkdir -p build
	echo 'Remember to execute export ( DOLLAR ) LD_LIBRARY_PATH= current directory'
	g++ -c -Wall -fPIC src/jsonhelper.cpp -std=c++11 -g -o build/jsonhelper.o -fsanitize=address
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o  -g -fsanitize=address
	gcc -L/home/user/git_mio/torchat/build $(ALL) -I. -g  -ljsonhelper -o build/main -lpthread -fsanitize=address

debug:
	mkdir -p build
	echo 'Remember to execute export ( DOLLAR ) LD_LIBRARY_PATH= current directory'
	g++ -c -fPIC src/logger.cpp -std=c++11 -lstdc++ -lpthread -ldl -o build/logger.o -g -DDEBUG
	g++ -shared -o build/liblogger.so build/logger.o  -g -DDEBUG
	g++ -c -fPIC src/jsonhelper.cpp -std=c++11 -g -o build/jsonhelper.o -DDEBUG
	g++ -shared -o build/libjsonhelper.so build/jsonhelper.o  -g -DDEBUG
	gcc -L/home/user/git_mio/torchat/build $(ALL) -I. -g  -ljsonhelper -o build/main -lpthread -llogger -ldl -DDEBUG 
