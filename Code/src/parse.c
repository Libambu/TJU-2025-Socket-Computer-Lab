#include "parse.h"

/**
* Given a char buffer returns the parsed request headers
*/
Request *parse(const char *buffer, const int size, int socketFd) {
    //Different states in the state machine
    enum {
        STATE_START = 0, STATE_CR, STATE_CRLF, STATE_CRLFCR, STATE_CRLFCRLF
    };

    int i = 0;
    size_t offset = 0;
    char parse_buf[8192] = {0};

    // FSM to parse the request
    // expect for this regular expression:
    // .*(\r\n\r\n)
    // represent end of the http header part
    int state = STATE_START;
    while (state != STATE_CRLFCRLF) {
        char expected = 0;

        // when reach the end of the buffer, break
        if (i == size)
            break;

        const char ch = buffer[i++];
        parse_buf[offset++] = ch;

        switch (state) {
            case STATE_START:
            case STATE_CRLF:
                expected = '\r';
                break;
            case STATE_CR:
            case STATE_CRLFCR:
                expected = '\n';
                break;
            default:
                state = STATE_START;
                continue;
        }

        if (ch == expected)
            state++;
        else
            state = STATE_START;
    }

    //Valid End State
    if (state == STATE_CRLFCRLF) {
        Request *request = malloc(sizeof(Request));
        request->header_count = 0;
        request->headers = (Request_header *) malloc(sizeof(Request_header) * 1);
        set_parsing_options(parse_buf, i, request);

        if (yyparse() == SUCCESS) {
            return request;
        }
    }

    return NULL;
}
