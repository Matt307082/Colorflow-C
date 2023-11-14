# Makefile

CC = gcc
CFLAGS = -Wall -lpng -ljpeg lib/libbmp.c

all: colorflow

colorflow: colorflow.c 
	${CC} colorflow.c -o colorflow ${CFLAGS}

test: mktests.sh
	$(shell) ./mktests.sh