const int REED_PIN = 4;
const unsigned long DEBOUNCE_MS = 50;

int lastReading = HIGH;
int stableState = HIGH;
unsigned long lastDebounceTs = 0;

void setup() {
  Serial.begin(115200);
  pinMode(REED_PIN, INPUT_PULLUP);
  delay(50);
  lastReading = digitalRead(REED_PIN);
  stableState = lastReading;
  Serial.printf("Starting. initial state=%s\n", stableState == LOW ? "OPEN" : "CLOSED");
}

void loop() {
  int reading = digitalRead(REED_PIN);

  if (reading != lastReading) {
    // input changed — start debounce timer
    lastDebounceTs = millis();
  }

  if ((millis() - lastDebounceTs) > DEBOUNCE_MS) {
    // If the stable state has changed, update and emit event
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) {
        Serial.printf("[%lu] Door: OPEN\n", millis());
      } else {
        Serial.printf("[%lu] Door: CLOSED\n", millis());
      }
    }
  }

  lastReading = reading;
  delay(10);
}