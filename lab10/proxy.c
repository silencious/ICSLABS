/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Andrew Carnegie, ac00@cs.cmu.edu
 *     Harry Q. Bovik, bovik@cs.cmu.edu
 *
 * Name: TianJiahe
 * ID: 5130379056
 *
 * This is a basic proxy that simply sends requests from client
 * to server, and then transfer data from server to client.
 * Use multithreads to do the work.
 * Maximum number of threads is NTHREADS as below.
 *
 */

#include "csapp.h"
#define NTHREADS 8
#define SBUFSIZE 16

// struct of the client, consists of fd and addr.
typedef struct{
    int fd;
    struct sockaddr_in addr;
}client;

// struct of sbuf
typedef struct{
    client **buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
}sbuf_t;

/*
 * Function prototypes
 * Detailed descriptions are above their definition
 */
void *thread(void* vargp);
int Open_clientfd_ts(char* servername, int port);
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, client* cl);
client* sbuf_remove(sbuf_t *sp);
void doit(client *cl);
void writelog(char* log);
ssize_t Rio_readn_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);

sem_t mutex1;   // mutex to be used in Open_clientfd_ts()
sem_t mutex2;   // mutex to be used in writelog()
sbuf_t sbuf;

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }
    // Clear previous log
    system("if [ -f proxy.log ]; then\nrm proxy.log\nfi");
    // Ignore SIGPIPE
    Signal(SIGPIPE, SIG_IGN);
    int listenfd, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    pthread_t tid;

    Sem_init(&mutex1, 0, 1);
    Sem_init(&mutex2, 0, 1);
    sbuf_init(&sbuf, SBUFSIZE);
    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);

    // Create working threads in advance
    int i;
    for(i=0; i<NTHREADS; i++)
        Pthread_create(&tid, NULL, thread, NULL);

    // Busy loop to accept clients and insert them to sbuf
    while(1){
        client *cl = (client *)Malloc(sizeof(client));
        cl->fd = Accept(listenfd, (SA *)&cl->addr, &clientlen);
        sbuf_insert(&sbuf, cl);
    }
    // Control should never reach here
    sbuf_deinit(&sbuf);
    exit(0);
}

// Thread routine, remove a client from sbuf and
// response to the request
void *thread(void *vargp){
    Pthread_detach(Pthread_self());
    while(1){
        client* cl = sbuf_remove(&sbuf);
        doit(cl);
    }
    return NULL;
}

// The core function to send requests from client to server,
// and then transfer data from server to client
void doit(client* cl){
    // Variables needed later
    int clientfd = cl->fd;
    struct sockaddr_in clientaddr = cl->addr;
    Free(cl);   // cl is no longer needed
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t clientRio, serverRio;

    char toServer[MAXLINE], toClient[MAXLINE];
    char servername[MAXLINE], pathname[MAXLINE];
    int serverfd, port;

    Rio_readinitb(&clientRio, clientfd);
    if(Rio_readlineb_w(&clientRio, buf, MAXLINE) < 0)
        return;     // Failed to receive request from client
    sscanf(buf, "%s %s %s", method, uri, version);
    // Proxy doesn't support other methods
    if(strcasecmp(method, "GET")){
        clienterror(clientfd, method, "501", "Not Implemented",
                "Proxy does not implement this method");
        return;
    }
    parse_uri(uri, servername, pathname, &port);

    // Failed to connect to server
    if((serverfd = Open_clientfd_ts(servername, port)) < 0){
        strcpy(toClient, "Cannot connect to server");
        Rio_writen_w(clientfd, toClient, strlen(toClient));
        return;
    }
    // Format string to be sent to server
    sprintf(toServer, "%s %s %s\r\n", method, pathname, version);
    // Other infos client wants to send to server
    while(strcmp(buf, "\r\n") != 0){
        Rio_readlineb_w(&clientRio, buf, MAXLINE);
        strcat(toServer, buf);
    }

    Rio_writen_w(serverfd, toServer, sizeof(toServer));
    printf("Connecting to: %s\n", servername);
    // Prepare to receive data from server
    Rio_readinitb(&serverRio, serverfd);
    int n, count = 0;
    while((n = Rio_readn_w(&serverRio, buf, MAXLINE)) > 0){
        if(Rio_writen_w(clientfd, buf, n) != n)
            break;
        count += n;     // count counts the bytes of data
    }
    Close(serverfd);
    printf("Disconnect from: %s\nretrieve %d bytes.\n", servername, count);
    // Format string to be logged
    format_log_entry(buf, &clientaddr, uri, count);
    writelog(buf);
    Close(clientfd);
    return;
}

// Thread-safe version of open_clientfd, simply lock and unlock
int Open_clientfd_ts(char* servername, int port){
    P(&mutex1);
    int fd;
    while((fd = open_clientfd(servername, port)) < 0)
        printf("Error with open_clientfd.\n");
    V(&mutex1);
    return fd;
}

// These are just same as in the CSAPP textbook
// Init sbuf
void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

// Deinit sbuf. Free all malloced data
void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf);
}

// Insert a client to sbuf
void sbuf_insert(sbuf_t *sp, client* item){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear)%(sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
}

// Remove and return a client's pointer
client* sbuf_remove(sbuf_t *sp){
    client* item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front)%(sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}

// Rio read n bytes. Print message and return if there's error.
ssize_t Rio_readn_w(rio_t *rp, void *usrbuf, size_t n){
    ssize_t rc;
    if ((rc = rio_readnb(rp, usrbuf, n)) < 0){
        printf("Error with rio_readnb.\n");
        return 0;
    }
    return rc;
}

// Rio read a line. Print message and return if there's error.
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen){
    ssize_t rc;
    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0){
        printf("Error with rio_readlineb.\n");
        return -1;
    }
    return rc;
}

// Rio write n bytes. Print message and return if there's error.
ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n){
    ssize_t rc;
    if ((rc = rio_writen(fd, usrbuf, n)) != n){
        printf("Error with rio_writen.\n");
        return 0;
    }
    return n;
}

// Use lock to write the log, so that there will not be race
void writelog(char* log){
    P(&mutex2);
    FILE* logfile = Fopen("proxy.log", "a");
    fprintf(logfile, "%s", log);
    Fclose(logfile);
    V(&mutex2);
}

// Print client error message
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen_w(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen_w(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen_w(fd, buf, strlen(buf));
    Rio_writen_w(fd, body, strlen(body));
}

/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }

    *port = 80; /* default */

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n");
    len = hostend - hostbegin;
    if(strlen(hostbegin) < len){
        // Failed to find the characters
        strcpy(hostname, hostbegin);
        strcpy(pathname, "/");
        return 0;
    }
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    if (*hostend == ':')
        *port = atoi(hostend + 1);

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL)
        strcpy(pathbegin, "/");
    else
        strcpy(pathname, pathbegin);

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
        char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /*
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %d\n", time_str, a, b, c, d, uri, size);
}

