//SMART TRAFFIC LIGHT SYSTEM

#define RED1 2
#define YEL1 3
#define GRN1 4
#define BTN1 7

#define RED2 8
#define YEL2 9
#define GRN2 10
#define BTN2 11

enum State { RED, YELLOW, GREEN };

//STRUCT
struct Intersection {
  int red, yellow, green, button;
  State state;
  unsigned long startTime;
  int vehicleCount;
  bool manualMode;
};


Intersection int1 = {RED1, YEL1, GRN1, BTN1, GREEN, 0, 0, false};
Intersection int2 = {RED2, YEL2, GRN2, BTN2, RED, 0, 0, false};

unsigned long greenTime = 2000;   // shortened for testing
unsigned long yellowTime = 1000;

unsigned long lastPress1 = 0;
unsigned long lastPress2 = 0;

bool isInt1Active = true;
bool systemActive = true;

State int1PrevState = int1.state;
State int2PrevState = int2.state;


void setLights(Intersection &i, State s);
void handleTraffic();
void detectVehicles();
void handleSerial();
void printStatus();
String stateName(State s);
void emergencyStop();
void resetSystem();

//setup
void setup() {
  Serial.begin(9600);

  pinMode(RED1, OUTPUT); pinMode(YEL1, OUTPUT); pinMode(GRN1, OUTPUT);
  pinMode(RED2, OUTPUT); pinMode(YEL2, OUTPUT); pinMode(GRN2, OUTPUT);

  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);

  unsigned long now = millis();

  setLights(int1, GREEN);
  int1.startTime = now;

  setLights(int2, RED);
  int2.startTime = now;

  Serial.println("=== SMART TRAFFIC SYSTEM STARTED ===");
  Serial.println("Commands: M=Manual | E=Emergency | R=Reset");
}

// Loop
void loop() {
  if (systemActive) {
    handleTraffic();
    detectVehicles();
    printStatus();
    printActiveState(); // prints whenever state changes
  }

  handleSerial();
}

//set lights
void setLights(Intersection &i, State s) {
  digitalWrite(i.red, LOW);
  digitalWrite(i.yellow, LOW);
  digitalWrite(i.green, LOW);

  if (s == RED) digitalWrite(i.red, HIGH);
  if (s == YELLOW) digitalWrite(i.yellow, HIGH);
  if (s == GREEN) digitalWrite(i.green, HIGH);

  i.state = s;
  i.startTime = millis();
}

//trafic control
void handleTraffic() {
  unsigned long now = millis();

  //if manual mode is ON
  if (int1.manualMode || int2.manualMode) {
    return;
  }

  if (isInt1Active) {
    if (int1.state == GREEN && now - int1.startTime >= greenTime) {
      setLights(int1, YELLOW);
    }
    else if (int1.state == YELLOW && now - int1.startTime >= yellowTime) {
      setLights(int1, RED);
      setLights(int2, GREEN);
      isInt1Active = false;
    }
  } 
  else {
    if (int2.state == GREEN && now - int2.startTime >= greenTime) {
      setLights(int2, YELLOW);
    }
    else if (int2.state == YELLOW && now - int2.startTime >= yellowTime) {
      setLights(int2, RED);
      setLights(int1, GREEN);
      isInt1Active = true;
    }
  }
}

// detection
void detectVehicles() {
  unsigned long now = millis();

  if (digitalRead(BTN1) == LOW && now - lastPress1 > 200) {
    int1.vehicleCount++;
    lastPress1 = now;
  }

  if (digitalRead(BTN2) == LOW && now - lastPress2 > 200) {
    int2.vehicleCount++;
    lastPress2 = now;
  }

  // Adaptive timing
  if (int1.vehicleCount > 5 || int2.vehicleCount > 5) {
    greenTime = 2000;
  } else {
    greenTime = 1000;
  }
}

// serial control
void handleSerial() {
  if (Serial.available() == 0) return;

  char cmd = Serial.read();

  switch (cmd) {
    case 'M':
    case 'm':
      int1.manualMode = !int1.manualMode;
      int2.manualMode = !int2.manualMode;
      Serial.println("Manual Mode TOGGLED");
      break;

    case 'R':
    case 'r':
      resetSystem();
      break;

    case 'E':
    case 'e':
      emergencyStop();
      break;

    default:
      Serial.println("Invalid Command");
  }
}

// status print
void printStatus() {
  static unsigned long lastPrint = 0;

  if (millis() - lastPrint > 2000) {
    Serial.print("Int1: ");
    Serial.print(stateName(int1.state));
    Serial.print(" | Int2: ");
    Serial.print(stateName(int2.state));
    Serial.print(" | V1: ");
    Serial.print(int1.vehicleCount);
    Serial.print(" | V2: ");
    Serial.println(int2.vehicleCount);

    lastPrint = millis();
  }
}

//active state
void printActiveState() {
  if (int1.state != int1PrevState || int2.state != int2PrevState) {
    Serial.print("Active: ");
    Serial.print(isInt1Active ? "INT1" : "INT2");
    Serial.print(" | State: ");
    Serial.println(isInt1Active ? stateName(int1.state) : stateName(int2.state));

    int1PrevState = int1.state;
    int2PrevState = int2.state;
  }
}


String stateName(State s) {
  if (s == RED) return "RED";
  if (s == YELLOW) return "YELLOW";
  return "GREEN";
}

// emergency stop
void emergencyStop() {
  Serial.println("!!! EMERGENCY STOP ACTIVATED !!!");

  setLights(int1, RED);
  setLights(int2, RED);

  systemActive = false;
}

// reset
void resetSystem() {
  Serial.println("System Reset...");

  systemActive = true;

  int1.vehicleCount = 0;
  int2.vehicleCount = 0;

  setLights(int1, GREEN);
  setLights(int2, RED);

  isInt1Active = true;
  int1PrevState = int1.state;
  int2PrevState = int2.state;
}