#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>  // Include the Wi-Fi library
#include <ESP8266mDNS.h>
#include <Wire.h>

#include "accel_data.h"

const char *ssid = "Wineglass";       // The name of the Wi-Fi network that will be created
const char *password = "nocheating";  // The password required to connect to it, leave blank for an open network

ESP8266WebServer server(80);  // Create an HTTP server on port 80

Adafruit_MPU6050 mpu;
enum Behavior {
    good = 0,
    okey = 1,
    bad = 2,
    horrible = 3
};

// struct DataFrame {
//     double pitch;
//     double roll;
// };
//
std::vector<DataFrame> accel_data;
int tamper_counter = 0;
int tamper_counter_limit = 30;
float tamper_theshold = 0.3f;
float good_threshold = 4.0f;
float okey_threshold = 6.0f;
Behavior behavior;
int GREEN = 14;
int BLUE = 12;
int RED = 16;

void set_behavior(Behavior new_behavior) {
    switch (new_behavior) {
        case Behavior::good:
            analogWrite(GREEN, 0xff);
            digitalWrite(RED, LOW);
            behavior = Behavior::good;
            break;
        case Behavior::okey:
            analogWrite(GREEN, 0x4c);
            digitalWrite(RED, HIGH);
            behavior = Behavior::okey;
            break;
        case Behavior::bad:
            analogWrite(GREEN, 0x00);
            digitalWrite(RED, HIGH);
            behavior = Behavior::bad;
            break;
        case Behavior::horrible:
            analogWrite(GREEN, 0x00);
            digitalWrite(RED, HIGH);
            behavior = Behavior::horrible;
            break;
    }
}

// void fetch_new_dataframe(std::vector<DataFrame> &frames, const sensors_vec_t new_value) {
//     if (frames.size() > 4) {
//         frames.erase(frames.begin());
//     }
//     auto xa = new_value.x;
//     auto ya = new_value.y;
//     auto za = new_value.z;
//     auto roll = abs(atan2(ya, za) * 180.0 / PI);
//     auto pitch = abs(atan2(-xa, sqrt(ya * ya + za * za)) * 180.0 / PI);
//     frames.emplace_back(DataFrame{pitch, roll});
// }

auto get_lowest(const std::vector<DataFrame> &frames) -> DataFrame {
    DataFrame smallest{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    for (auto frame : frames) {
        if (frame.pitch < smallest.pitch) {
            smallest.pitch = frame.pitch;
        }
        if (frame.roll < smallest.roll) {
            smallest.roll = frame.roll;
        }
    }
    Serial.print("pitch: ");
    Serial.print(smallest.pitch);
    Serial.print(" roll: ");
    Serial.println(smallest.roll);
    return smallest;
}

auto get_behavior_from_accel(const std::vector<DataFrame> &accel_data) {
    auto lowest_data = get_lowest(accel_data);
    if (std::max(lowest_data.pitch, lowest_data.roll) > okey_threshold) {
        return Behavior::bad;
    }
    if (std::max(lowest_data.pitch, lowest_data.roll) > good_threshold) {
        return Behavior::okey;
    }
    return Behavior::good;
}
auto detect_tamper(const std::vector<DataFrame> &accel_data) {
    DataFrame avg{0.0f, 0.0f};
    for (auto frame : accel_data) {
        avg.pitch += frame.pitch;
        avg.roll += frame.roll;
    }
    avg.pitch = avg.pitch / accel_data.size();
    avg.roll = avg.roll / accel_data.size();

    for (auto frame : accel_data) {
        if (abs(avg.pitch - frame.pitch) > tamper_theshold) {
            return false;
        }
        if (abs(avg.roll - frame.roll) > tamper_theshold) {
            return false;
        }
    }
    return true;
}

void handleRoot() {
    set_behavior(Behavior::good);
    server.send(200, "text/plain", "Wineglass has been reset!");
}

void setup(void) {
    Serial.begin(9600);

    accel_data.reserve(11);
    for (int i = 0; i < 5; i++) {
        accel_data.emplace_back(DataFrame{0.0f, 0.0f});
    }

    delay(10);
    Serial.println('\n');

    WiFi.softAP(ssid, password);  // Start the access point
    Serial.print("Access Point \"");
    Serial.print(ssid);
    Serial.println("\" started");

    Serial.print("IP address:\t");
    Serial.println(WiFi.softAPIP());

    if (MDNS.begin("wineglass")) {  // Start the mDNS responder for esp8266.local
        Serial.println("mDNS responder started");
    } else {
        Serial.println("Error setting up MDNS responder!");
    }

    server.on("/", handleRoot);
    server.begin();
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
    pinMode(RED, OUTPUT);
    set_behavior(Behavior::bad);
    while (!Serial) {
        delay(10);  // will pause Zero, Leonardo, etc until serial console opens
    }

    // Try to initialize!
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(10);
        }
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    // Serial.println("");
    delay(100);
}

void loop() {
    server.handleClient();
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    fetch_new_dataframe(accel_data, a.acceleration);
    if (detect_tamper(accel_data)) {
        tamper_counter++;
        if (tamper_counter >= tamper_counter_limit) {
            set_behavior(Behavior::bad);
        }
    } else {
        tamper_counter = 0;
    }
    auto new_behavior = get_behavior_from_accel(accel_data);
    if (new_behavior > behavior) {
        set_behavior(new_behavior);
    }

    delay(50);
}

// Serial.println(g.gyro.z);