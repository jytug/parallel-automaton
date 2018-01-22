#include "automaton.h"
#include "config.h"
#include "err.h"
#include "in_out.h"

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

/* global variables */
mqd_t desc_in = -1,
      desc_out = -1;

char *mq_name_in;
char *mq_name_out;
char *word;

int word_len;
struct mq_attr spec = {.mq_maxmsg = 4, .mq_msgsize = MAX_MSG_LEN - 1 };

int val_main_pid;

void usage_err() {
    fprintf(stderr, "Usage: run <mq_name_in> <mq_name_out> <word> <parent_pid>");
    exit(1);
}

void cleanup() {
    if (desc_in != -1)
        mq_close(desc_in);
    if (desc_out != -1)
        mq_close(desc_out);
}

void exit_err() {
    kill(val_main_pid, SIG_FLT);
    exit(1);
}

int accepts(Automaton *aut, int state, char *word, int word_len, int pos) {
    char buf[MAX_PIPE_MSG_LEN];
    memset(buf, 0, MAX_PIPE_MSG_LEN);

    int i,
        current_char = encode_char(word[pos]);

    if (pos == word_len)
        return is_accepting(state, aut);

    int n_children = aut->trans_lengths[state][current_char];
    int pipes[n_children][2];
    int pids[n_children];

    for (i = 0; i < n_children; i++) {
        if (pipe(pipes[i]) == -1) exit_err();
        
        switch (pids[i] = fork()) {
            case -1:
                exit_err();
            case 0: ;
                if (close(pipes[i][0]) == -1) exit_err();
                int result = accepts(aut, aut->transitions[state][current_char][i],
                                     word, word_len, pos+1);

                sprintf(buf, "%d", result);
                int written;
                if ((written = write(pipes[i][1], buf, strlen(buf))) != strlen(buf)) exit_err();
                if (close(pipes[i][1]) == -1) exit_err();
                exit(0);
            default:
                if (close(pipes[i][1]) == -1) exit_err();
                break;
        }
    }

    int result = is_universal(state, aut);
    for (i = 0; i < n_children; i++) {
        waitpid(pids[i], NULL, 0);
        memset(buf, 0, MAX_PIPE_MSG_LEN);
        int partial_result;
        int bytes_read = 0;
        int read_once = 0;
        while ((bytes_read += (read_once = read(pipes[i][0], buf, MAX_PIPE_MSG_LEN))) == 0)
            if (read_once == -1)
               exit_err();
        if (bytes_read == 0) fatal("unexpected end-of-file");
        buf[bytes_read] = '\0';
        if (sscanf(buf, "%d", &partial_result) != 1) fatal("unexpected message");

        if (is_universal(state, aut))
            result = result && partial_result;
        else
            result = result || partial_result;
        if (close(pipes[i][0]) == -1) exit_err();
    }
    return result;
}

int main(int argc, char *argv[]) {
    Automaton aut;
    if (argc != 5) {
        usage_err();
    }

    mq_name_in = argv[1];
    mq_name_out = argv[2];
    word = argv[3];
    word_len = strlen(word);
    val_main_pid = atoi(argv[4]);

    /* get the description of the automaton */
    desc_in = mq_open(mq_name_in, O_RDONLY | O_CREAT, 0660, &spec);
    if (desc_in == (mqd_t) -1) exit_err();

    read_automaton_from_mq(desc_in, &aut);
    if (mq_close(desc_in) == -1) exit_err();
    desc_in = -1;
    
    int result = accepts(&aut, aut.init_state, word, word_len, 0);

    desc_out = mq_open(mq_name_out, O_WRONLY | O_CREAT, 0660, &spec);
    if (desc_out == (mqd_t) -1) exit_err();

    char *response = result ? ANS_AFF : ANS_NEG;

    if (mq_send(desc_out, response, strlen(response), MSG_PRIO) == -1) exit_err();
    if (mq_close(desc_out) == -1) exit_err();
    desc_out = -1;

    destroy_automaton(&aut);
    return 0;
}
