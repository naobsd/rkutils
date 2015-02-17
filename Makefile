CC=gcc
CFLAGS := -g -O2 -DUSE_OPENSSL
LDLIBS := -lcrypto


TARGETS = rkafpack rkcrc rkdump rkflash rkunpack
SOURCES = rkafpack.c rkcrc.c rkdump.c rkflash.c rkunpack.c
OBJECTS = rkafpack rkcrc rkdump rkflash rkunpack




all: $(TARGETS)
	
rkafpack:
	$(CC) $(CFLAGS) -o rkafpack rkafpack.c $(LDLIBS)
	
rkcrc:
	$(CC) $(CFLAGS) -o rkcrc rkcrc.c $(LDLIBS)


rkdump:
	$(CC) $(CFLAGS) -o rkdump rkdump.c $(LDLIBS)
	
rkflash:
	$(CC) $(CFLAGS) -o rkflash rkflash.c $(LDLIBS)
	
rkunpack:
	$(CC) $(CFLAGS) -o rkunpack rkunpack.c $(LDLIBS)
	
clean: 
	rm -rf $(OBJECTS)
