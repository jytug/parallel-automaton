#pragma once

#include <signal.h>

#define VALIDATOR_MAIN_QUEUE_IN "/validator_main_in_mq"
#define VALIDATOR_END_QUEUE_IN "/validator_end_in_mq"

#define TERM_MSG "!"
#define MSG_PRIO 1
#define TERM_PRIO 2
#define SIG_END SIGUSR1
#define SIG_FLT SIGUSR2
