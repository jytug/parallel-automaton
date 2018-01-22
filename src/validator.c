#include "automaton.h"
#include "config.h"
#include "in_out.h"
#include "err.h"
#include "tester_info.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <sys/wait.h>
#include <mqueue.h>
#include <unistd.h>

#define MAX_CHILDREN 100

/* global variables */

char buf[MAX_MSG_LEN];
char word[MAX_MSG_LEN];

char tester_pid_in[MAX_MSG_LEN];
char tester_pid_out[MAX_MSG_LEN];

char my_pid_in[MAX_MSG_LEN];
char my_pid_out[MAX_MSG_LEN];

TesterInfo info = {.pid = 0, .snt = 0, .rcd = 0, .acc = 0};

mqd_t tester_mq_desc_out = -1,
      tester_mq_desc_in = -1,
      desc_in = -1,
      desc_out = -1,
      val_in_mq_desc = -1,
      end_mq_desc = -1;

Automaton aut;

struct mq_attr spec = {.mq_maxmsg = 4, .mq_msgsize = MAX_MSG_LEN - 1};

/* only used by parent */
int parent_pid;
int n_children = 0;
int children_pids[MAX_CHILDREN];


void cleanup() {
    if (desc_in != -1)
        mq_close(desc_in);
    desc_in = -1;
    if (desc_out != -1)
        mq_close(desc_out);
    desc_out = -1;
    if (tester_mq_desc_in != -1)
        mq_close(tester_mq_desc_out);
    tester_mq_desc_in = -1;
    if (tester_mq_desc_out != -1)
        mq_close(tester_mq_desc_in);
    tester_mq_desc_out = -1;
    if (val_in_mq_desc != -1)
        mq_close(tester_mq_desc_in);
    val_in_mq_desc = -1;
    if (end_mq_desc != -1)
        mq_close(tester_mq_desc_in);
    end_mq_desc = -1;

    if (strlen(my_pid_in) > 0)
        mq_unlink(my_pid_in);
    if (strlen(my_pid_out) > 0)
        mq_unlink(my_pid_out);

    if (getpid() == parent_pid) {
        mq_unlink(VALIDATOR_MAIN_QUEUE_IN);
        mq_unlink(VALIDATOR_END_QUEUE_IN);
        destroy_automaton(&aut);
    }
}

void end_error() {
    kill(parent_pid, SIG_FLT);
}


void sighandle_normal(int signum) {
    cleanup();
    memset(buf, 0, MAX_MSG_LEN);
    sprintf(buf, "%d %d %d %d", info.pid, info.rcd, info.snt, info.acc);

    end_mq_desc = mq_open(VALIDATOR_END_QUEUE_IN, O_WRONLY | O_CREAT, 0660, &spec);
    if (end_mq_desc == (mqd_t) -1)
        syserr("mq_open");

    if (mq_send(end_mq_desc, buf, strlen(buf), MSG_PRIO) == -1) end_error();
    mq_close(end_mq_desc);
    end_mq_desc = -1;

    kill(info.pid, SIG_END);
    exit(0);
}

void sighandle_err_parent(int signum) {
    cleanup();
    int i;
    for (i = 0; i < n_children; i++) {
        kill(children_pids[i], SIG_FLT);
    }
    while (wait(NULL) > 0);
    exit(1);
}

void sighandle_err_child(int signum) {
    wait(NULL);
    kill(info.pid, SIG_END);
    cleanup();
}

