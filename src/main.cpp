/*
 * @author: Idlan Zafran Mohd Zaidie
 * Date: 2026
 * @license MIT
 */

#include <Arduino.h>
#include <Ticker.h>
#include <RapidBootWiFi.h>
#include <ThingsSentral.h>

// ==========================================
// 1. USER CONFIGURATION (Change these)
// ==========================================
namespace Config {
    // --- Server & Identity ---
    // [!] SECURITY NOTE: Replace these placeholders with your actual IDs.
    // Never commit your real User ID or Device IDs to a public Git repository.
    char userID[16]      = "YOUR_USER_ID";
    char sensorID[32]    = "YOUR_SENSOR_ID_HERE";   
    char buzzerID[32]    = "YOUR_BUZZER_ID_HERE";
    char conditionID[32] = "YOUR_CONDITION_ID_HERE";

    // --- Hardware Pins ---
    constexpr int LED_1      = 14; 
    constexpr int LED_2      = 12; 
    constexpr int LED_3      = 13; 
    constexpr int BUZZER_PIN = 5;
    constexpr int BATT_PIN   = A0;

    // --- Battery Calibration ---
    constexpr float R1          = 10000.0;
    constexpr float R2          = 4700.0;
    constexpr float CALIBRATION = 0.978;

    // --- Voltage Thresholds ---
    constexpr float VOLT_CRITICAL = 9.1;
    constexpr float VOLT_MID      = 9.3;
    constexpr float VOLT_FULL     = 9.6;
}

// ==========================================
// 2. SYSTEM CLASSES & STRUCTURES
// ==========================================
struct BatterySystem {
    float voltage = 0.0;

    bool isCritical() { return voltage < Config::VOLT_CRITICAL; }
    bool isMid()      { return voltage >= Config::VOLT_MID; }
    bool isFull()     { return voltage >= Config::VOLT_FULL; }

    void update() {
    long sum = 0;
    for (int i = 0; i < 20; i++) { 
        sum += analogRead(Config::BATT_PIN); 
        delay(2); 
    }
    float avg = (float)sum / 20.0;
    
    // The 3.3 in this formula is critical because NodeMCU scales A0 to 3.3V
    // Your external divider (10k + 4.7k) / 4.7k = ~3.12
    voltage = (avg / 1024.0) * 3.3 * ((Config::R1 + Config::R2) / Config::R2) * Config::CALIBRATION;
}
};

BatterySystem pwr;

enum class SystemMode { STANDBY, ALARMING };
SystemMode currentMode = SystemMode::STANDBY;

// Global State
bool buzzerToggle = false;
bool heartbeatToggle = false;
unsigned long lastHeartbeatTime = 0;
unsigned long lastVoltTime = 0;
unsigned long lastServerTime = 0;

Ticker alarmTicker;
Ticker setupTicker; 

// ==========================================
// 3. FUNCTIONS
// ==========================================

void setupHardware() {
    pinMode(Config::LED_1, OUTPUT); 
    pinMode(Config::LED_2, OUTPUT); 
    pinMode(Config::LED_3, OUTPUT);
    pinMode(Config::BUZZER_PIN, OUTPUT);
    digitalWrite(Config::BUZZER_PIN, HIGH); // Off (Active Low)
}

void resetIndicators(){
    digitalWrite(Config::LED_1, LOW); 
    digitalWrite(Config::LED_2, LOW); 
    digitalWrite(Config::LED_3, LOW);
}

void bootCountCheck(){
  myWiFi.setAPName("Remote_Buzzer_Alarm");
  myWiFi.begin();
  int currentCount = myWiFi.getCurrentBootCount();
  Serial.printf("Current Session Boot Count: %d\n", currentCount);
  digitalWrite(Config::LED_1, (currentCount >= 1) ? HIGH : LOW);
  digitalWrite(Config::LED_2, (currentCount>= 2) ? HIGH : LOW);
  digitalWrite(Config::LED_3, (currentCount>= 3) ? HIGH : LOW);
  delay(500);

  resetIndicators();

  if (!myWiFi.wasWiFiReset()) return;
    Serial.println("Action: WiFi reset detected. Initializing setup mode...");

    for(int i = 0; i <= 3; i++) {
      digitalWrite(Config::LED_1, HIGH);
      digitalWrite(Config::LED_2, HIGH);
      digitalWrite(Config::LED_3, HIGH);
      delay(500);

      digitalWrite(Config::LED_1, LOW);
      digitalWrite(Config::LED_2, LOW);
      digitalWrite(Config::LED_3, LOW);
      delay(500);
    }
}

