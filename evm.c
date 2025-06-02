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

// Function Prototypes
void load_candidates();
void save_votes();
void display_candidates();
int authenticate_voter();
void voting_session();
void confirm_vote();
void record_vote();
void display_results();
void reset_session();
void feedback(const char* message);
void timeout_check();
void log_vote_time(int voter_index);
int find_voter(const char *id);

int main() {
    load_candidates();
    printf("Welcome to Virtual EVM Simulator\n");
    while (1) {
        timeout_check();
        switch (current_state) {
            case STATE_IDLE:
                printf("\n--- EVM Idle ---\n");
                printf("1. Start Voting\n2. Display Results\n3. Reset Session\n4. Exit\nEnter choice: ");
                int choice;
                scanf("%d", &choice);
                switch (choice) {
                    case 1:
                        current_voter = authenticate_voter();
                        if (current_voter != -1) {
                            current_state = STATE_VOTING;
                            last_action_time = time(NULL);
                        }
                        break;
                    case 2:
                        current_state = STATE_RESULT_DISPLAY;
                        break;
                    case 3:
                        reset_session();
                        break;
                    case 4:
                        save_votes();
                        exit(0);
                    default:
                        feedback("Invalid choice.");
                }
                break;
            case STATE_VOTING:
                voting_session();
                break;
            case STATE_CONFIRMATION:
                confirm_vote();
                break;
            case STATE_VOTE_RECORDED:
                record_vote();
                current_state = STATE_IDLE;
                break;
            case STATE_RESULT_DISPLAY:
                display_results();
                current_state = STATE_IDLE;
                break;
            case STATE_LOCKED:
                feedback("EVM is locked. Please reset session.");
                current_state = STATE_IDLE;
                break;
        }
    }
    return 0;
}

// Load candidate data (hardcoded for demo)
void load_candidates() {
    num_candidates = 4; // 4 candidates + NOTA
    strcpy(candidates[0].name, "Candidate A");
    strcpy(candidates[1].name, "Candidate B");
    strcpy(candidates[2].name, "Candidate C");
    strcpy(candidates[3].name, "Candidate D");
    strcpy(candidates[4].name, "None of the Above (NOTA)");
    for (int i = 0; i <= num_candidates; i++) candidates[i].votes = 0;
}

// Save votes (simulate secure storage)
void save_votes() {
    FILE *f = fopen("votes.dat", "w");
    for (int i = 0; i <= num_candidates; i++)
        fprintf(f, "%s,%d\n", candidates[i].name, candidates[i].votes);
    fclose(f);
}

// Display candidates
void display_candidates() {
    printf("\n--- Candidates ---\n");
    for (int i = 0; i <= num_candidates; i++)
        printf("%d. %s\n", i + 1, candidates[i].name);
}

// Find voter index by ID
int find_voter(const char *id) {
    for (int i = 0; i < num_voters; i++) {
        if (strcmp(voters[i].voter_id, id) == 0)
            return i;
    }
    return -1;
}

// Authenticate voter (accept any unique ID)
int authenticate_voter() {
    char id[MAX_ID_LEN];
    printf("Enter Voter ID: ");
    scanf("%s", id);

    int idx = find_voter(id);
    if (idx != -1) {
        if (voters[idx].has_voted) {
            feedback("You have already voted.");
            return -1;
        }
        feedback("Welcome back! You may vote.");
        last_action_time = time(NULL);
        return idx;
    } else {
        if (num_voters < MAX_VOTERS) {
            strcpy(voters[num_voters].voter_id, id);
            voters[num_voters].has_voted = 0;
            voters[num_voters].vote_time = 0;
            feedback("New voter registered. You may vote.");
            last_action_time = time(NULL);
            return num_voters++;
        } else {
            feedback("Voter limit reached.");
            return -1;
        }
    }
}

// Voting session
void voting_session() {
    display_candidates();
    printf("Select candidate (1-%d): ", num_candidates + 1);
    scanf("%d", &selected_candidate);
    if (selected_candidate < 1 || selected_candidate > num_candidates + 1) {
        feedback("Invalid selection.");
        return;
    }
    selected_candidate--; // zero-based index
    current_state = STATE_CONFIRMATION;
    last_action_time = time(NULL);
}

// Confirm vote
void confirm_vote() {
    printf("You selected: %s\n", candidates[selected_candidate].name);
    printf("1. Confirm\n2. Change Selection\n3. Cancel\nEnter choice: ");
    int choice;
    scanf("%d", &choice);
    switch (choice) {
        case 1:
            current_state = STATE_VOTE_RECORDED;
            break;
        case 2:
            current_state = STATE_VOTING;
            break;
        case 3:
            current_state = STATE_IDLE;
            break;
        default:
            feedback("Invalid choice.");
    }
    last_action_time = time(NULL);
}

// Record vote
void record_vote() {
    candidates[selected_candidate].votes++;
    voters[current_voter].has_voted = 1;
    log_vote_time(current_voter);
    feedback("Vote recorded successfully!");
    last_action_time = time(NULL);
}

// Display results
void display_results() {
    printf("\n--- Voting Results ---\n");
    int max_votes = 0, winner = -1, tie = 0;
    for (int i = 0; i <= num_candidates; i++) {
        printf("%s: %d votes\n", candidates[i].name, candidates[i].votes);
        if (candidates[i].votes > max_votes) {
            max_votes = candidates[i].votes;
            winner = i;
            tie = 0;
        } else if (candidates[i].votes == max_votes && max_votes != 0) {
            tie = 1;
        }
    }
    if (max_votes == 0) {
        printf("No votes cast yet.\n");
    } else if (tie) {
        printf("Result: Tie detected.\n");
    } else {
        printf("Winner: %s\n", candidates[winner].name);
    }
}

// Reset session and clear votes
void reset_session() {
    for (int i = 0; i <= num_candidates; i++) candidates[i].votes = 0;
    num_voters = 0; // Clear all registered voters
    feedback("Session reset. All votes and voter data cleared.");
}

// Feedback (simulate LED/sound)
void feedback(const char* message) {
    printf("[Feedback]: %s\n", message);
}

// Timeout check
void timeout_check() {
    if (current_state != STATE_IDLE && difftime(time(NULL), last_action_time) > TIMEOUT_SECONDS) {
        feedback("Session timed out. Returning to Idle state.");
        current_state = STATE_IDLE;
    }
}

// Log voting timestamp
void log_vote_time(int voter_index) {
    voters[voter_index].vote_time = time(NULL);
    char* timestamp = ctime(&voters[voter_index].vote_time);
    timestamp[strlen(timestamp)-1] = '\0'; // Remove newline
    printf("Vote timestamp: %s\n", timestamp);
}
