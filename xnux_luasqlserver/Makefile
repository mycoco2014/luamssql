lib:
	gcc -fPIC -c tdssqlserver.h tdssqlserver.cpp
	gcc -lstdc++ -fPIC -shared tdssqlserver.cpp tdssqlserver.o -o tdssqlserver.so -llua.5.2
	rm tdssqlserver.o

clean:
	rm *.o *.so
