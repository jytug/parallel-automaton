#include "automaton.h"
#include "config.h"
#include "err.h"
#include "in_out.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_automaton_from_stdin(Automaton *aut) {
    int N, A, Q, U, F,
        q_init;

    int j;

    char line[MAX_LINE_LEN];

    if (read_line(line) == NULL) goto err;
    if (sscanf(line, "%d%d%d%d%d", &N, &A, &Q, &U, &F) != 5) goto err;

    aut->descr_size = N;
    aut->alpha_size = A;
    aut->n_states = Q;
    aut->n_univ = U;
    aut->n_accept = F;

    if (read_line(line) == NULL) goto err;
    if (sscanf(line, "%d", &q_init) != 1) goto err;

    aut->init_state = q_init;

    if (read_line(line) == NULL) goto err;
    if (strlen(line) == 0) {
        aut->acc_states = 0;
    } else {
        aut->acc_states = calloc(F, sizeof(int));
        aut->acc_states[0] = atoi(strtok(line, " "));
        for (j = 1; j < F; j++) {
            aut->acc_states[j] = atoi(strtok(NULL, " "));
        }
    }

    aut->trans_lengths = calloc(Q, sizeof(int *));
    aut->transitions = calloc(Q, sizeof(int **));

    for (j = 0; j < N - 3; j++) {
        int p;
        char r;
        char *dummy;

        if (read_line(line) == NULL) goto err;

        p = atoi(strtok(line, " "));
        r = strtok(NULL, " ")[0];

        if (aut->trans_lengths[p] == NULL)
            aut->trans_lengths[p] = calloc(A, sizeof(int));
        if (aut->transitions[p] == NULL)
            aut->transitions[p] = calloc(A, sizeof(int *));
        if (aut->transitions[p][encode_char(r)] == NULL)
            aut->transitions[p][encode_char(r)] = calloc(Q, sizeof(int));


        int trans_size = 0;
        while ((dummy = strtok(NULL, " "))) {
            aut->transitions[p][encode_char(r)][trans_size++] = atoi(dummy);
        }

        if (trans_size <= 0)
            goto err;

        /* reallocate the memory (unnecessary, for memory efficiency) */
        aut->transitions[p][encode_char(r)] = realloc(
                aut->transitions[p][encode_char(r)],
                trans_size * sizeof(int)
            );

        aut->trans_lengths[p][encode_char(r)] = trans_size;
    }

    return 0;

err:
    destroy_automaton(aut);
    return -1;
}

int read_automaton_from_mq(mqd_t desc, Automaton *aut) {
    int N, A, Q, U, F,
        q_init;

    int j;

    char line[MAX_LINE_LEN];
    if (read_line_from_mq(desc, line) == NULL) goto err;
    if (sscanf(line, "%d%d%d%d%d", &N, &A, &Q, &U, &F) != 5) goto err;

    aut->descr_size = N;
    aut->alpha_size = A;
    aut->n_states = Q;
    aut->n_univ = U;
    aut->n_accept = F;

    if (read_line_from_mq(desc, line) == NULL) goto err;
    if (sscanf(line, "%d", &q_init) != 1) goto err;

    aut->init_state = q_init;

    if (read_line_from_mq(desc, line) == NULL) goto err;
    if (strlen(line) == 0) {
        aut->acc_states = 0;
    } else {
        aut->acc_states = calloc(F, sizeof(int));
        aut->acc_states[0] = atoi(strtok(line, " "));
        for (j = 1; j < F; j++) {
            aut->acc_states[j] = atoi(strtok(NULL, " "));
        }
    }

    aut->trans_lengths = calloc(Q, sizeof(int *));
    aut->transitions = calloc(Q, sizeof(int **));

    for (j = 0; j < N - 3; j++) {
        int p;
        char r;
        char *dummy;

        if (read_line_from_mq(desc, line) == NULL) goto err;

        p = atoi(strtok(line, " "));
        r = strtok(NULL, " ")[0];

        if (aut->trans_lengths[p] == NULL)
            aut->trans_lengths[p] = calloc(A, sizeof(int));
        if (aut->transitions[p] == NULL)
            aut->transitions[p] = calloc(A, sizeof(int *));
        if (aut->transitions[p][encode_char(r)] == NULL)
            aut->transitions[p][encode_char(r)] = calloc(Q, sizeof(int));


        int trans_size = 0;
        while ((dummy = strtok(NULL, " "))) {
            aut->transitions[p][encode_char(r)][trans_size++] = atoi(dummy);
        }

        if (trans_size <= 0)
            goto err;

        /* reallocate the memory (unnecessary, for memory efficiency) */
        aut->transitions[p][encode_char(r)] = realloc(
                aut->transitions[p][encode_char(r)],
                trans_size * sizeof(int)
            );

        aut->trans_lengths[p][encode_char(r)] = trans_size;
    }

    return 0;

err:
    destroy_automaton(aut);
    return -1;
}

