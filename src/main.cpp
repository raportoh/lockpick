#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SS_PIN 53
#define RST_PIN 5
#define RELAY_PIN 9
#define BUZZER_PIN 8

MFRC522 rfid(SS_PIN, RST_PIN);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Defina a UID do cartão master (0389af0d)
byte masterUID[] = {0x03, 0x89, 0xaf, 0x0d};

// Frequências das notas (em Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_G3  196
#define NOTE_A3  220
#define NOTE_B3  247
#define NOTE_E5  659
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_G4  392
#define NOTE_F5  698
#define NOTE_DS4 311
#define NOTE_B4  494

// Melodia de Sucesso
int successMelody[] = {
        NOTE_E4, NOTE_G4, NOTE_C5, NOTE_D5, NOTE_E5
};

// Durações das notas: 4 = semínima, 8 = colcheia, etc.
int successDurations[] = {
        8, 8, 8, 8, 8
};

// Melodia de Falha (Game Over)
int failMelody[] = {
        NOTE_E5, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_DS4, NOTE_C4, NOTE_D4, NOTE_B4, NOTE_C5
};

// Durações das notas: 4 = semínima, 8 = colcheia, etc.
int failDurations[] = {
        8, 8, 8, 8, 8, 8, 8, 8, 8
};

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

    delay(2000);
    display.clearDisplay();
}

bool isMasterCard(byte *buffer, byte bufferSize) {
    if (bufferSize != sizeof(masterUID)) return false;
    for (byte i = 0; i < bufferSize; i++) {
        if (buffer[i] != masterUID[i]) return false;
    }
    return true;
}

void playMelody(int melody[], int durations[], int size) {
    for (int thisNote = 0; thisNote < size; thisNote++) {
        int noteDuration = 1000 / durations[thisNote];
        tone(BUZZER_PIN, melody[thisNote], noteDuration);

        // Para distinguir as notas, define um tempo mínimo entre elas.
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
        noTone(BUZZER_PIN);
    }
}

void setup() {
    Serial.begin(9600);
    SPI.begin();
    rfid.PCD_Init();

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

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

        if (isMasterCard(rfid.uid.uidByte, rfid.uid.size)) {
            Serial.println("Master card detected!");
            // Toca a melodia de sucesso
            playMelody(successMelody, successDurations, sizeof(successMelody) / sizeof(successMelody[0]));
            // Ativa o relé
            digitalWrite(RELAY_PIN, HIGH);
            delay(1000); // Mantém o relé ativado por 1 segundo
            digitalWrite(RELAY_PIN, LOW);
        } else {
            // Toca a melodia de falha
            playMelody(failMelody, failDurations, sizeof(failMelody) / sizeof(failMelody[0]));
        }
    }
    delay(500);
}
