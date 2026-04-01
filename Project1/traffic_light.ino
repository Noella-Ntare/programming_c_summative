

#include <Arduino.h>

//Pin Definitions
#define A_RED    2
#define A_YELLOW 3
#define A_GREEN  4
#define B_RED    8
#define B_YELLOW 9
#define B_GREEN  10
#define BTN_A    7
#define BTN_B    11

// Timing (milliseconds) 
#define BASE_GREEN_TIME   5000UL   // normal green = 5 seconds
#define EXTENDED_GREEN    8000UL   // 3+ vehicles = 8 seconds
#define MIN_GREEN_TIME    3000UL   // no vehicles = 3 seconds
#define YELLOW_TIME       2000UL   // always 2 seconds yellow
#define STATUS_INTERVAL   3000UL   // print status every 3 seconds

//Signal State 
typedef enum {
  SIG_RED,
  SIG_YELLOW,
  SIG_GREEN
} SignalState;

// Intersection Structure 

typedef struct {
  const char*   name;           // "A" or "B"
  uint8_t       pinRed;
  uint8_t       pinYellow;
  uint8_t       pinGreen;
  uint8_t       pinButton;
  SignalState   state;          // current light colour
  unsigned long stateStart;     // when current state began (ms)
  unsigned long greenDuration;  // how long to stay green (ms)
  uint16_t      vehicleCount;   // vehicles detected this cycle
  bool          vehicleWaiting; // is button currently held?
  bool          manualOverride; // set true via serial command
} Intersection;

//Globals 
Intersection  intersections[2];
uint8_t       activeIntersection = 0;
unsigned long lastStatusPrint    = 0;
bool          systemRunning      = true;

//Function Prototypes
void          initIntersection(Intersection* inter, const char* name,
                               uint8_t r, uint8_t y, uint8_t g, uint8_t btn);
void          setSignal(Intersection* inter, SignalState newState);
void          updateIntersection(Intersection* inter, uint8_t idx);
void          checkButtons();
void          printStatus();
void          handleSerial();
void          logEvent(const char* event, const char* intersection);
unsigned long calcGreenTime(Intersection* inter);

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  initIntersection(&intersections[0], "A", A_RED, A_YELLOW, A_GREEN, BTN_A);
  initIntersection(&intersections[1], "B", B_RED, B_YELLOW, B_GREEN, BTN_B);

  // A starts GREEN, B starts RED
  setSignal(&intersections[0], SIG_GREEN);
  setSignal(&intersections[1], SIG_RED);

  Serial.println(F("=== Smart Traffic Light Controller ==="));
  Serial.println(F("Commands: s=status | a=force A green | b=force B green | q=stop"));
  logEvent("SYSTEM_START", "ALL");
}


void loop() {
  if (!systemRunning) return;

  checkButtons();
  updateIntersection(&intersections[0], 0);
  updateIntersection(&intersections[1], 1);
  handleSerial();

  if (millis() - lastStatusPrint >= STATUS_INTERVAL) {
    printStatus();
    lastStatusPrint = millis();
  }
}


// Initialise one intersection 
void initIntersection(Intersection* inter, const char* name,
                      uint8_t r, uint8_t y, uint8_t g, uint8_t btn) {
  inter->name           = name;
  inter->pinRed         = r;
  inter->pinYellow      = y;
  inter->pinGreen       = g;
  inter->pinButton      = btn;
  inter->state          = SIG_RED;
  inter->stateStart     = 0;
  inter->greenDuration  = BASE_GREEN_TIME;
  inter->vehicleCount   = 0;
  inter->vehicleWaiting = false;
  inter->manualOverride = false;

  pinMode(r,   OUTPUT);
  pinMode(y,   OUTPUT);
  pinMode(g,   OUTPUT);
  pinMode(btn, INPUT_PULLUP);   

  // All LEDs off at start
  digitalWrite(r, LOW);
  digitalWrite(y, LOW);
  digitalWrite(g, LOW);
}


