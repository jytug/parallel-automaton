#pragma once

#include <mqueue.h>

typedef struct {
    int descr_size;      /* size of the auto's description */
    int alpha_size;      /* size of the alphabet */
    int n_states;        /* number of states */
    int n_univ;          /* number of universal states */
    int n_accept;        /* number of accepting states */

    int init_state;      /* initial state */
    int *acc_states;     /* accepting states */

    int **trans_lengths; /* sizes for each transition */
    int ***transitions;  /* encoded transition function */
} Automaton;

/* read and destruct */
int read_automaton_from_stdin(Automaton *);
void destroy_automaton(Automaton *);

/* helpers to pass an automaton via a message queue */
int read_automaton_from_mq(mqd_t desc, Automaton *aut);
int write_automaton_to_mq(mqd_t desc, Automaton *aut);

/* automaton logic functions */
int is_accepting(int state, const Automaton *aut);
int is_universal(int state, const Automaton *aut);

/* pretty print (for debugging purposes) */
void pretty_print_automaton(Automaton);
