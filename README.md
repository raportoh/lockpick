# LockPick

# Projeto Arduino Mega com RFID MFRC522, Display OLED e Relé

Este projeto utiliza um Arduino Mega para ler cartões RFID utilizando o módulo MFRC522, exibir informações em um display OLED e controlar um relé.

## Pinagens Utilizadas

### RFID MFRC522
- **SDA (SS_PIN)**: Pino 53
- **SCK**: Pino 52
- **MOSI**: Pino 51
- **MISO**: Pino 50
- **RST**: Pino 5
- **GND**: GND
- **3.3V**: 3.3V

### Display OLED
- **VCC**: 5V
- **GND**: GND
- **SDA**: Pino 20 (SDA do Arduino Mega)
- **SCL**: Pino 21 (SCL do Arduino Mega)

### Relé
- **S (Signal)**: Pino 9
- **- (GND)**: GND
- **+ (VCC)**: 5V

### ESP8266
- **VCC**: 3.3V
- **GND**: GND
- **RX**: Pino 11 (TX)
- **TX**: Pino 10 (RX)

## Código

O código para este projeto está configurado para inicializar e utilizar todos os componentes mencionados acima. Certifique-se de que as bibliotecas necessárias estão instaladas no PlatformIO.

## Dependências

Adicione as seguintes dependências no seu arquivo `platformio.ini`:

```ini
[env:megaatmega2560]
platform = atmelavr
board = megaatmega2560
framework = arduino
lib_deps =
    miguelbalboa/MFRC522 @ ^1.4.10
    adafruit/Adafruit SSD1306 @ ^2.4.7
    adafruit/Adafruit GFX Library @ ^1.10.10
    paulstoffregen/SoftwareSerial @ ^1.0
