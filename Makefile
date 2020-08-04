CC = gcc -fPIC
CFLAGS= -g
CPPFLAGS= -g
DEBUG=
NETLIBS=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	NETLIBS = -lnsl
endif
LINK= -pthread
OBJS= myhttpd.o auth.o http-concurrency.o

all: myhttpd

myhttpd : $(OBJS)
	$(CXX) -o $@ $(OBJS) $(NETLIBS) $(LINK)

%.o: %.c
	@echo 'Building $@ from $<'
	$(CC) $(CFLAGS) $(DEBUG) -o $@ -c -I. $<

clean:
	rm -f *.o myhttpd
