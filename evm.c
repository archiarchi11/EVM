// === State machine states ===
enum State {
  STATE_IDLE,
  STATE_VOTING,
  STATE_CONFIRMATION,
  STATE_VOTE_RECORDED,
  STATE_RESULT_DISPLAY,
  STATE_LOCKED
};

State currentState = STATE_IDLE;
bool stateJustEntered = true;

// === Button pins ===
const int candidate1Btn = 2;
const int candidate2Btn = 3;
const int candidate3Btn = 4;
const int notaBtn       = 5;
const int confirmBtn    = 6;
const int resetBtn      = 7;

// === Output pins ===
const int greenLED = 8;
const int redLED   = 9;
const int buzzerPin = 10;

// === Vote storage: [C1, C2, C3, NOTA] ===
int votes[4] = {0, 0, 0, 0};

// === User state tracking ===
bool hasVoted = false;
int selectedCandidate = -1;

// === Timeout setup ===
unsigned long lastActivity = 0;
const unsigned long TIMEOUT = 30000; // 30 seconds
bool timeoutHandled = false;

void setup() {
  Serial.begin(9600);

  pinMode(candidate1Btn, INPUT);
  pinMode(candidate2Btn, INPUT);
  pinMode(candidate3Btn, INPUT);
  pinMode(notaBtn, INPUT);
  pinMode(confirmBtn, INPUT);
  pinMode(resetBtn, INPUT);

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.println("EVM Initialized. Ready to vote.");
}

bool waitForButtonPress(int pin) {
  if (digitalRead(pin) == HIGH) {
    delay(50); // debounce
    while (digitalRead(pin) == HIGH);
    delay(50);
    return true;
  }
  return false;
}

void loop() {
  if ((millis() - lastActivity > TIMEOUT) && currentState != STATE_IDLE && !timeoutHandled) {
    Serial.println("Timeout occurred. Returning to IDLE.");
    digitalWrite(redLED, HIGH);
    tone(buzzerPin, 400);
    delay(1000);  // Longer warning
    noTone(buzzerPin);
    digitalWrite(redLED, LOW);
    selectedCandidate = -1;
    currentState = STATE_IDLE;
    stateJustEntered = true;
    timeoutHandled = true; // avoid repeated alerts
    return;
  }

  switch (currentState) {
    case STATE_IDLE:            handleIdle(); break;
    case STATE_VOTING:          handleVoting(); break;
    case STATE_CONFIRMATION:   handleConfirmation(); break;
    case STATE_VOTE_RECORDED:  handleVoteRecorded(); break;
    case STATE_RESULT_DISPLAY: handleResultDisplay(); break;
    case STATE_LOCKED:         handleLocked(); break;
  }

  delay(50); // stability
}

// === STATE HANDLERS ===

void handleIdle() {
  if (stateJustEntered) {
    digitalWrite(redLED, LOW);
    Serial.println("Press any button to start voting...");
    timeoutHandled = false;
    stateJustEntered = false;
  }

  if (hasVoted) {
    Serial.println("Vote already cast. System locked.");
    for (int i = 0; i < 6; i++) {
      digitalWrite(redLED, HIGH);
      delay(250);
      digitalWrite(redLED, LOW);
      delay(250);
    }

    tone(buzzerPin, 500);
    delay(300);
    noTone(buzzerPin);

    currentState = STATE_LOCKED;
    stateJustEntered = true;
    return;
  }

  // Just wait for ANY button to begin the voting process
  if (waitForButtonPress(candidate1Btn) || waitForButtonPress(candidate2Btn) ||
      waitForButtonPress(candidate3Btn) || waitForButtonPress(notaBtn)) {
    Serial.println("Voting started. Please select your candidate.");
    tone(buzzerPin, 800);
    delay(100);
    noTone(buzzerPin);
    currentState = STATE_VOTING;
    stateJustEntered = true;
    lastActivity = millis();
  }
}

void handleVoting() {
  if (stateJustEntered) {
    Serial.println("Select a candidate (1–3) or NOTA...");
    selectedCandidate = -1;
    stateJustEntered = false;
  }

  if (waitForButtonPress(candidate1Btn)) selectedCandidate = 0;
  else if (waitForButtonPress(candidate2Btn)) selectedCandidate = 1;
  else if (waitForButtonPress(candidate3Btn)) selectedCandidate = 2;
  else if (waitForButtonPress(notaBtn))       selectedCandidate = 3;

  if (selectedCandidate != -1) {
    Serial.print("Selected: ");
    Serial.println(selectedCandidate == 3 ? "NOTA" : "Candidate " + String(selectedCandidate + 1));
    tone(buzzerPin, 1000);
    delay(100);
    noTone(buzzerPin);
    currentState = STATE_CONFIRMATION;
    stateJustEntered = true;
    lastActivity = millis();
  }
}

void handleConfirmation() {
  if (stateJustEntered) {
    Serial.println("Press CONFIRM to record your vote.");
    stateJustEntered = false;
  }

  if (waitForButtonPress(confirmBtn)) {
    votes[selectedCandidate]++;
    hasVoted = true;

    digitalWrite(greenLED, HIGH);
    tone(buzzerPin, 1500);
    delay(200);
    noTone(buzzerPin);
    Serial.println("Vote recorded successfully.");
    delay(1000);
    digitalWrite(greenLED, LOW);

    currentState = STATE_VOTE_RECORDED;
    stateJustEntered = true;
    lastActivity = millis();
  }
}

void handleVoteRecorded() {
  if (stateJustEntered) {
    Serial.println("Thank you! Returning to IDLE...");
    stateJustEntered = false;
  }

  selectedCandidate = -1;
  currentState = STATE_IDLE;
  stateJustEntered = true;
}

void handleResultDisplay() {
  if (stateJustEntered) {
    Serial.println("=== Voting Results ===");
    for (int i = 0; i < 4; i++) {
      if (i == 3) Serial.print("NOTA: ");
      else Serial.print("Candidate ");
      if (i != 3) Serial.print(i + 1);
      Serial.print(" - Votes: ");
      Serial.println(votes[i]);
    }

    int maxVotes = 0;
    int winner = -1;
    bool tie = false;

    for (int i = 0; i < 4; i++) {
      if (votes[i] > maxVotes) {
        maxVotes = votes[i];
        winner = i;
        tie = false;
      } else if (votes[i] == maxVotes && maxVotes > 0) {
        tie = true;
      }
    }

    if (tie) Serial.println("Result: It's a tie.");
    else if (winner == 3) Serial.println("Winner: NOTA");
    else Serial.println("Winner: Candidate " + String(winner + 1));

    stateJustEntered = false;
  }

  currentState = STATE_IDLE;
  stateJustEntered = true;
}

void handleLocked() {
  if (stateJustEntered) {
    Serial.println("System locked. Press RESET to clear votes.");
    tone(buzzerPin, 400);
    delay(200);
    noTone(buzzerPin);
    delay(200);
    stateJustEntered = false;
  }

  if (waitForButtonPress(resetBtn)) {
    for (int i = 0; i < 4; i++) votes[i] = 0;
    hasVoted = false;
    selectedCandidate = -1;
    Serial.println("System reset. Returning to IDLE.");

    tone(buzzerPin, 1000);
    delay(100);
    noTone(buzzerPin);
    tone(buzzerPin, 1500);
    delay(100);
    noTone(buzzerPin);

    currentState = STATE_IDLE;
    stateJustEntered = true;
  }
}
