#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <openssl/sha.h>
#include <unistd.h>
#include <pthread.h> 
#include <dirent.h>
#include <fcntl.h>

//------------Command function declarations--------------

void *upgrade(void*ptr);
void push(char* projName);
void substr(char* str, char* sub, int start, int len);


//SERVER
void *destroy(void*ptr);
void *create(void *ptr); 
void *checkout(void *ptr);
void history(char* projName);
void* rollback(void*ptr);
void *currentVersion(void*ptr);
void *update(void *ptr);
void *commit(void*ptr);

//CLIENT
void checkoutClient(char* projName);
void configure(char* host, char* port);
void connectToServer(char* ipHost, int port);
void createClient(char* projName); 
void addFile(char* projName, char* filePath);
void removeFile(char* projName, char* filePath);