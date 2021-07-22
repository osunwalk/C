all: WTF WTFserver

WTF: client.c
	gcc -lssl -lcrypto -pthread client.c -g -o WTF

WTFserver: 
	gcc -lssl -lcrypto -pthread server.c -g -o WTFserver
test:
	
.PHONY = clean
clean:
	rm -f WTF