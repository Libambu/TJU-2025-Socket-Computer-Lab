/* C declarations used in actions */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "parse.h"

int main(int argc, char **argv) {
    //Read from the file the sample
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 0;
    }

    const int fd_in = open(argv[1], O_RDONLY);
    char buf[BUFSIZ];
    if (fd_in < 0) {
        printf("Failed to open the file\n");
        return 0;
    }
    const int readRet = (int) read(fd_in, buf, BUFSIZ);

    //Parse the buffer to the parse function. You will need to pass the socket fd and the buffer would need to
    //be read from that fd
    Request *request = parse(buf, readRet, fd_in);
    if (request == NULL) {
        return 0;
    }
    //Just printing everything
    printf("Http Method %s\n", request->http_method);
    printf("Http Version %s\n", request->http_version);
    printf("Http Uri %s\n", request->http_uri);
    for (int index = 0; index < request->header_count; index++) {
        printf("Request Header\n");
        printf("Header name %s Header Value %s\n", request->headers[index].header_name,
               request->headers[index].header_value);
    }
    free(request->headers);
    free(request);
    return 0;
}
