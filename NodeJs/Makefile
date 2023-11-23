libcolorflow: native/colorflow.c
	gcc -c -Wall -Werror -fpic native/colorflow.c
	gcc -shared -o libcolorflow.so colorflow.o -lpng -ljpeg native/include/libnsbmp.c
	rm colorflow.o
