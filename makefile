ALL = src/main.c include/mongoose.c src/socks_helper.c src/util.c src/list.c


default:
	mkdir -p build
	echo 'Remember to execute export ( DOLLAR ) LD_LIBRARY_PATH= current directory'
	g++ -c -Wall -fPIC src/jsonhelper.cpp -std=c++11 -g
	g++ -shared -o libjsonhelper.so jsonhelper.o  -g
	gcc -L/home/francesco/Desktop/Programs/torchat $(ALL) -I. -g  -ljsonhelper -o build/main -lpthread

clang:
	mkdir -p build
	echo 'Remember to execute export ( DOLLAR ) LD_LIBRARY_PATH= current directory'
	g++ -c -Wall -fPIC src/jsonhelper.cpp -std=c++11 -g
	g++ -shared -o libjsonhelper.so jsonhelper.o  -g
	clang -L/home/francesco/Desktop/Programs/torchat $(ALL) -I. -g  -ljsonhelper -o build/main -lpthread
