CC=gcc
CFLAGS := -g -O3 -Wall

TARGETS := rkafpack  rkcrc  rkdump  rkflash  rkunpack

all: ${TARGETS}

clean:
	rm ${TARGETS}
