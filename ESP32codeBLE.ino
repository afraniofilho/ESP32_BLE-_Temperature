#include <Wire.h>                //Biblioteca para I2C
#include <DHT.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

#define FONT FreeSans9pt7b

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 15      
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


BLECharacteristic *characteristicTX; //através desse objeto iremos enviar dados para o client

bool deviceConnected = false; //controle de dispositivo conectado

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "ab0828b1-198e-4351-b779-901fa0e0371e" // UART service UUID
#define CHARACTERISTIC_UUID_RX "4ac8a682-9736-4e5d-932b-e9b31405049c"
#define CHARACTERISTIC_UUID_TX "0972EF8C-7613-4075-AD52-756F33D4DA91"

const int LED = 2; //LED interno do ESP32 (esse pino pode variar de placa para placa) // Could be different depending on the dev board. I used the DOIT ESP32 dev board.


//callback para receber os eventos de conexão de dispositivos
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      display.clearDisplay();
      display.setFont(&FONT);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(10, 30);
      display.println(F("Desconectado"));
      display.display();
      delay(2000);
      
    }
};

//callback  para envendos das características
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {
      //retorna ponteiro para o registrador contendo o valor atual da caracteristica
      std::string rxValue = characteristic->getValue(); 
      //verifica se existe dados (tamanho maior que zero)
      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }

        Serial.println();

        // Do stuff based on the command received
        if (rxValue.find("L1") != -1) { 
          Serial.print("Turning LED ON!");
          digitalWrite(LED, HIGH);
        }
        else if (rxValue.find("L0") != -1) {
          Serial.print("Turning LED OFF!");
          digitalWrite(LED, LOW);
        }

        Serial.println();
        Serial.println("*********");
      }
    }
};

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.cp437(true);
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
      display.clearDisplay();
      display.setFont(&FONT);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(10, 30);
      display.println(F("Desconectado"));
      display.display();

  pinMode(LED, OUTPUT);

  // Create the BLE Device
  BLEDevice::init("ESP32-BLE"); // nome do dispositivo bluetooth
  // Create the BLE Server
  BLEServer *server = BLEDevice::createServer(); //cria um BLE server 
  server->setCallbacks(new ServerCallbacks()); //seta o callback do server
  // Create the BLE Service
  BLEService *service = server->createService(SERVICE_UUID);
  // Create a BLE Characteristic para envio de dados
  characteristicTX = service->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  characteristicTX->addDescriptor(new BLE2902());

  // Create a BLE Characteristic para recebimento de dados
  BLECharacteristic *characteristic = service->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  characteristic->setCallbacks(new CharacteristicCallbacks());
  // Start the service
  service->start();
  // Start advertising (descoberta do ESP32)
  server->getAdvertising()->start();

  
  Serial.println("Waiting a client connection to notify...");

}

void loop() {
     //se existe algum dispositivo conectado
    if (deviceConnected) {
      //recupera a leitura da temperatura do ambiente
      float tempAmbiente = dht.readTemperature();

      display.clearDisplay();
      display.setCursor(10, 20);
      display.print(F("Conectado"));
      display.setCursor(10, 50);
      display.print(F("T:"));
      display.print(tempAmbiente); //indicando a temperatura
      display.write(167);
      display.display();      // Show initial text
      
      char txString[8]; // make sure this is big enuffz
      dtostrf(tempAmbiente, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

      characteristicTX->setValue(txString); //seta o valor que a caracteristica notificará (enviar)       
      characteristicTX->notify(); // Envia o valor para o smartphone

      Serial.print("*** Sent Value: ");
      Serial.print(txString);
      Serial.println(" ***");
  }
  delay(2000);

}
