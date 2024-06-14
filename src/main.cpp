#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
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

const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";
const char *awsEndpoint = "YOUR_AWS_IOT_ENDPOINT";
const char *topic = "YOUR_SNS_TOPIC_ARN";

byte masterUID[] = {0x03, 0x89, 0xaf, 0x0d};

void connectWiFi();

String sendATCommand(const char *cmd, int timeout, bool debug);

void setup() {
    Serial.begin(9600);
    esp8266.begin(115200);
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
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();
    delay(2000);
    display.clearDisplay();

    connectWiFi();
}

void connectWiFi() {
    sendATCommand("AT+RST", 1000, true);
    sendATCommand("AT+CWMODE=1", 1000, true);
    String cmd = "AT+CWJAP=\"" + String(ssid) + "\",\"" + String(password) + "\"";
    sendATCommand(cmd.c_str(), 5000, true);
}

String sendATCommand(const char *cmd, int timeout, bool debug) {
    String response = "";
    esp8266.print(cmd);
    long int time = millis();
    while ((time + timeout) > millis()) {
        while (esp8266.available()) {
            char c = esp8266.read();
            response += c;
        }
    }
    if (debug) {
        Serial.print(response);
    }
    return response;
}

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

void sendSNSTopic(String message) {
    String atCommand = "AT+CIPSTART=\"TCP\",\"" + String(awsEndpoint) + "\",443";
    sendATCommand(atCommand.c_str(), 10000, true);

    String httpRequest = "POST /sns HTTP/1.1\r\n";
    httpRequest += "Host: " + String(awsEndpoint) + "\r\n";
    httpRequest += "Content-Type: application/json\r\n";
    httpRequest += "Content-Length: " + String(message.length()) + "\r\n\r\n";
    httpRequest += message;

    String sendCommand = "AT+CIPSEND=" + String(httpRequest.length());
    sendATCommand(sendCommand.c_str(), 1000, true);
    sendATCommand(httpRequest.c_str(), 1000, true);

    sendATCommand("AT+CIPCLOSE", 1000, true);
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

void loop() {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        dump_byte_array(rfid.uid.uidByte, rfid.uid.size);

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
            sendSNSTopic(message);
        } else {
            // Toca a melodia de falha
            int failMelody[] = {659, 523, 587, 523, 311}; // E5, C5, D5, C5, DS4
            int failDurations[] = {8, 8, 8, 8, 8};
            playMelody(failMelody, failDurations, sizeof(failMelody) / sizeof(failMelody[0]));
        }
    }
    delay(500);
}