void tester_subprocess(pid_t tester_pid, Automaton *aut) {
    signal(SIG_END, sighandle_normal);
    signal(SIG_FLT, sighandle_err_child);

    int pid;

    sprintf(tester_pid_in, "/%d_in", tester_pid);
    sprintf(tester_pid_out, "/%d_out", tester_pid);

    sprintf(my_pid_in, "/%d_in", getpid());
    sprintf(my_pid_out, "/%d_out", getpid());


    info.pid = tester_pid;
    while (1) {
        desc_in = mq_open(my_pid_in, O_RDWR | O_CREAT, 0660, &spec);
        desc_out = mq_open(my_pid_out, O_RDWR | O_CREAT, 0660, &spec);

        if (desc_in == -1) end_error();
        if (desc_out == -1) end_error();

        memset(word, 0, MAX_MSG_LEN);
        memset(buf, 0, MAX_MSG_LEN);

        tester_mq_desc_out = mq_open(tester_pid_out, O_RDONLY | O_CREAT, 0660, &spec);
        if (tester_mq_desc_out == (mqd_t) -1) end_error();
        if (mq_receive(tester_mq_desc_out, word, AMOUNT_TO_READ, NULL) == -1) end_error();
        if (mq_close(tester_mq_desc_out) == -1) end_error();
        tester_mq_desc_out = -1;
        info.rcd++;

        switch (pid = fork()) {
            case -1:
                end_error();
            case 0:
                memset(buf, 0, MAX_MSG_LEN);
                sprintf(buf, "%d", parent_pid);
                execlp("./run", "run", my_pid_out, my_pid_in, word, buf, NULL);
                end_error();
            default:
                if (write_automaton_to_mq(desc_out, aut) == -1) end_error();
                if (mq_receive(desc_in, buf, AMOUNT_TO_READ, NULL) == (mqd_t) -1) end_error();
                if (strcmp(buf, ANS_AFF) == 0)
                    info.acc++;
                wait(NULL);
                break;
        }
        mq_close(desc_in);
        mq_close(desc_out);
        desc_in = -1;
        desc_out = -1;
        mq_unlink(my_pid_in);
        mq_unlink(my_pid_out);


        tester_mq_desc_in = mq_open(tester_pid_in, O_WRONLY | O_CREAT, 0660, &spec);
        if (tester_mq_desc_in == (mqd_t) -1) end_error();
        if (mq_send(tester_mq_desc_in, buf, strlen(buf), MSG_PRIO) == -1)
            end_error();
        info.snt++;
        if (mq_close(tester_mq_desc_in) == -1) end_error();
        tester_mq_desc_in = -1;
    }

}

int main() {
    signal(SIG_FLT, sighandle_err_parent);
    parent_pid = getpid();
    int i;
    pid_t pid;

    read_automaton_from_stdin(&aut);

    for (;;) {
        memset(buf, 0, MAX_MSG_LEN);
        val_in_mq_desc = mq_open(VALIDATOR_MAIN_QUEUE_IN, O_RDONLY | O_CREAT, 0660, &spec);
        if (val_in_mq_desc == (mqd_t) -1) end_error();
        if (mq_receive(val_in_mq_desc, buf, AMOUNT_TO_READ, NULL) == -1)
            end_error();
        mq_close(val_in_mq_desc);
        val_in_mq_desc = -1;

        if (is_number(buf)) {
            switch (pid = fork()) {
                case -1:
                    end_error();
                case 0:
                    tester_subprocess(atoi(buf), &aut);
                    return 0;
                default:
                    children_pids[n_children++] = pid;
                    break;
            }
        } else if (strcmp(buf, TERM_MSG) == 0) {

            TesterInfo tester_infos[n_children];
            int rcd_total = 0,
                snt_total = 0,
                acc_total = 0;

            /* kill all children and wait for summaries */
            for (i = 0; i < n_children; i++) {
                memset(buf, 0, MAX_MSG_LEN);
                if (kill(children_pids[i], SIG_END) == -1) end_error();
                end_mq_desc = mq_open(VALIDATOR_END_QUEUE_IN, O_RDONLY | O_CREAT, 0660, NULL);
                if (end_mq_desc == (mqd_t) -1)
                    end_error();

                if (mq_receive(end_mq_desc, buf, AMOUNT_TO_READ, NULL) == -1) end_error();
                mq_close(end_mq_desc);
                end_mq_desc = -1;

                sscanf(buf, "%d %d %d %d", &tester_infos[i].pid,
                                           &tester_infos[i].rcd,
                                           &tester_infos[i].snt,
                                           &tester_infos[i].acc);
                rcd_total += tester_infos[i].rcd;
                snt_total += tester_infos[i].snt;
                acc_total += tester_infos[i].acc;
            }

            printf("Rcd: %d\nSnt: %d\nAcc: %d\n", rcd_total, snt_total, acc_total);
            for (i = 0; i < n_children; i++) {
                printf("PID: %d\nRcd: %d\nAcc: %d\n",
                        tester_infos[i].pid,
                        tester_infos[i].rcd,
                        tester_infos[i].acc);
            }
            

            mq_unlink(VALIDATOR_END_QUEUE_IN);
            mq_unlink(VALIDATOR_MAIN_QUEUE_IN);
            destroy_automaton(&aut);
            break;
        }
    }
    return 0;
}
