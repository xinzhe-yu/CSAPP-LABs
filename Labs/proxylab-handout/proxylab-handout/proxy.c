#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <pthread.h>
#include "csapp.h"

void *thread(void *vargp); 
int parse_uri(char *uri, char *host, char *port, char *path);
void service(int fd);

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
//#define LISTENQ 10
#define PORTLEN 16

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char *argv[])
{   
    /* Check command-line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* A client that disappears mid-response must not kill the proxy */
    signal(SIGPIPE, SIG_IGN);

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

    freeaddrinfo(listp);
    if (!p) { return -1; } //P ran to end of linked list
    
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
        return -1;
    }
    
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    int *connfd;

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = malloc(sizeof(int));
        if (!connfd) {
            fprintf(stderr, "Malloc failed\n");
            continue;
        }
        *connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
        if (*connfd < 0) {
            free(connfd);
            continue;
        }
        Pthread_create(&tid, NULL, thread, connfd);
    }
    return 0;
}

/* Thread routine */
void *thread(void *vargp) {
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    service(connfd);
    close(connfd);
    return NULL;
}

void service(int fd) {
    rio_t rio; 
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    rio_readinitb(&rio, fd); // Init rio and its buffer
    if (rio_readlineb(&rio, buf, MAXLINE) <= 0) { // client hung up or errored
        return;
    }

    printf("Request headers:\n");
    printf("%s", buf);
    // Parse first line GET URL Version
    if (sscanf(buf, "%s %s %s", method, uri, version) != 3) {
        printf("Malformed request line\n");
        return;
    }

    if (strcasecmp(method, "GET")) { // Return 1 when !match
        printf("Proxy does not implement this method");
        return;
    }

    /* Parse URI from GET request */
    char hostname[MAXLINE], port[PORTLEN], path[MAXLINE];
    if (parse_uri(uri, hostname, port, path) < 0) {
        printf("Invalid URI\n");
        return;
    }

    char host_hdr[MAXLINE] = ""; //'\0'
    char other_hdrs[MAXLINE] = "";
    char hdr[MAXLINE];

    ssize_t hn;
    while ((hn = rio_readlineb(&rio, hdr, MAXLINE)) > 0) {
        if (!strcmp(hdr, "\r\n")) {
            break;
        }
        if (!strncasecmp(hdr, "Host:", 5)) { // if host exist, parse to host_hdr 
            snprintf(host_hdr, MAXLINE, "%s", hdr);
        }
        else if (!strncasecmp(hdr, "User-Agent:", 11) || //drop these
                 !strncasecmp(hdr, "Connection:", 11) ||
                 !strncasecmp(hdr, "Proxy-Connection:", 17)) {
            continue;
        }
        else {
            strncat(other_hdrs, hdr, MAXLINE - strlen(other_hdrs) - 1); //append other headers
        }
    }

    if (hn < 0) { // client vanished mid-request
        return;
    }

    if (host_hdr[0] == '\0') {
        snprintf(host_hdr, MAXLINE, "Host: %s\r\n", hostname);
    }


    /* Build the request */
    char request[MAXLINE]; // "GET <path> HTTP/1.0\r\n Host: <hostname>\r\n ..."
    snprintf(request, MAXLINE,
        "%s %s HTTP/1.0\r\n%s%sConnection: close\r\nProxy-Connection: close\r\n%s\r\n",
        method, path, host_hdr, user_agent_hdr, other_hdrs);

    printf("=== forwarding ===\n%s=== end ===\n", request);
    fflush(stdout);
        
    /* Open new connection */
    int serverfd = open_clientfd(hostname, port);
    if (serverfd < 0) {
        return;
    }
    printf("connect to [%s]:[%s] -> %d\n", hostname, port, serverfd);
    fflush(stdout);

    // Write send request to new connection
    if (rio_writen(serverfd, request, strlen(request)) < 0) {
        close(serverfd);
        return;
    }

    // New rio to read from connection
    rio_t sio;
    char sbuf[MAXLINE];
    ssize_t n; 

    rio_readinitb(&sio, serverfd);
    while ((n = rio_readnb(&sio, sbuf, MAXLINE)) > 0) { // Read from server n
        if (rio_writen(fd, sbuf, n) < 0) break;         // Write to client n 
    }

    close(serverfd);


}

    // char *testcase[10] = {
    //     "http://www.cmu.edu:8080/hub/index.html",
    //     "http://www.cmu.edu:8080/index.html",
    //     "http://www.cmu.edu:8080",
    //     "http://www.cmu.edu/",
    //     "http://www.cmu.edu/a:b/c",
    //     "http://www.cmu.edu/x?t=1:2",
    //     "http://www.cmu.edu/s?q=cs&n=2",
    //     "http://www.cmu.edu:8080/p?x=1#frag",
    //     "www.cmu.edu/index.html"
    // };

    

    // int n = sizeof(testcase)/ sizeof(testcase[0]);
    // for (int i = 0; i < n; i++) {
    //     char host[MAXLINE], port[PORTLEN], path[MAXLINE];
    //     parse_uri(testcase[i], host, port, path);
    //     printf("%s:%s/%s\n", host, port, path);
    // }
int parse_uri(char *uri, char *host, char *port, char *path) {
    char *p = strstr(uri, "://");
    p = p ? p + 3 : uri; // P points to start of www.

    char *colon = strchr(p, ':');
    char *slash = strchr(p, '/');


    if (colon == NULL || (slash && colon > slash)) { // No valid colon
        strcpy(port, "80");
        colon = NULL;

        if (slash) {    
            snprintf(host, MAXLINE, "%.*s", (int)(slash - p), p);
            snprintf(path, MAXLINE, "%s", slash);
        } 
        else { // No valid slash
            snprintf(host, MAXLINE, "%s", p);
            strcpy(path, "/");
        }

        if (host[0] == '\0') {
            return -1;
        }
        return 1;
    }
    else {
        snprintf(host, MAXLINE, "%.*s", (int)(colon - p), p);

        if (slash) {
            snprintf(port, PORTLEN, "%.*s", (int)(slash - colon -1), colon + 1);
            snprintf(path, MAXLINE, "%s", slash);
        }
        else {
            snprintf(port, PORTLEN, "%s", colon + 1);
            strcpy(path, "/");
        }

        if (host[0] == '\0') {
            return -1;
        }
        return 1;
    }

    return -1;
}