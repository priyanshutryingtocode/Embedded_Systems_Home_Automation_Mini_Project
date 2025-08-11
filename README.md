# Embedded_Systems_Home_Automation_Mini_Project

-----


A simple yet effective home automation project using an ESP8266 to control four appliances via Google's Firebase Realtime Database. This system features both manual remote control and an intelligent load scheduling mechanism to manage power consumption by running only one appliance at a time in a round-robin fashion. It also tracks the total ON-time and energy usage for each appliance.

## ⭐ Features

  * **Remote Control:** Toggle up to 4 appliances ON/OFF from anywhere using Firebase.
  * **Automated Load Scheduling:** Set a total time duration in Firebase, and the system will intelligently schedule each appliance to run for an equal fraction of that time, one after another. This is perfect for managing loads on a limited power supply like an inverter.
  * **Manual Override:** Seamlessly switch between scheduled operation and manual control. Manually turning an appliance ON will exclude it from the scheduling cycle until it's turned OFF.
  * **Energy Monitoring:** Keep track of how long each appliance has been running (in seconds) and its total energy consumption (in Watt-hours).
  * **Dynamic Configuration:** Change the scheduling duration on the fly directly from the Firebase database without reprogramming the ESP8266.

-----

## 🛠️ Hardware & Software Requirements

### Hardware

  * ESP8266 Development Board (e.g., NodeMCU, Wemos D1 Mini)
  * 4-Channel 5V Relay Module
  * Appliances to control (e.g., Bulbs, Fans)
  * Jumper Wires
  * 5V Power Supply

### Software & Libraries

  * [Arduino IDE](https://www.arduino.cc/en/software)
  * ESP8266 Board Manager for Arduino IDE
  * **Libraries:**
      * `ESP8266WiFi.h` (included with ESP8266 core)
      * `FirebaseESP8266.h` (Install via Arduino Library Manager)

-----

## ⚙️ Setup and Installation

### 1\. Hardware Wiring

🚨 **Safety First\!** Working with mains voltage is dangerous. Ensure the main power is disconnected before wiring the appliances to the relay.

  * Connect the ESP8266 to the 4-Channel Relay Module as follows:
      * **D1** -\> **IN1**
      * **D3** -\> **IN2**
      * **D4** -\> **IN3**
      * **D5** -\> **IN4**
      * **VIN** -\> **VCC** on the relay module
      * **GND** -\> **GND** on the relay module
  * Wire your appliances through the **COM** (Common) and **NO** (Normally Open) terminals of each relay channel.

### 2\. Firebase Setup

1.  Go to the [Firebase Console](https://console.firebase.google.com/) and create a new project.

2.  In the project dashboard, go to **Build -\> Realtime Database**.

3.  Click **Create Database** and start in **test mode** (or set up rules for read/write access).

4.  Once created, you will see your **Database URL** (e.g., `https://your-project-id-default-rtdb.firebaseio.com/`). This is your `FIREBASE_HOST`.

5.  Go to **Project Settings -\> Service accounts -\> Database secrets** to find your legacy **Database Secret**. This is your `FIREBASE_AUTH`.

6.  Import the following JSON structure into your Realtime Database by clicking the three dots and selecting "Import JSON".

    ```json
    {
      "ES_Project": {
        "S1": "0",
        "S1Energy": 0,
        "S1Time": 0,
        "S2": "0",
        "S2Energy": 0,
        "S2Time": 0,
        "S3": "0",
        "S3Energy": 0,
        "S3Time": 0,
        "S4": "0",
        "S4Energy": 0,
        "S4Time": 0
      },
      "ScheduleTime": {
        "ScheduleTime": 60
      }
    }
    ```

### 3\. Arduino Code Configuration

1.  Clone this repository or download the `.ino` file.

2.  Open the sketch in the Arduino IDE.

3.  Fill in your credentials in the following lines:

    ```cpp
    // WiFi credentials
    #define ssid "yourssid"
    #define password "yourpass"

    // Firebase project credentials
    #define FIREBASE_HOST "your-firebase-database-url" // Without https://
    #define FIREBASE_AUTH "your-firebase-database-secret"
    ```

4.  (Optional) Adjust the power rating of your appliances (in Watts) for accurate energy calculation:

    ```cpp
    // Appliance power ratings (in watts)
    const float powerRatings[4] = {10.0, 5.0, 80.0, 0.5};
    ```

5.  Select your ESP8266 board from **Tools \> Board** (e.g., "NodeMCU 1.0 (ESP-12E Module)") and the correct COM port.

6.  Upload the code to your ESP8266.

-----

## 🕹️ How It Works

### Database Structure

  * **/ScheduleTime/ScheduleTime**: The **total duration in seconds** for one complete scheduling cycle. The code divides this value by 4 to get the run time for each appliance. Setting this to `0` disables the scheduling feature.
  * **/ES\_Project/S1...S4**: Manual control switches. Set to `"1"` to turn the corresponding appliance ON and `"0"` to turn it OFF.
  * **/ES\_Project/S1Time...S4Time**: (Read-only) Reports the total time the appliance has been ON, in seconds.
  * **/ES\_Project/S1Energy...S4Energy**: (Read-only) Reports the cumulative energy consumed by the appliance in **Watt-hours**.

### Operational Logic

1.  **Initialization**: On boot, the ESP8266 connects to your WiFi network and initializes the connection to Firebase.
2.  **Main Loop**:
      * It first fetches the `ScheduleTime` from Firebase.
      * Then, it checks the status of the manual control switches (`S1` to `S4`). If a switch is set to `"1"`, it activates the corresponding relay and flags it as a "manual override".
      * **If `ScheduleTime` is greater than 0**: The `handleScheduling()` function is called. This function cycles through the appliances, turning one ON at a time for the calculated duration (`scheduleDuration = ScheduleTime * 1000 / 4`). It skips any appliance that is currently under manual override.
      * **If `ScheduleTime` is 0**: Scheduling is turned OFF. All appliances are turned off unless they are manually overridden.
      * Finally, it pushes the updated ON-time and calculated energy consumption statistics back to Firebase for monitoring.

-----

## 📜 License

This project is licensed under the MIT License. See the [LICENSE](https://www.google.com/search?q=LICENSE) file for details.
