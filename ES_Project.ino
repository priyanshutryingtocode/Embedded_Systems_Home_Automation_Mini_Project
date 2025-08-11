#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Define the pins for the four relays
#define Bulb1 D1  
#define Bulb2 D3  
#define Bulb3 D4  
#define Bulb4 D5  

// Firebase credentials
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

#define USER_EMAIL "priyanshusrivastava406@gmail.com"
#define USER_PASSWORD "12345678"

// WiFi credentials
#define ssid "1+Hotspot"
#define password "Maul7011ftw"

// Firebase project credentials
#define FIREBASE_HOST "https://home-automationdb-70f57-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "wW8fsnzbeaEetV5rHJoZTtQFlT3dyETUHfLrY9y8"

// Appliance power ratings (in watts)
const float powerRatings[4] = {10.0, 5.0, 80.0, 0.5}; // Example: Bulb1: 60W, Bulb2: 75W, etc.

// Scheduling variables
unsigned long lastScheduleTime = 0;
int currentAppliance = 0;  // Tracks which appliance is currently active
unsigned long scheduleDuration = 0;  // Default duration (in milliseconds) for each appliance

// Energy and time tracking
unsigned long deviceOnTime[4] = {0, 0, 0, 0};  // Total ON time (in milliseconds) for each appliance
float energyConsumed[4] = {0.0, 0.0, 0.0, 0.0}; // Energy consumed by each appliance (in Wh)

// Manual control states
bool manualStates[4] = {false, false, false, false}; // Tracks manual control states for Bulb1 to Bulb4
bool manualOverrides[4] = {false, false, false, false}; // Flags to check if an appliance has manual control

void setup() {
  Serial.begin(115200);

  // Configure relay pins as output and set to HIGH (OFF state)
  pinMode(Bulb1, OUTPUT);
  pinMode(Bulb2, OUTPUT);
  pinMode(Bulb3, OUTPUT);
  pinMode(Bulb4, OUTPUT);
  digitalWrite(Bulb1, LOW);
  digitalWrite(Bulb2, LOW);
  digitalWrite(Bulb3, LOW);
  digitalWrite(Bulb4, LOW);

  // Connect to WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Firebase configuration
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  // Initialize Firebase
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
      scheduleDuration = (durationInSeconds * 1000) / 4;  // Divide total time equally among 4 appliances
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
    // Turn off all devices
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



  // Update energy statistics on Firebase
  updateEnergyStatsOnFirebase();
}

// Function to handle scheduling logic
void handleScheduling() {
  unsigned long currentTime = millis();

  if (currentTime - lastScheduleTime >= scheduleDuration) {
    lastScheduleTime = currentTime;

    // Turn off all appliances unless manually controlled
    for (int i = 0; i < 4; i++) {
      if (!manualOverrides[i]) {
        digitalWrite(getRelayPin(i), LOW);  // Turn OFF the relay
      }
    }

    // Schedule the next appliance if no manual control
    if (!manualOverrides[currentAppliance]) {
      digitalWrite(getRelayPin(currentAppliance), HIGH);  // Turn ON the relay
      deviceOnTime[currentAppliance] += scheduleDuration; // Accumulate ON time
      energyConsumed[currentAppliance] += 
          (powerRatings[currentAppliance] * scheduleDuration / 3600000.0); // Convert ms to hours
      Serial.printf("Bulb%d is ON (scheduled)\n", currentAppliance + 1);
    } else {
      Serial.printf("Bulb%d is under manual control, skipping schedule.\n", currentAppliance + 1);
    }

    // Move to the next appliance in the schedule
    currentAppliance = (currentAppliance + 1) % 4;
  }
}

// Function to check manual control commands in Firebase
void checkManualControlFromFirebase() {
  for (int i = 0; i < 4; i++) {
    String path = "/ES_Project/S" + String(i + 1);  // S1, S2, S3, S4
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

// Helper function to get relay pin based on index
int getRelayPin(int index) {
  switch (index) {
    case 0: return Bulb1;
    case 1: return Bulb2;
    case 2: return Bulb3;
    case 3: return Bulb4;
    default: return -1;
  }
}

// Function to update energy statistics on Firebase
void updateEnergyStatsOnFirebase() {
  for (int i = 0; i < 4; i++) {
    String timePath = "/ES_Project/S" + String(i + 1) + "Time";
    String energyPath = "/ES_Project/S" + String(i + 1) + "Energy";

    Firebase.setInt(firebaseData, timePath, deviceOnTime[i] / 1000);  // OnTime in seconds
    Firebase.setFloat(firebaseData, energyPath, energyConsumed[i]);  // Energy in Wh
  }
}
