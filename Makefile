# Makefile

CC = gcc
CFLAGS = -Wall -lpng -ljpeg -L/usr/lib/libbmp -lbmp

all: colorflow

colorflow: colorflow.c 
	${CC} colorflow.c -o colorflow ${CFLAGS}

test: mktests.sh
	$(shell) ./mktests.sh