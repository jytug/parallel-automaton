#pragma once

#include <mqueue.h>

#define MAX_LINE_LEN 1001
#define MAX_MSG_LEN 1001
#define AMOUNT_TO_READ 10000
#define MAX_PIPE_MSG_LEN 100

#define ANS_AFF "yes"
#define ANS_NEG "no"

char *read_line(char *);
char *read_line_from_mq(mqd_t desc, char *);
int encode_char(char);
char decode_char(int);
int is_number(char *);
