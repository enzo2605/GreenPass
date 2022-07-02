#include "LabUtilities.h"

int Socket(int domain, int type, int protocol) {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }
    return sockfd;
}

void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (connect(sockfd, addr, addrlen) < 0) {
        perror("connect error");
        exit(1);
    }
}

void Close(int fd) {
    if (close(fd) < 0) {
        perror("close error");
        exit(-1);
    }
}

void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (bind(sockfd, addr, addrlen) < 0 ) {
		perror("bind error");
		exit(1);
	}
}

void Listen(int sockfd, int backlog) {
    if (listen(sockfd, backlog) < 0) {
		perror("listen error");
		exit(1);
	}
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int connfd;
    if ((connfd = accept(sockfd, addr, addrlen) ) < 0 ) {
		perror("accept error");
		exit(1);
	}
    return connfd;
}


void IPConversion(int af, const char *src, struct in_addr *dst) {
    if (inet_pton(AF_INET, src, dst) < 0) {
        fprintf(stderr,"inet_pton error for %s\n", src);
        exit(1);
    }
}

void *getCurrentDir(void) {
    char *currWorkDir, *token;
    char buffer[MAX_PATH + 1];
    char *directory;
    size_t length;

    // Ottieni il path della directory corrente
    currWorkDir = getcwd(buffer, MAX_PATH + 1);
    if(currWorkDir == NULL) {
        perror("getcwd error");
        exit(1);
    }

    // Cerca l'ultima occorrenza del carattere backslash
    // Il puntatore token punterrà al carattere successivo all'ultima occorrenza di backslash
    token = strrchr(currWorkDir, '/');
    if (token == NULL) {
        fprintf(stderr, "Errore: directory mancante");
        exit(1);
    }

    // Alloca una stringa per contenere il nome della directory
    length = strlen(token);
    directory = malloc(length);
    if (directory == NULL) {
        fprintf(stderr, "malloc error\n");
        exit(CALLOC_ERROR);
    }
    // Copia il contenuto di token in directory
    memcpy(directory, token + 1, length);

    return directory;
}

void Chdir(const char *path) {
    if (strcmp((const char *)getCurrentDir(), path) != 0) {
        if (chdir(path) == -1) {
            perror("chdir error");
            exit(1);
        }
    }
}

void Remove(const char *pathname) {
    if (remove(pathname) == -1) {
        perror("remove error");
        exit(1);
    }
}

void Rename(const char *oldpath, const char *newpath) {
    if (rename(oldpath, newpath) == -1) {
        perror("rename error");
        exit(1);
    }
}

ssize_t FullWrite(int fd, void *buf, size_t count) {
    size_t nleft;
    ssize_t nwritten;
    nleft = count;
    while (nleft > 0) { // repeat until no left
        if ((nwritten = write(fd, buf, count)) < 0) {
            if (errno == EINTR) { // if interrupted by system call
                continue; // continue the loop
            }
            else { // otherwise exit with error
                perror("write");
                exit(nwritten);
            }
        }
        nleft -= nwritten; // decreas nleft by nwritten
        buf += nwritten; // increase the buf pointer by nwritten
    }
    return (nleft);
}

ssize_t FullRead(int fd, void *buf, size_t count) {
    size_t nleft;
    ssize_t nread;
    nleft = count;
    while (nleft > 0) { // repeat until no left
        if ((nread = read(fd, buf, count)) < 0) {
            if (errno == EINTR) { // if interrupted by system call
                continue; // continue the loop
            }
            else { // otherwise exit with error
                perror("read");
                exit(nread);
            }
        }
        else if (nread == 0) { // if nread == 0 server went off
            break;
        }
        nleft -= nread; // decreas nleft by nread
        buf += nread; // increase the buf pointer by nwritten
    }
    buf = 0;
    return (nleft);
}