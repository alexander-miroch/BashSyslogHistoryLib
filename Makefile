all:
	gcc -shared -fPIC ld.c -o bashpreload.so -ldl

