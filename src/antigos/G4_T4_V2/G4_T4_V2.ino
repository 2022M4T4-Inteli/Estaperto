// Libraries used in the project
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#define RFID_SS_SDA   21
#define RFID_RST      14
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_AHTX0.h>

// Ports
int buzzer = 10;
int iniciar = 0;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Instanciate the temperature sensor
Adafruit_AHTX0 aht;

// Variable to save time
String formattedDate;

// Creating the lcd element from the adress 0x27 with 16 columns and two rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Local Wifi network name and password
const char* ssid = "Inteli-COLLEGE";
const char* password = "QazWsx@123";

// FTM Wifi network name and password
const char* ssidFTM = "Estaperto";
const char* passwordFTM = "40028922";

// FTM settings
// Number of FTM frames requested in terms of 4 or 8 bursts (allowed values - 0 (No pref), 16, 24, 32, 64)
const uint8_t FTM_FRAME_COUNT = 16;

// Requested time period between consecutive FTM bursts in 100’s of milliseconds (allowed values - 0 (No pref) or 2-255)
const uint16_t FTM_BURST_PERIOD = 2;

// Semaphore to signal when FTM Report has been received
xSemaphoreHandle ftmSemaphore;

// Status of the received FTM Report
bool ftmSuccess = true;

// FTM report handler with the calculated data from the round trip
void onFtmReport(arduino_event_t *event) {
  const char * status_str[5] = {"SUCCESS", "UNSUPPORTED", "CONF_REJECTED", "NO_RESPONSE", "FAIL"};
  wifi_event_ftm_report_t * report = &event->event_info.wifi_ftm_report;
  // Set the global report status
  ftmSuccess = report->status == FTM_STATUS_SUCCESS;
  if (ftmSuccess) {
    // Calculating the distance using FTM
    lcd.print((float)report-> dist_est / 100.0 - 39.7);
    lcd.setCursor(4, 0);
    lcd.print("m");
    // Pointer to FTM Report with multiple entries, should be freed after use
    free(report->ftm_report_data);
  } else {
    Serial.print("FTM Error: ");
    Serial.println(status_str[report->status]);
  }
  // Signal that report is received
  xSemaphoreGive(ftmSemaphore);
};

// Initiate FTM Session and wait for FTM Report
bool getFtmReport(){
  if(!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD)){
    Serial.println("FTM Error: Initiate Session Failed");
    return false;
  }
  // Wait for signal that report is received and return true if status was success
  return xSemaphoreTake(ftmSemaphore, portMAX_DELAY) == pdPASS && ftmSuccess;
}

int cardReadOnce = 0;
MFRC522 rfidBase = MFRC522(RFID_SS_SDA, RFID_RST);
class LeitorRFID{
  private:
    char codigoRFIDLido[100] = "";
    char codigoRFIDEsperado[100] = "";
    MFRC522 *rfid = NULL;
    int cartaoDetectado = 0;
    int cartaoJaLido = 0;
    void processaCodigoLido(){
      char codigo[3*rfid->uid.size+1];
      codigo[0] = 0;
      char temp[10];
      for(int i=0; i < rfid->uid.size; i++){
        sprintf(temp,"%X",rfid->uid.uidByte[i]);
        strcat(codigo,temp);
      }
      codigo[3*rfid->uid.size+1] = 0;
      strcpy(codigoRFIDLido,codigo);
      Serial.println(codigoRFIDLido);
    }
  public:
    LeitorRFID(MFRC522 *leitor){
      rfid = leitor;
      rfid->PCD_Init();
    };
    char *tipoCartao(){
      MFRC522::PICC_Type piccType = rfid->PICC_GetType(rfid->uid.sak);
      Serial.println(rfid->PICC_GetTypeName(piccType));
      return((char *)rfid->PICC_GetTypeName(piccType));
    };
    int cartaoPresente(){
      return(cartaoDetectado);
    };
    int cartaoFoiLido(){
      return(cartaoJaLido);
    };
    void leCartao(){
      if (rfid->PICC_IsNewCardPresent()) { // new tag is available
        iniciar = 7;
        Serial.println("Cartao presente");
          while (iniciar != 0){
            iniciar -= 1;
          }
        cartaoDetectado = 1;
        if (rfid->PICC_ReadCardSerial()) { // NUID has been readed
          Serial.println("Cartao lido");
          cartaoJaLido = 1;
          processaCodigoLido();
          rfid->PICC_HaltA(); // halt PICC
          rfid->PCD_StopCrypto1(); // stop encryption on PCD
          tone(buzzer, 3000, 500);
          delay(1000);
        }
      }else{
        cartaoDetectado = 0;
        iniciar = 10;
      }
    };
    char *cartaoLido(){
      return(codigoRFIDLido);
    };
    void resetarLeitura(){
      cartaoDetectado = 0;
      cartaoJaLido = 0;
      iniciar = 10;
    }
};
LeitorRFID *leitor = NULL;

String getDate(void) {
  // Ensure that we get a valid time
  // Sometimes the NTP Client retrieves 1970. To ensure that doesn't happen we need to force the update.
  timeClient.update();
  // Convert time to a readable format
  formattedDate = timeClient.getFormattedTime();
  return formattedDate;
}