void setupAnimationTask() {
    static int step = 0;

    digitalWrite(Config::LED_1, LOW); 
    digitalWrite(Config::LED_2, LOW); 
    digitalWrite(Config::LED_3, LOW);

    if (step == 0) digitalWrite(Config::LED_3, HIGH);
    else if (step == 1) digitalWrite(Config::LED_2, HIGH);
    else if (step == 2) digitalWrite(Config::LED_1, HIGH);

    step++;
    if (step > 2) step = 0;
}

void setupWiFiConfig() {   
    setupTicker.attach_ms(500, setupAnimationTask);

    myWiFi.addParameter("uID", "User ID", Config::userID, 15);
    myWiFi.addParameter("sID", "Sensor ID", Config::sensorID, 31); 
    myWiFi.addParameter("bID", "Buzzer ID", Config::buzzerID, 31);
    myWiFi.addParameter("cID", "Condition ID", Config::conditionID, 31);
    
    // --- THE MAGIC BUTTON CHECK ---
    pinMode(0, INPUT_PULLUP); // Pin 0 is the BOOT/FLASH button on the ESP
    
    // If the button is held down (LOW) during boot, open the portal
    if (digitalRead(0) == LOW) {
        Serial.println("Setup button held! Opening Configuration Portal...");
        myWiFi.openPortal(); // This stays open until you hit save on the web
    } else {
        myWiFi.connect(); // Normal boot up / Rapid Boot logic
    }
    // ------------------------------
    
    setupTicker.detach();
}

void updateConfig() {
    // Clear everything
    memset(Config::userID, 0, sizeof(Config::userID));
    memset(Config::sensorID, 0, sizeof(Config::sensorID));
    memset(Config::buzzerID, 0, sizeof(Config::buzzerID));
    memset(Config::conditionID, 0, sizeof(Config::conditionID));

    // Copy values
    const char* u = myWiFi.getParameterValue("uID");
    const char* s = myWiFi.getParameterValue("sID");
    const char* b = myWiFi.getParameterValue("bID");
    const char* c = myWiFi.getParameterValue("cID");

    if (u) strncpy(Config::userID, u, 15);
    if (s) strncpy(Config::sensorID, s, 31);
    if (b) strncpy(Config::buzzerID, b, 31);
    if (c) strncpy(Config::conditionID, c, 31);
}

void backgroundBeepTask() {
    if (currentMode == SystemMode::ALARMING) {
        buzzerToggle = !buzzerToggle;
        int state = buzzerToggle ? HIGH : LOW;
        
        digitalWrite(Config::LED_1, state); 
        digitalWrite(Config::LED_2, state); 
        digitalWrite(Config::LED_3, state);

        if (!pwr.isCritical()) {
            digitalWrite(Config::BUZZER_PIN, buzzerToggle ? LOW : HIGH);
        } else {
            digitalWrite(Config::BUZZER_PIN, HIGH); // Mute if battery low
        }
    }
}

void showBatteryLevel() {
    unsigned long now = millis();

    if (pwr.voltage >= Config::VOLT_FULL) {
        // 3 LEDs: 9.6V+
        digitalWrite(Config::LED_1, HIGH); 
        digitalWrite(Config::LED_2, HIGH); 
        digitalWrite(Config::LED_3, HIGH);         
    } 
    else if (pwr.voltage >= Config::VOLT_MID) {
        // 2 LEDs: 9.3V - 9.59V
        digitalWrite(Config::LED_1, HIGH); 
        digitalWrite(Config::LED_2, HIGH); 
        digitalWrite(Config::LED_3, LOW);         
    } 
    else if (pwr.voltage >= Config::VOLT_CRITICAL) {
        // 1 LED: 9.1V - 9.29V
        digitalWrite(Config::LED_1, HIGH); 
        digitalWrite(Config::LED_2, LOW); 
        digitalWrite(Config::LED_3, LOW);   
    }
    else {
        // FAST BLINK: Below 9.1V
        digitalWrite(Config::LED_2, LOW);
        digitalWrite(Config::LED_3, LOW);
        
        // Blink LED_1 every 100ms
        if ((now / 100) % 2 == 0) {
            digitalWrite(Config::LED_1, HIGH);
        } else {
            digitalWrite(Config::LED_1, LOW);
        }
    }
}

