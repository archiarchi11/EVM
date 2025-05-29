#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_CANDIDATES 5
#define MAX_VOTERS 100
#define MAX_ID_LEN 50
#define TIMEOUT_SECONDS 30

typedef enum {
    STATE_IDLE,
    STATE_VOTING,
    STATE_CONFIRMATION,
    STATE_VOTE_RECORDED,
    STATE_RESULT_DISPLAY,
    STATE_LOCKED
} State;

typedef struct {
    char name[50];
    int votes;
} Candidate;

typedef struct {
    char voter_id[MAX_ID_LEN];
    int has_voted;
    time_t vote_time;
} Voter;

Candidate candidates[MAX_CANDIDATES + 1]; // +1 for NOTA
Voter voters[MAX_VOTERS];
int num_candidates = 0, num_voters = 0;
State current_state = STATE_IDLE;
int current_voter = -1, selected_candidate = -1;
time_t last_action_time = 0;

