const int RELAY_PIN = 15;
const bool ACTIVE_LOW = true;

bool relayOn = false;
unsigned long lastToggle = 0;

void setRelay(bool off) {
  relayOn = off;
  if (ACTIVE_LOW) digitalWrite(RELAY_PIN, off ? LOW : HIGH);
  else            digitalWrite(RELAY_PIN, off ? HIGH : LOW);
  Serial.printf("[%lu] Relay: %s\n", millis(), off ? "ON (solenoid energized)" : "OFF (released)");
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  setRelay(false); // start off
}

void loop() {
  if (millis() - lastToggle >= 2000) {
    lastToggle = millis();
    setRelay(!relayOn);
  }
}