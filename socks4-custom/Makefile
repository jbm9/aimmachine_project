VERSION =	0.9.1

INCLUDE =	-I/usr/local/include/Sockets

CFLAGS =	-Wall -pedantic -g -O2 $(INCLUDE) `Sockets-config` -MD
CPPFLAGS =	$(CFLAGS) 

LIBS =		-L/usr/local/lib -lSockets -lpthread -lssl -lcrypto

PROGS =		socks4

all:		inject_am $(PROGS)

inject_am:
	cp ../oscar2.c ../oscar2.h ../frame.c ../frame.h ../oft.c ../oft.h .


socks4:		Socks4Socket.o socks4.o RemoteSocket.o Socks4Handler.o oscar2.o frame.o
		g++ -o $@ $^ $(LIBS)


clean:
		rm -f *~ *.o slask $(PROGS) *.d

-include	*.d



tar:		clean
		tar czf socks4-$(VERSION).tar.gz *.h *.cpp Makefile gpl.txt
		/usr/local/bin/tarfix.sh socks4-$(VERSION)
		cp socks4-$(VERSION).tar.gz /usr/local/apache/www.alhem.net/htdocs/project/socks4
		cp socks4-$(VERSION).zip /usr/local/apache/www.alhem.net/htdocs/project/socks4

