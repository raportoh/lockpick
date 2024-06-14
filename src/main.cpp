#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

#define SS_PIN 53
#define RST_PIN 5
#define RELAY_PIN 9
#define BUZZER_PIN 8
#define ESP8266_RX 18
#define ESP8266_TX 19

MFRC522 rfid(SS_PIN, RST_PIN);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

SoftwareSerial esp8266(ESP8266_RX, ESP8266_TX);

byte masterUID[] = {0x03, 0x89, 0xaf, 0x0d};
bool isOnline = false;

void checkESPConnection();

void displayStatus();

void displayMessage(const String &message);

void dump_byte_array(byte *buffer, byte bufferSize);

bool isMasterCard(byte *buffer, byte bufferSize);

String checkUserInCloud(const String &uid);

void playMelody(int melody[], int durations[], int size);

void sendHttpRequest(String payload);

void logSerialESP();

void setup() {
    Serial.begin(9600);
    esp8266.begin(9600);
    SPIClass::begin();
    rfid.PCD_Init();

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 initialization failed"));
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();
    delay(2000);
    display.clearDisplay();

    checkESPConnection();
    displayStatus();
}

void checkESPConnection() {
    esp8266.println("AT");
    delay(1000);
    if (esp8266.find(*"OK")) {
        isOnline = true;
        Serial.println("ESP-01 is online.");
    } else {
        isOnline = false;
        Serial.println("ESP-01 is offline.");
    }
}

void displayStatus() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(isOnline ? "Modo Online" : "Modo Offline");
    display.display();
}

void displayMessage(const String &message) {
    display.clearDisplay();
    displayStatus();
    display.setCursor(0, 10); // Posiciona a mensagem abaixo do status
    display.println(message);
    display.display();
}

void dump_byte_array(byte *buffer, byte bufferSize) {
    String uidString;
    for (byte i = 0; i < bufferSize; i++) {
        uidString += String(buffer[i] < 0x10 ? "0" : "");
        uidString += String(buffer[i], HEX);
    }
    Serial.print("UID:");
    Serial.println(uidString);

    displayMessage("UID: " + uidString);
}

bool isMasterCard(byte *buffer, byte bufferSize) {
    if (bufferSize != sizeof(masterUID)) return false;
    for (byte i = 0; i < bufferSize; i++) {
        if (buffer[i] != masterUID[i]) return false;
    }
    return true;
}

String checkUserInCloud(const String &uid) {
    // Simula a verificação de um usuário cadastrado
    // Em um cenário real, seira feito uma solicitação HTTP para o endpoint da AWS para verificar o usuário
    if (uid == "12345678") {
        return "Admin";
    } else if (uid == "87654321") {
        return "User";
    }
    return "";
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

void sendHttpRequest(String payload) {
    if (isOnline) {
        Serial.print("Sending to ESP: ");
        Serial.println(payload);
        esp8266.println(payload);
    } else {
        Serial.println("ESP-01 is offline. Cannot send request.");
    }
}

void logSerialESP() {
    while (esp8266.available()) {
        char c = esp8266.read();
        Serial.write(c);
    }
}

void loop() {
    if (rfid.PICC_IsNewCardPresent()) {
        if (rfid.PICC_ReadCardSerial()) {
            displayMessage("Realizando Leitura...");

            String uidString;
            for (byte i = 0; i < rfid.uid.size; i++) {
                uidString += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
                uidString += String(rfid.uid.uidByte[i], HEX);
            }

            if (isMasterCard(rfid.uid.uidByte, rfid.uid.size)) {
                Serial.println("Master card detected!");

                // Toca a melodia de sucesso
                int successMelody[] = {262, 294, 330, 349, 392}; // C4, D4, E4, F4, G4
                int successDurations[] = {8, 8, 8, 8, 8};
                playMelody(successMelody, successDurations, sizeof(successMelody) / sizeof(successMelody[0]));

                // Ativa o relé
                digitalWrite(RELAY_PIN, HIGH);
                delay(1000); // Mantém o relé ativado por 1 segundo
                digitalWrite(RELAY_PIN, LOW);

                // Envia mensagem SNS
                String message = R"({"default": "User Accessed: Master at )" + String(millis()) + "\"}";
                sendHttpRequest(message);

                // Atualiza o display
                displayMessage("Acesso Permitido:\nMaster");

            } else {
                String userName = checkUserInCloud(uidString);
                if (userName != "") {
                    Serial.println("User detected: " + userName);

                    // Toca a melodia de sucesso
                    int successMelody[] = {262, 294, 330, 349, 392}; // C4, D4, E4, F4, G4
                    int successDurations[] = {8, 8, 8, 8, 8};
                    playMelody(successMelody, successDurations, sizeof(successMelody) / sizeof(successMelody[0]));

                    // Ativa o relé
                    digitalWrite(RELAY_PIN, HIGH);
                    delay(1000); // Mantém o relé ativado por 1 segundo
                    digitalWrite(RELAY_PIN, LOW);

                    // Envia mensagem SNS
                    String message = R"({"default": "User Accessed: )" + userName + " at " + String(millis()) + "\"}";
                    sendHttpRequest(message);

                    // Atualiza o display
                    displayMessage("Acesso Permitido:\n" + userName);

                } else {
                    // Toca a melodia de falha
                    int failMelody[] = {659, 523, 587, 523, 311}; // E5, C5, D5, C5, DS4
                    int failDurations[] = {8, 8, 8, 8, 8};
                    playMelody(failMelody, failDurations, sizeof(failMelody) / sizeof(failMelody[0]));

                    // Atualiza o display
                    displayMessage("Acesso Negado");
                }
            }
            delay(2000); // Delay para manter a mensagem visível por 2 segundos
            display.clearDisplay();
            displayStatus(); // Atualiza o display com o status online/offline após cada operação
        }
    }

    logSerialESP(); // Log the ESP-01 serial output to the Arduino serial monitor
    delay(500);
}
