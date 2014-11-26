###########################################
#Makefile for simple programs
###########################################
CC=gcc

OBJ=rkafpack rkcrc rkcrc16 rkdump rkflash rkunpack

all:$(OBJ)

clean:
	rm -rf rkafpack rkcrc rkcrc16 rkdump rkflash rkunpack