int write_automaton_to_mq(mqd_t desc, Automaton* aut) {
    int i, j, k, offset;
    char line[MAX_LINE_LEN];
    memset(line, 0, MAX_LINE_LEN);
    sprintf(line, "%d %d %d %d %d",
            aut->descr_size,
            aut->alpha_size,
            aut->n_states,
            aut->n_univ,
            aut->n_accept);
    if (mq_send(desc, line, strlen(line), MSG_PRIO) == (mqd_t) -1)
        return -1;

    memset(line, 0, MAX_LINE_LEN);
    sprintf(line, "%d", aut->init_state);
    if (mq_send(desc, line, strlen(line), MSG_PRIO) == (mqd_t) -1)
        return -1;

    offset = 0;
    memset(line, 0, MAX_LINE_LEN);
    for (i = 0; i < aut->n_accept; i++)
        offset += sprintf(line + offset, "%d ", aut->acc_states[i]);

    if (mq_send(desc, line, strlen(line), MSG_PRIO) == (mqd_t) -1)
        return -1;

    for (i = 0; i < aut->n_states; i++) {
        for (j = 0; j < aut->alpha_size; j++) {
            if (aut->transitions[i][j] == NULL)
                continue;

            offset = 0;
            memset(line, 0, MAX_LINE_LEN);
            offset += sprintf(line, "%d %c ", i, decode_char(j));
            for (k = 0; k < aut->trans_lengths[i][j]; k++) {
                offset += sprintf(line + offset, "%d ", aut->transitions[i][j][k]);
            }
            if (mq_send(desc, line, strlen(line), MSG_PRIO) == (mqd_t) -1)
                return -1;
        }
    }
    return 0;
}

void destroy_automaton(Automaton *aut) {
    int i, j;

    if (aut->acc_states != NULL)
        free(aut->acc_states);
    aut->acc_states = NULL;

    if (aut->trans_lengths != NULL) {
        for (i = 0; i < aut->n_states; i++) {
            free(aut->trans_lengths[i]); 
            aut->trans_lengths[i] = NULL;
        }
    }
    free(aut->trans_lengths);
    aut->trans_lengths = NULL;

    if (aut->transitions != NULL) {
        for (i = 0; i < aut->n_states; i++) {
            if (aut->transitions[i] != NULL) {
                for (j = 0; j < aut->alpha_size; j++) {
                    free(aut->transitions[i][j]); 
                    aut->transitions[i][j] = NULL;
                }
            }
            free(aut->transitions[i]);
            aut->transitions[i] = NULL;
        }
    }
    free(aut->transitions);
    aut->transitions = NULL;
}

int is_accepting(int state, const Automaton *aut) {
    int i;
    for (i = 0; i < aut->n_accept; i++)
        if (aut->acc_states[i] == state)
            return 1;

    return 0;
}

int is_universal(int state, const Automaton *aut) {
    return state < aut->n_univ;
}

void pretty_print_automaton(Automaton aut) {
    int i, j, k;
    printf("=================================================\n");
    printf("Automaton description:\n");
    printf("States:\t\t\t0..%d\n", aut.n_states - 1);
    printf("Alphabet:\t\ta..%c\n", 'a' + (aut.alpha_size - 1));
    printf("Initial state:\t\t%d\n", aut.init_state);
    printf("No. univ. states:\t%d\n", aut.n_univ);
    printf("No. acc. states:\t%d\n", aut.n_accept);
    printf("Accepting states:\t{");
    for (i = 0; i < aut.n_accept; i++) {
        printf("%d%s", aut.acc_states[i], i == aut.n_accept - 1 ? "}\n" : ", ");
    }
    printf("Transitions:\n");
    for (i = 0; i < aut.n_states; i++) {
        if (aut.transitions[i] == NULL)
            continue;

        for (j = 0; j < aut.alpha_size; j++) {
            if (aut.transitions[i][j] == NULL)
                continue;

            printf("\t\t\t(%d, %c) -> {", i, decode_char(j));
            for (k = 0; k < aut.trans_lengths[i][j]; k++) {
                printf("%d%s", aut.transitions[i][j][k],
                               k == aut.trans_lengths[i][j] - 1 ? "}\n" : ", ");
            }
        } 

    }
    printf("=================================================\n");

}
