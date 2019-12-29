#ifndef URL_H
#define URL_H

#define DEFAULT_PORT "80"

typedef struct Url {
    char *scheme;
    char *hostname;
    char *port;
    char *path;
} Url;

Url *url_parse(char *url, size_t len);
void url_free(Url *url);

#endif
