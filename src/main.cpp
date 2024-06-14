#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SS_PIN 53
#define RST_PIN 5
#define RELAY_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void dump_byte_array(byte *buffer, byte bufferSize) {
    String uidString;
    for (byte i = 0; i < bufferSize; i++) {
        uidString += String(buffer[i] < 0x10 ? "0" : "");
        uidString += String(buffer[i], HEX);
    }
    Serial.print("UID:");
    Serial.println(uidString);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("UID:");
    display.println(uidString);
    display.display();

    delay(5000);
    display.clearDisplay();
}

void setup() {
    Serial.begin(9600);
    SPI.begin();
    rfid.PCD_Init();

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 initialization failed"));
        for (;;);
    }
    delay(2000);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();
    delay(1000);
    display.clearDisplay();
}

void loop() {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        dump_byte_array(rfid.uid.uidByte, rfid.uid.size);
        // Ativa o relé
        digitalWrite(RELAY_PIN, HIGH);
        delay(5000); // Mantém o relé ativado por 5 segundos
        digitalWrite(RELAY_PIN, LOW);
    }
    delay(1000);
}
