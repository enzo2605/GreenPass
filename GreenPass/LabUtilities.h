/**
 * LabUtilities.h
 * Created by Vincenzo Iannucci
 * 
 * Questa libreria contiene le funzioni wrapper per le system call utilizzate nel progetto. Ciò permette
 * agli altri file del progetto di richiamare le suddette system call senza preoccuparsi della gestione di 
 * eventuali errori connessi al loro utilizzo.
 * 
 * */

#ifndef LabUtilies_h
#define LabUtilies_h

#include <stdio.h>      // fprintf, perror
#include <stdlib.h>     // exit
#include <unistd.h>     // read, write, close
#include <errno.h>      // perror, errno
#include <string.h>     // strlen
#include <sys/types.h>  // socket, bind, listen, connect
#include <sys/socket.h> 
#include <arpa/inet.h>  // struct sockaddr_in
#include <time.h>       // time
#include <ctype.h>
#include <sys/select.h> // select
#include "ErrorNo.h"

#define MAXLINE 256
#define CLIENT_QUEUE_SIZE 1024
#define MAX_PATH 4096

#define max(x, y) ({typeof (x) x_ = (x); typeof (y) y_ = (y); \
x_ > y_ ? x_ : y_;}) 

int Socket(int domain, int type, int protocol);
void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void Listen(int sockfd, int backlog);
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
void IPConversion(int af, const char *src, struct in_addr *dst);
void Close(int fd);
void *getCurrentDir(void);
int Chdir(const char *path);
int Remove(const char *pathname);
int Rename(const char *oldpath, const char *newpath);
ssize_t FullWrite(int fd, void *buf, size_t count);
ssize_t FullRead(int fd, void *buf, size_t count);

#endif