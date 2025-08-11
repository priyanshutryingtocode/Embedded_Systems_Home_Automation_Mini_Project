#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>


#define Bulb1 D1  
#define Bulb2 D3  
#define Bulb3 D4  
#define Bulb4 D5  

// Firebase credentials
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

#define USER_EMAIL "youremail"
#define USER_PASSWORD "yourpassword"

// WiFi credentials
#define ssid "yourssid"
#define password "pass"

// Firebase project credentials
#define FIREBASE_HOST "yourhost"
#define FIREBASE_AUTH "yourtoken"

// Appliance power ratings (in watts)
const float powerRatings[4] = {10.0, 5.0, 80.0, 0.5}; 

// Scheduling variables
unsigned long lastScheduleTime = 0;
int currentAppliance = 0;  
unsigned long scheduleDuration = 0; 

// Energy and time tracking
unsigned long deviceOnTime[4] = {0, 0, 0, 0};  
float energyConsumed[4] = {0.0, 0.0, 0.0, 0.0}; 

// Manual control states
bool manualStates[4] = {false, false, false, false}; 
bool manualOverrides[4] = {false, false, false, false}; 

void setup() {
  Serial.begin(115200);


  pinMode(Bulb1, OUTPUT);
  pinMode(Bulb2, OUTPUT);
  pinMode(Bulb3, OUTPUT);
  pinMode(Bulb4, OUTPUT);
  digitalWrite(Bulb1, LOW);
  digitalWrite(Bulb2, LOW);
  digitalWrite(Bulb3, LOW);
  digitalWrite(Bulb4, LOW);


  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");


  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;


  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase initialized");
}

void loop() {
  int durationInSeconds;

  if (Firebase.getInt(firebaseData, "/ScheduleTime/ScheduleTime")) 
  {

    durationInSeconds = firebaseData.intData();
    if (durationInSeconds > 0) 
    {
      scheduleDuration = (durationInSeconds * 1000) / 4;  
      Serial.printf("Updated schedule duration: %d ms per appliance\n", scheduleDuration);
    } else {
      Serial.println("Invalid schedule duration received. Using default.");
    }
  } else {
    Serial.println("Failed to fetch schedule duration from Firebase");
  }


  checkManualControlFromFirebase();

  if (durationInSeconds == 0) 
  {

    Serial.println("Load Scheduling Off");
    for (int i = 0; i < 4; i++) 
    { 
      if (!manualOverrides[i])
      {digitalWrite(getRelayPin(i), LOW);}
    }
  }
  else 
  {
      handleScheduling();
  }

  updateEnergyStatsOnFirebase();
}

//function to handle scheduling
void handleScheduling() {
  unsigned long currentTime = millis();

  if (currentTime - lastScheduleTime >= scheduleDuration) {
    lastScheduleTime = currentTime;


    for (int i = 0; i < 4; i++) {
      if (!manualOverrides[i]) {
        digitalWrite(getRelayPin(i), LOW); 
      }
    }


    if (!manualOverrides[currentAppliance]) {
      digitalWrite(getRelayPin(currentAppliance), HIGH);  
      deviceOnTime[currentAppliance] += scheduleDuration; 
      energyConsumed[currentAppliance] += 
          (powerRatings[currentAppliance] * scheduleDuration / 3600000.0); 
      Serial.printf("Bulb%d is ON (scheduled)\n", currentAppliance + 1);
    } else {
      Serial.printf("Bulb%d is under manual control, skipping schedule.\n", currentAppliance + 1);
    }


    currentAppliance = (currentAppliance + 1) % 4;
  }
}

//Function to control manually via firebase
void checkManualControlFromFirebase() {
  for (int i = 0; i < 4; i++) {
    String path = "/ES_Project/S" + String(i + 1); 
    if (Firebase.getString(firebaseData, path)) {
      String state = firebaseData.stringData();
      if (state == "1") {
        manualStates[i] = true;
        manualOverrides[i] = true;
        digitalWrite(getRelayPin(i), HIGH);
      } else if (state == "0") {
        manualOverrides[i] = false;
        manualStates[i] = false;
      }
    }
  }
}


int getRelayPin(int index) {
  switch (index) {
    case 0: return Bulb1;
    case 1: return Bulb2;
    case 2: return Bulb3;
    case 3: return Bulb4;
    default: return -1;
  }
}


void updateEnergyStatsOnFirebase() {
  for (int i = 0; i < 4; i++) {
    String timePath = "/ES_Project/S" + String(i + 1) + "Time";
    String energyPath = "/ES_Project/S" + String(i + 1) + "Energy";

    Firebase.setInt(firebaseData, timePath, deviceOnTime[i] / 1000);  
    Firebase.setFloat(firebaseData, energyPath, energyConsumed[i]);  
  }
}