// Function to scroll the text in the display when the text is longer than 16 rows
void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i=0; i < lcdColumns; i++) {
    message = " " + message;
  }
  message = message + " ";
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

// Print in display the temperature if it is normal and when it is higher than 45ºC print an alert message and play an alert sound
void messageTemperature(double temp){
  String messageStatic = "Alerta!";
  String messageToScroll = "Temperatura do sistema acima de 45 graus";
  String messageToScroll2 = "Temperatura do sistema em estado critico";
  if(temp >= 45.0 && temp < 50.0){
    for (int i = 0; i <= 5; i++) {
      digitalWrite(buzzer, HIGH); // turn on buzzer
      tone(buzzer, 2000, 300);
      digitalWrite(buzzer, LOW); // turn off buzzer
      delay(500);
    }
    lcd.setCursor(0, 0);
    lcd.print(messageStatic);
    scrollText(1, messageToScroll, 250, 16);
  }
  else if(temp >= 50.0){
    lcd.setCursor(0, 0);
    lcd.print(messageStatic);
    scrollText(1, messageToScroll2, 250, 16);
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print("Temperatura:");
    lcd.setCursor(4, 1);
    lcd.print(temp);
    lcd.setCursor(9, 1);
    lcd.print(" C");
  }
}

void setup() {
  // Begin serial at port 115200
  Serial.begin(115200);
  SPI.begin();
  pinMode(buzzer, OUTPUT);

  leitor = new LeitorRFID(&rfidBase);

  // LCD ports
  Wire.begin(4, 5); // SDA, SCL

  // Temperature sensor ports  
  Wire1.begin(42, 41);

  // Begin the sensor at the correct ports
  aht.begin(&Wire1);

  // initialize LCD
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();

  // set cursor to first column, first row
  lcd.setCursor(0, 0);

  // Station mode: the ESP32 connects to an access point
  WiFi.mode(WIFI_STA);

  // Connect to local netork
  WiFi.begin(ssid, password);

  // Begin the NTP Client server
  timeClient.begin();

  // Brazil offset
  timeClient.setTimeOffset(-10800); //GTM -3 = -10800
}
void loop() {
  leitor->leCartao();
  if(leitor->cartaoFoiLido()){
    Serial.println(leitor->tipoCartao());
    Serial.println(leitor->cartaoLido());
    leitor->resetarLeitura();
    // When the RFID card is read for the first time, the esp will disconnect from local network and connect to FTM. The display will show the connection status, the hour of activation,
    // the temperature and the distances beetween the esp's.
    if (cardReadOnce == 0) {
      // get actual hour
      formattedDate = getDate();
      WiFi.disconnect();
      ftmSemaphore = xSemaphoreCreateBinary();

      // Listen for FTM Report events
      WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
      // Connect to AP that has FTM Enabled
      WiFi.begin(ssidFTM, passwordFTM);

      while (WiFi.status() != WL_CONNECTED) {
        lcd.print("Conectando...");
        delay(1000);
        lcd.clear();
      }
      // Print in the lcd the time and play a sound when Wifi is connected
      if (WiFi.status() == 3) {
        lcd.clear();
        lcd.print("Wi-fi conectado!");
        digitalWrite(buzzer, HIGH); // turn on buzzer
        tone(buzzer, 2000, 300);
        lcd.setCursor(4, 1);
        lcd.println(formattedDate);
        digitalWrite(buzzer, LOW); // turn off buzzer
        cardReadOnce = 1;
        delay(1000);
      }
      // Print in the lcd if Wifi is not connected
      else if (WiFi.status() == WL_CONNECT_FAILED) {
        lcd.println("Falha na conexao");
      }
      delay(2000);
      lcd.clear();
      sensors_event_t humidity, temp;

      //get the temperature of the sensor readings
      aht.getEvent(&humidity, &temp);
      float temperature = temp.temperature;
      messageTemperature(temperature);
      delay(2000);
      lcd.clear();

      // Get the distances
      for (int i = 0; i < 10; i++) {
        lcd.setCursor(0, 0);
        getFtmReport();
        delay(2000);
        lcd.clear();
      }
    }

    // When the RFID card is read for the second time the FTM is disconnected
    else if(cardReadOnce == 1) {
      WiFi.disconnect();
      // Print in the lcd the disconnecting message while Wifi is disconnecting
      while (WiFi.status() != WL_DISCONNECTED) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Desconectando...");
      }
      lcd.clear();
      // Print in the lcd a message when Wifi is disconnected
      if (WiFi.status() == WL_DISCONNECTED) {
        digitalWrite(buzzer, HIGH); // turn on buzzer
        tone(buzzer, 4440, 200);
        digitalWrite(buzzer, LOW); //turn off buzzer
        lcd.setCursor(5, 0);
        lcd.print("Wi-fi");
        lcd.setCursor(1, 1);
        lcd.println("desconectado!");
        cardReadOnce = 0;
      }
      delay(1000);
    }
    delay(1000);
    lcd.clear();
  }
}