void handlePowerMonitoring(unsigned long now) {
    if (now - lastVoltTime >= 2000) { // Increased to 2s to make Serial readable
        lastVoltTime = now;
        pwr.update();
        
        Serial.print("--- Power Status ---\n");
        Serial.print("Voltage: "); Serial.print(pwr.voltage); Serial.println("V");
        Serial.print("Status: ");
        if (pwr.isFull()) Serial.println("FULL");
        else if (pwr.isMid()) Serial.println("MEDIUM");
        else if (pwr.isCritical()) Serial.println("CRITICAL (Muted)");
        else Serial.println("LOW");
        Serial.println("--------------------");
    }
}

void updateSystemMode() {
    if (!TS.isOnline()) return;

    // 1. Fetch values
    ReadResult sensorRes = TS.Command.read(Config::sensorID);
    ReadResult condRes   = TS.Command.read(Config::conditionID);
    ReadResult buzzRes   = TS.Command.read(Config::buzzerID);

    float sensorVal    = sensorRes.value.toFloat();
    float conditionVal = condRes.value.toFloat();
    bool manualTrigger = (buzzRes.value == "1" || buzzRes.value == "true");

    // 2. Logic for Trigger
    bool autoTrigger = (conditionVal > 0 && sensorVal >= conditionVal);
    bool shouldAlarm = manualTrigger || autoTrigger;

    // 3. Sync Logic
    if (shouldAlarm) {
        if (currentMode == SystemMode::STANDBY) {
            currentMode = SystemMode::ALARMING;
            Serial.println(">>> ALARM TRIGGERED <<<");
        }
        // Force the dashboard to 'true' if it was an auto-trigger
        if (!manualTrigger) {
            TS.Command.send(Config::buzzerID, "true");
            manualTrigger = true; // Update local for the print
        }
    } else {
        if (currentMode == SystemMode::ALARMING) {
            currentMode = SystemMode::STANDBY;
            TS.Command.send(Config::buzzerID, "false");
            manualTrigger = false; // Update local for the print
            resetIndicators();
            digitalWrite(Config::BUZZER_PIN, HIGH); 
            Serial.println(">>> ALARM STOPPED <<<");
        }
    }

    // 4. PRINT (Now using local bool to ensure it shows '1' if active)
    Serial.println("--- Sync Status ---");
    Serial.printf("Sensor: %.2f | Thresh: %.2f\n", sensorVal, conditionVal);
    Serial.printf("Buzzer State: %d\n", shouldAlarm ? 1 : 0);
    Serial.println("-------------------");
}

void handleUI(unsigned long now) {
    if (currentMode != SystemMode::STANDBY) return;

    // If online, just show the level (including fast blink if low)
    if (TS.isOnline()) {
        showBatteryLevel();
    } 
    else {
        // If offline AND battery is healthy, do the slow heartbeat
        if (!pwr.isCritical() && pwr.voltage >= Config::VOLT_CRITICAL) {
            if ((now / 500) % 2 == 0) {
                showBatteryLevel();
            } else {
                resetIndicators();
            }
        } 
        else {
            // If offline BUT battery is CRITICAL, skip heartbeat and just do FAST BLINK
            showBatteryLevel(); 
        }
    }
}

// ==========================================
// 4. MAIN EXECUTION
// ==========================================

void setup() {
    Serial.begin(115200);
    setupHardware();

    bootCountCheck();
    setupWiFiConfig(); 
    resetIndicators();
    updateConfig();
    Serial.print("SensorID in memory: "); Serial.println(Config::sensorID);
    Serial.print("CondID in memory: "); Serial.println(Config::conditionID);
    
    TS.begin(Config::userID);

    alarmTicker.attach_ms(150, backgroundBeepTask);

    Serial.println("System Initialized.");
}

void loop() {
    unsigned long now = millis();
    myWiFi.loop();

    handlePowerMonitoring(now);

    if (now - lastServerTime >= 1000) {
        lastServerTime = now;
        updateSystemMode();
    }

    handleUI(now);
}