void setSignal(Intersection* inter, SignalState newState) {
  digitalWrite(inter->pinRed,    LOW);
  digitalWrite(inter->pinYellow, LOW);
  digitalWrite(inter->pinGreen,  LOW);

  inter->state      = newState;
  inter->stateStart = millis();

  switch (newState) {
    case SIG_RED:    digitalWrite(inter->pinRed,    HIGH); break;
    case SIG_YELLOW: digitalWrite(inter->pinYellow, HIGH); break;
    case SIG_GREEN:  digitalWrite(inter->pinGreen,  HIGH); break;
  }

  char buf[48];
  snprintf(buf, sizeof(buf), "INT_%s -> %s",
           inter->name,
           newState == SIG_RED    ? "RED"    :
           newState == SIG_YELLOW ? "YELLOW" : "GREEN");
  logEvent(buf, inter->name);
}

//Dynamic green time based on traffic volume
unsigned long calcGreenTime(Intersection* inter) {
  if (inter->vehicleCount >= 3) return EXTENDED_GREEN;
  if (inter->vehicleCount == 0) return MIN_GREEN_TIME;
  return BASE_GREEN_TIME;
}


void updateIntersection(Intersection* inter, uint8_t idx) {
  unsigned long elapsed = millis() - inter->stateStart;

  if (inter->state == SIG_GREEN) {
    inter->greenDuration = calcGreenTime(inter);
    if (elapsed >= inter->greenDuration)
      setSignal(inter, SIG_YELLOW);          // time's up → go yellow
  }
  else if (inter->state == SIG_YELLOW) {
    if (elapsed >= YELLOW_TIME) {
      setSignal(inter, SIG_RED);             // yellow done → go red
      inter->vehicleCount   = 0;
      inter->vehicleWaiting = false;

      activeIntersection = 1 - idx;          // hand over to other light
      setSignal(&intersections[activeIntersection], SIG_GREEN);
    }
  }
}

// ── Read push buttons — each press = one vehicle detected ─────────
void checkButtons() {
  for (uint8_t i = 0; i < 2; i++) {
    if (digitalRead(intersections[i].pinButton) == LOW) {
      if (!intersections[i].vehicleWaiting) {
        intersections[i].vehicleWaiting = true;
        intersections[i].vehicleCount++;

        char buf[32];
        snprintf(buf, sizeof(buf), "VEHICLE_DETECTED (cnt=%u)",
                 intersections[i].vehicleCount);
        logEvent(buf, intersections[i].name);
      }
    } else {
      intersections[i].vehicleWaiting = false;
    }
  }
}

// ── Print current status to serial ───────────────────────────────
void printStatus() {
  Serial.println(F("--- System Status ---"));
  for (uint8_t i = 0; i < 2; i++) {
    Intersection* inter = &intersections[i];
    Serial.print(F("  Intersection ")); Serial.print(inter->name);
    Serial.print(F(": "));
    switch (inter->state) {
      case SIG_RED:    Serial.print(F("RED   ")); break;
      case SIG_YELLOW: Serial.print(F("YELLOW")); break;
      case SIG_GREEN:  Serial.print(F("GREEN ")); break;
    }
    Serial.print(F(" | Vehicles: ")); Serial.print(inter->vehicleCount);
    Serial.print(F(" | GreenTime: ")); Serial.print(inter->greenDuration / 1000);
    Serial.println(F("s"));
  }
}

// ── Handle serial commands ────────────────────────────────────────
// s = status | a = force A green | b = force B green | q = stop
void handleSerial() {
  if (!Serial.available()) return;
  char cmd = Serial.read();

  switch (cmd) {
    case 's': case 'S':
      printStatus();
      break;
    case 'a': case 'A':
      Serial.println(F("[OVERRIDE] Forcing Intersection A GREEN"));
      setSignal(&intersections[0], SIG_GREEN);
      setSignal(&intersections[1], SIG_RED);
      activeIntersection = 0;
      break;
    case 'b': case 'B':
      Serial.println(F("[OVERRIDE] Forcing Intersection B GREEN"));
      setSignal(&intersections[1], SIG_GREEN);
      setSignal(&intersections[0], SIG_RED);
      activeIntersection = 1;
      break;
    case 'q': case 'Q':
      Serial.println(F("[SYSTEM] Stopping all signals."));
      setSignal(&intersections[0], SIG_RED);
      setSignal(&intersections[1], SIG_RED);
      systemRunning = false;
      break;
  }
}

// ── Log an event to Serial with timestamp ────────────────────────
void logEvent(const char* event, const char* intersection) {
  Serial.print(F("[LOG t="));
  Serial.print(millis());
  Serial.print(F("ms INT="));
  Serial.print(intersection);
  Serial.print(F("] "));
  Serial.println(event);
}