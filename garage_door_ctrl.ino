#include "parking_meter.h"

/*
 * Neo Pixel Stuff
 */
// Include the NeoPixel Library
#include "neopixel/neopixel.h"
#include "neopixelcolors.h"

// Define the NeoPixel attributes
#define NEOPIXEL_COUNT 1
#define NEOPIXEL_TYPE WS2811

#define OPEN    0
#define CLOSED  1
#define SLOW_DIST   750 // Distance to start slowing down
#define STOP_DIST   500 // Distance you should STOP!
#define FLSH_DIST   250 // Distance to start flashing RED to STOP!

// Define the pins
#define NEOPIXEL_PIN D4
#define MFET_PIN    D6
#define SWITCH_PIN  D3
#define RNG1_PIN    A6
#define RNG2_PIN    A7
#define MFET_DBG_PIN D7

int garage_status = 0;
int last_garage_status = 0;
long last_pub = 0;

/*
 * Spark Variables, Functions, and Events:
 *
 * Variables:
 *  state:  Read the state of the Reed Switch on the garage door (1 = closed, 0 = open)
 *          Reflects 'garage_status'
 *
 * Functions:
 *  button: Press the garage door button.  Calls press_garage_button();
 *
 * Events:
 *  garage_door:    Private event.  Publish the status of the Reed Switch (1 = closed, 0 = open)
 */
 

// Include the NeoPixel
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEOPIXEL_TYPE);

void setup() {
    // Add Spark functions and variables
    Spark.variable("state", &garage_status, INT);
    Spark.function("button", press_garage_button);

    // Init the Serial port
    Serial.begin(115200);

    // Init the NeoPixel
    strip.begin();
    strip.show();   // Init NeoPixel to off

    // Init the pins, 1 for the MosFET, one for the hall effect switch
    pinMode(MFET_DBG_PIN, OUTPUT);
    pinMode(MFET_PIN, OUTPUT);
    pinMode(SWITCH_PIN, INPUT_PULLDOWN);

    // Set the MosFET to off
    digitalWrite(MFET_PIN, LOW);
}

void loop() {
    // Get the status of the Garage Door Reed Switch
    garage_status = get_door_state();
    unsigned int reading;
    
    // Check if the status changed and publish an event appropriately
    if (last_garage_status != garage_status && (millis() - last_pub) >= 1000) {
        if (garage_status == CLOSED) {
            Serial.println("Garage Door Closed");
            Spark.publish("garage_door", "Garage Door Closed", 60, PRIVATE);
        } else {
            Serial.println("Garage Door Opened");
            Spark.publish("garage_door", "Garage Door Opened", 60, PRIVATE);
        }

        // Update the last status
        last_garage_status = garage_status;
        last_pub = millis();
    }

    // Get the distance from the range finder only when the door is open
    if (garage_status == OPEN) {
        strip.setBrightness(0xFF);  // Set to full brightness
        reading = analogRead(RNG1_PIN);

        // Light the LED
        if (reading > SLOW_DIST) {
            Serial.print(reading);
            Serial.println(" : Pull Forward / GREEN");
            strip.setPixelColor(0, RED);
            strip.show();
        } else if (reading > STOP_DIST && reading <= SLOW_DIST) {
            Serial.print(reading);
            Serial.println(" : Slow Down / YELLOW");
            strip.setPixelColor(0, YELLOW);
            strip.show();
        } else if (reading > FLSH_DIST && reading <= STOP_DIST) {
            Serial.print(reading);
            Serial.println(" : STOP! / RED");
            strip.setPixelColor(0, GREEN);
            strip.show();
        } else if (reading <= FLSH_DIST) {
            Serial.print(reading);
            Serial.println(" : REALLY STOP! / BLUE");
            strip.setPixelColor(0, BLUE);
            strip.show();
        }
    } else {
        strip.setBrightness(0); // Turn the LED off if garage door closed
        strip.show();
    }

    delay(100);
}

// Read the status of the garage door
// 1 = open
// 0 = closed
int get_door_state () {
    if (digitalRead(SWITCH_PIN))
        return CLOSED;
    else
        return OPEN;
}

// Press the garage door button by:
// Write 1 to the MosFET
// Wait 100ms
// Write 0 to the MosFET
int press_garage_button(String command) {
    digitalWrite(MFET_PIN, 1);  // Press the garage door button
    digitalWrite(MFET_DBG_PIN, 1);
    delay(100); // Wait a bit to release the button
    digitalWrite(MFET_PIN, 0);  // Release the garge door button
    digitalWrite(MFET_DBG_PIN, 0);
    
    return 0;   // Spark functions must return something
}

// Open the garage door
// Make sure it is not already open
int open_garage_door() {
    int garage_count;

    if (get_door_state() == OPEN) {  // If the door is already open, do nothing
        return 0;
    } else {    // Otherwise, open the door and make sure it stays open
//        press_garage_button();
        for (garage_count = 120; garage_count != 0; garage_count--) {
            delay(1000);    // Wait 1 second before checking if door is still open
        }
        if(get_door_state() == OPEN) {
            return 0;   // Success
        } else {
            return 1;   // Failed to open the door
        }
    }
}

// Close the garage door
// Make sure it is not already closed
int close_garage_door() {
    int garage_count;

    if (get_door_state() == CLOSED) {
        return 0;   // Already closed, do nothing
    } else {
//        press_garage_button();
        for (garage_count = 120; garage_count != 0; garage_count--) {
            if (get_door_state() == CLOSED) {
                return 0;   // Success
            } else {
                delay(1000);    // Wait a bit to poll again
            }
        }
        return 1;   // Failed to close the door
        
    }
}

// Toggle the garage door open or closed
// Gather current state and make sure it changed
int toggle_garage_door() {
    int current_state;
    int garage_count;
    
    current_state = get_door_state();
//    press_garage_button();
    for (garage_count = 120; garage_count != 0; garage_count) {
        delay(1000);
    }
    
    if (current_state != get_door_state()) {
        return 0;   // Success, garage door is in opposite state
    } else {
        return 1;   // Failed, garage door is in same state as before
    }
}

