#include "in_out.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_line(char *line) {
    memset(line, 0, MAX_LINE_LEN);
    if (fgets(line, MAX_LINE_LEN - 1, stdin) == NULL)
        return NULL;
    line[strlen(line) - 1] = '\0';
    return line;
}

char *read_line_from_mq(mqd_t desc, char *line) {
    memset(line, 0, MAX_LINE_LEN);
    if (mq_receive(desc, line, AMOUNT_TO_READ, NULL) == (mqd_t) -1)
        return NULL;
    return line;
}

int encode_char(char c) {
    return c - 'a';
}

char decode_char(int i) {
    return 'a' + i;
}

int is_number(char *str) {
    int i;
    for (i = 0; i < strlen(str); i++)
        if (!isdigit(str[i]))
            return 0;
    return 1;
}
