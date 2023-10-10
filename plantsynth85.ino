/*
   Plant Paper Synth
   by Jordan Hodges, 2023

   Learn more at http://studioakustiks.com
*/

#include <avr/pgmspace.h>
// Constants
const byte SAMPLE_SIZE = 30; // Number of samples we collect before analyzing
const byte ANALYSIS_SIZE = SAMPLE_SIZE - 1; // Used for analyzing
const byte SENSOR_PIN = 2;  // Sensor connection
const byte LED_PIN = 3;     // LED connection
const float THRESHOLD = 1; // Threshold for determining significant change
#define SPEAKER_PIN PB0

// Variables for storing sensor data
volatile byte sensorState = LOW;
volatile unsigned long previousTime = 0;
volatile byte currentSampleIndex = 0;
volatile unsigned long sensorReadings[SAMPLE_SIZE];
byte significantChangeCount = 1; // Counts how many times readings significantly change

// Prepare the speaker to play audio
void setupPWM() {
    pinMode(SPEAKER_PIN, OUTPUT);
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    TCCR0B = (1 << CS00);
}

// Generate a square wave based on pitch
void generateSquareWave(int pitch) {
    int halfPeriod = 5000000 / pitch; // Half of the period in microseconds

    for (int i = 0; i < halfPeriod; i++) {
        digitalWrite(SPEAKER_PIN, HIGH);
        delayMicroseconds(halfPeriod);
        digitalWrite(SPEAKER_PIN, LOW);
        delayMicroseconds(halfPeriod);
    }
}

// Analyze the collected sensor data
void analyzeSamples() {
    unsigned long total = 0;
    unsigned long maximumValue = 0;
    unsigned long minimumValue = 100;
    byte hasChanged = 0;

    // If we've collected enough samples, analyze them
    if (currentSampleIndex == SAMPLE_SIZE) {
        PORTB &= ~(1 << LED_PIN); // Turn off the LED
        unsigned long analysisData[ANALYSIS_SIZE];
        
        // Extract readings and find maximum, minimum and total
        for (byte i = 0; i < ANALYSIS_SIZE; i++) {
            analysisData[i] = sensorReadings[i+1];
            if (analysisData[i] > maximumValue) { maximumValue = analysisData[i]; }
            if (analysisData[i] < minimumValue) { minimumValue = analysisData[i]; }
            total += analysisData[i];
        }

        unsigned long average = total / ANALYSIS_SIZE;
        unsigned long range = maximumValue - minimumValue; 

        // Check if readings significantly change
        if (range > (average * THRESHOLD)) {
            significantChangeCount++;
            if (significantChangeCount >= 2) {
                hasChanged = 1;
            }
        } else {
            significantChangeCount = 0;
        }

        // If there's significant change, generate a square wave
        if (hasChanged) {
            int pitch = map(average, 0, 1023, 1000, 1200); 
            PORTB ^= (1 << LED_PIN); // Toggle the LED
            generateSquareWave(pitch); 
            significantChangeCount = 0;
        }

        currentSampleIndex = 0;
    }
}

// Capture and store sensor readings
void captureSensorReading() {
    sensorState = !sensorState; 
    unsigned long currentTime = micros(); 
    if (currentSampleIndex < SAMPLE_SIZE) {
        sensorReadings[currentSampleIndex] = currentTime - previousTime;
        previousTime = currentTime; 
        currentSampleIndex++; 
    }
}

// Initialize the system
void setup() {
    DDRB &= ~(1 << SENSOR_PIN);
    PORTB |= (1 << SENSOR_PIN);
    PCMSK |= (1 << PCINT2);
    GIMSK |= (1 << PCIE);
    pinMode(LED_PIN, OUTPUT);
    setupPWM();
    //delay(500); 
}

// Interrupt to detect sensor change
ISR(PCINT0_vect) {
  if (PINB & (1 << SENSOR_PIN)) {
    captureSensorReading();
  }
}

// Main loop: Analyze samples
void loop() {
    analyzeSamples();
}
