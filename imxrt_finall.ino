#define SENSOR_PIN  2// Digital pin for reed switch
#define BUZZER_PIN  8 // Digital pin for buzzer
#define BUTTON_PIN  5 // Digital pin for trip meter reset button
#define DEBOUNCE_TIME 150  // Debounce time in milliseconds

#include <EEPROM.h>
#define EEPROM_SIZE 8
const int adcPin = A1;  // ADC input pin on Arduino Uno
const float refVoltage = 
3.3;
const int adcMax =1023;

const float R1 = 390000.0;
const float R2 = 33000.0;


const float maxVoltage = 42.0;
const float minVoltage = 36.0;
#define CORRECTION_FACTOR 1.713 

volatile int pulseCount = 0;
volatile unsigned long lastPulseTime = 0;
volatile unsigned long lastTimeBetweenPulses = 0;  // Prevent division by zero

float wheelCircumference = 2.29;  
float speed = 0;
float tripDistance = 0.0;
float odometer = 0.0;
unsigned long lastTime = 0;

unsigned long idleTimeout = 2000;
unsigned long lastMovementTime = 0;


bool buttonState = HIGH;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void countPulse() {
    unsigned long currentTime = millis();
    if (currentTime - lastPulseTime > DEBOUNCE_TIME) {
        lastTimeBetweenPulses = currentTime - lastPulseTime;
        lastPulseTime = currentTime;
        pulseCount++;
        lastMovementTime = currentTime;
    }
}

void setup() {
    Serial1.begin(115200);
    pinMode(SENSOR_PIN, INPUT_PULLUP);  // Use internal pull-up
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Trip meter reset button
    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), countPulse, CHANGE);
   // EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, odometer);
    lastTime = millis();
}

void formatAndPrintData(float speed, float batteryPercentage, float odometer, float tripDistance) {
    int odo = static_cast<int>(odometer);
    int odoLow = odo & 0xFF;
    int odoHigh = (odo >> 8) & 0xFF;
   //  Serial.println( batteryPercentage);
    // Serial1.println( speed);
    uint8_t dataArray[7] = {0xF6,static_cast<int>(speed), static_cast<int>(batteryPercentage), odoHigh, odoLow, static_cast<int>(tripDistance),0xFB};
    
    // for (int i = 0; i < sizeof(dataArray); i++) {
    //     Serial.print((dataArray[i]));
    //     Serial.print(" ");
    // }
    //Serial.println();
   for (int i = 0; i < sizeof(dataArray); i++) {
        Serial1.write((dataArray[i]));
    }
   //delay(50);
    

}
float getBatteryPercentage() {
       long sum = 0;
     const int samples =50,samples1=25;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(adcPin);
     //   delay(5);
    }
    //   for (int i = 0; i < samples1; i++) {
    //     sum1 += analogRead(adcPin);
    //     delay(5);
    // }



  int adcValue = sum / samples;
   //Serial1.write( adcValue);
 // int adcValue =analogRead(adcPin); ;
   int s;
   if(adcValue>511 && adcValue<730)
     s=map(adcValue,511,730,0,100);
   //s=(((s+2)/5)*5);
   return s;
    //  Serial.println(adcValue);
    // float dividerVoltage = ((adcValue) / float(adcMax)) * refVoltage;
    // float inputVoltage = (dividerVoltage * ((R1 + R2) / R2));
    // return constrain(((inputVoltage - minVoltage) / (maxVoltage - minVoltage)) * 100, 0, 100);
   
    //    Serial.println(dividerVoltage);
    //     Serial.println(inputVoltage);

}

void checkButtonPress() {
    int reading = digitalRead(BUTTON_PIN);
    
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading == LOW && buttonState == HIGH) {
            tripDistance = 0.0;
        }
        buttonState = reading;
    }

    lastButtonState = reading;
}

void loop() {
    unsigned long currentTime = millis();
    if (currentTime - lastMovementTime > idleTimeout)
   {
        speed = 0;
        lastTimeBetweenPulses = 0;
    }

    if (lastTimeBetweenPulses > 0) {
        speed = (wheelCircumference / (lastTimeBetweenPulses / 1000.0)) * 3.6;
    }

    if (pulseCount > 0) {
        float distanceCovered = pulseCount * (wheelCircumference / 1000.0);
        odometer += distanceCovered;
        tripDistance += distanceCovered;
        pulseCount = 0;
        lastTime = currentTime;
    }

    static float lastSaveOdometer = 0.0;
    if (odometer - lastSaveOdometer >= 1.0) {
        lastSaveOdometer = odometer;
        EEPROM.put(0, odometer);
     //   EEPROM.commit();
    }
    if (speed > 35) {
        digitalWrite(BUZZER_PIN, HIGH);
        // delay(500);
        // digitalWrite(BUZZER_PIN, LOW);
        // delay(500);
    } else {
        digitalWrite(BUZZER_PIN, LOW);
    }

    
    checkButtonPress();
    formatAndPrintData(speed, getBatteryPercentage(), odometer, tripDistance);
}

void resetTripMeter() {
    tripDistance = 0.0;
}
