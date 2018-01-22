#include "config.h"
#include "err.h"
#include "in_out.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>
#include <unistd.h>

/* global variables */
struct mq_attr spec = {.mq_maxmsg = 4, .mq_msgsize = MAX_MSG_LEN - 1 };

char line[MAX_LINE_LEN];
char buf[MAX_MSG_LEN];

mqd_t val_in_mq_desc = -1,
      tester_mq_desc_out = -1,
      tester_mq_desc_in = -1;

char my_pid[MAX_MSG_LEN];
char my_pid_in[MAX_MSG_LEN];
char my_pid_out[MAX_MSG_LEN];

int snt, rcd, acc;

/* free all resources */
void cleanup() {
    if (val_in_mq_desc != -1) {
        if (mq_close(val_in_mq_desc) == -1) syserr("mq_close");
        val_in_mq_desc = -1;
    }

    if (tester_mq_desc_in != -1) {
        if (mq_close(tester_mq_desc_in) == -1) syserr("mq_close");
        mq_unlink(my_pid_in);
        tester_mq_desc_in = -1;
    }

    if (tester_mq_desc_out != -1) {
        if (mq_close(tester_mq_desc_out) == -1) syserr("mq_close");
        mq_unlink(my_pid_out);
        tester_mq_desc_out = -1;
    }
}

void print_summary() {
    printf("Snt: %d\nRcd: %d\nAcc: %d\n", snt, rcd, acc);
}

/* signal handler */
void sighandle(int signal) {
    cleanup();
    print_summary();
    exit(0);
}

int main() {
    printf("PID: %d\n", getpid());
    signal(SIG_END, sighandle);

    sprintf(my_pid, "%d", getpid());
    sprintf(my_pid_in, "/%d_in", getpid());
    sprintf(my_pid_out, "/%d_out", getpid());

    val_in_mq_desc = mq_open(VALIDATOR_MAIN_QUEUE_IN, O_WRONLY | O_CREAT, 0660, &spec);
    if (val_in_mq_desc == (mqd_t) -1) goto err;

    /*send a request to open a personal message queue */
    if (mq_send(val_in_mq_desc, my_pid, strnlen(my_pid, MAX_MSG_LEN), MSG_PRIO) == -1) goto err;

    if (mq_close(val_in_mq_desc) == -1) goto err;
    val_in_mq_desc = -1;
    
    tester_mq_desc_in = mq_open(my_pid_in, O_RDONLY | O_CREAT, 0660, &spec);
    if (tester_mq_desc_in == (mqd_t) -1) goto err;

    tester_mq_desc_out = mq_open(my_pid_out, O_WRONLY | O_CREAT, 0660, &spec);
    if (tester_mq_desc_out == (mqd_t) -1) goto err;

    while (read_line(line) != NULL) {
        if (strcmp(line, TERM_MSG) == 0) {
            val_in_mq_desc = mq_open(VALIDATOR_MAIN_QUEUE_IN, O_WRONLY | O_CREAT, 0660, &spec);
            if (val_in_mq_desc == (mqd_t) -1) goto err;
            if (mq_send(val_in_mq_desc, TERM_MSG, strlen(TERM_MSG), TERM_PRIO) == -1) goto err;
            if (mq_close(val_in_mq_desc)) goto err;
            val_in_mq_desc = -1;
            continue;
        }

        memset(buf, 0, sizeof(buf));
        if (mq_send(tester_mq_desc_out, line, strlen(line), MSG_PRIO) == -1) goto err;

        snt++;

        if (mq_receive(tester_mq_desc_in, buf, AMOUNT_TO_READ, NULL) == -1) goto err;
        rcd++;

        if (strcmp(buf, ANS_AFF) == 0) {
            printf("%s A\n", line);
            acc++;
        } else if (strcmp(buf, ANS_NEG) == 0) {
            printf("%s N\n", line);
        } else {
            goto err;
        }
    }


    print_summary();

err:
    cleanup();
    if (errno != 0) syserr("tester");
    return 0;
}
