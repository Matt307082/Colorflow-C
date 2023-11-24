# Makefile

CC = gcc
CFLAGS = -Wall -lpng -ljpeg include/libnsbmp.c

all: colorflow

colorflow: colorflow.c 
	${CC} colorflow.c -o colorflow ${CFLAGS}

test: mktests.sh
	$(shell) ./mktests.sh

result: mkresult.sh colorflow
	$(shell) ./mkresult.sh

clean: colorflow
	rm colorflow