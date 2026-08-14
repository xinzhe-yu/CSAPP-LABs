#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <pthread.h>


void *thread(void *vargp); 


/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define LISTENQ 10

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char *argv[])
{   

    /* Check command-line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    printf("%s", user_agent_hdr);

    // Listening socket 
    /* Get a list of potential server addresses */
    struct addrinfo hints = {0}, *listp, *p;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;        // Passive listen
    hints.ai_flags |= AI_ADDRCONFIG;    // Recommended 
    hints.ai_flags |= AI_NUMERICSERV;   // using a numeric port arg

    int rc = getaddrinfo(NULL, argv[1], &hints, &listp);
    if (rc != 0) { fprintf(stderr, "%s\n", gai_strerror(rc)); return -1; }

    int listenfd, optval = 1; 
    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) >= 0) {
            break;
        }
        close(listenfd);
    }
        
    if (!p) { freeaddrinfo(listp); return -1; }
    freeaddrinfo(listp);

    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
        return -1;
    }
    
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    int connfd;

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
        if (connfd < 0) { continue; }
        service(connfd);
        close(connfd);
    }



    

    
    


    

    return 0;
}
