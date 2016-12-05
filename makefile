ALL = main.c mongoose.c


default:
	g++ -c -Wall -fPIC jsonhelper.cpp -std=c++11
	g++ -shared -o libjsonhelper.so jsonhelper.o 
	gcc -L/home/user/git_mio/torchat $(ALL) -I. -g  -ljsonhelper -o main
