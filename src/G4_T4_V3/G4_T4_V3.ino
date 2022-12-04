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
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Ports
int buzzer = 10;
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
// Instanciate the temperature sensor
Adafruit_AHTX0 aht;

// Variables to save time
String enterTime;
String exitTime;
int totalTime;
float firstDateHour;
float firstDateMinutes;
float secondDateHour;
float secondDateMinutes;

// variables for POSTs
int control = 0;
String idQuery;

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
// Initiate RFID library
MFRC522 rfidBase = MFRC522(RFID_SS_SDA, RFID_RST);
//Creating a class for RFID reader
class RFIDreader{
  private:
  // Variables to read the RFID card
    char readRFIDcode[100] = "";
    char expectedRFIDcode[100] = "";
    MFRC522 *rfid = NULL;
    int detectedCard = 0;
    int readCard = 0;
    //Function that processes the read code
    void processReadCode(){
      char code[3*rfid->uid.size+1];
      code[0] = 0;
      char temp[10];
      for(int i=0; i < rfid->uid.size; i++){
        sprintf(temp,"%X",rfid->uid.uidByte[i]);
        strcat(code,temp);
      }
      code[3*rfid->uid.size+1] = 0;
      strcpy(readRFIDcode,code);
      Serial.println(readRFIDcode);
    }
  public:
    RFIDreader(MFRC522 *reader){
      rfid = reader;
      rfid->PCD_Init();
    };
    //Identify the card type
    char *cardType(){
      MFRC522::PICC_Type piccType = rfid->PICC_GetType(rfid->uid.sak);
      Serial.println(rfid->PICC_GetTypeName(piccType));
      return((char *)rfid->PICC_GetTypeName(piccType));
    };
    //Function that detect if a card is close to the read
    int cardIsPresent(){
      return(detectedCard);
    };
    //Function that checks if the card code was read
    int cardWasRead(){
      return(readCard);
    };
    //Function that reads the card code
    void readingCard(){
      //Check and prints if a card is close to the reader
      if (rfid->PICC_IsNewCardPresent()) { // new tag is available
        Serial.println("Cartão presente");
        detectedCard = 1;
        //Reads the card's code
        if (rfid->PICC_ReadCardSerial()) { // NUID has been readed
          Serial.println("Cartão lido");
          readCard = 1;
          processReadCode();
          rfid->PICC_HaltA(); // halt PICC
          rfid->PCD_StopCrypto1(); // stop encryption on PCD
          tone(buzzer, 3000, 500); //Play a sound to let people know it read successufully
          delay(1000);
        }
      }
      //Else, no card was detected
      else{
        detectedCard = 0;
      }
    };
    //Card was already read
    char *CardWasRead(){
      return(readRFIDcode);
    };
    //Reset and prepare for a new reading
    void resetReading(){
      detectedCard = 0;
      readCard = 0;
    }
};
RFIDreader *reader = NULL;

String getDate(void) {
  // Ensure that we get a valid time
  // Sometimes the NTP Client retrieves 1970. To ensure that doesn't happen we need to force the update.
  timeClient.update();
  // Convert time to a readable format
  String formattedDate = timeClient.getFormattedTime();
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

String getId(){
  HTTPClient http;

  // Server address  
  String serverAddress = "http://10.128.65.251:3031/idQuery";

  // Begin http in the server
  http.begin(serverAddress);

  // Type of file: json
  http.addHeader("Content-Type", "application/json");
  StaticJsonDocument<200> doc;
  doc["placa"] = "ABC0D29";
  String requestBody;
  serializeJson(doc, requestBody);
  int httpResponseCode = http.POST(requestBody);
  if(httpResponseCode>0){
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  return response;
  }
}

// Function to make POST requests to  the database, posting information from the esp functionalities
void postDataToServer(String endpoint, int time, String receivingTime, String parkingTime, int docControl, String id) {
  Serial.println("Posting JSON data to server...");
  // Block until we are able to connect to the WiFi access point
    HTTPClient http;
    //endereço do servidor
    String serverAddress = "http://10.128.65.251:3031/" + endpoint;

    http.begin(serverAddress);
    http.addHeader("Content-Type", "application/json");
    StaticJsonDocument<200> doc;

    // Add values in the document
    if (docControl == 0){
      doc["placa"] = "ABC0D29";
      doc["manobristaIda"] = "CFM00";
      doc["tempoEstimado"] = time;
      doc["horarioRecebimento"] = receivingTime;
      doc["horarioEstacionamento"] = parkingTime;
    }

    else if (docControl == 1) {
      doc["idQuery"] = id;
      doc["placa"] = "ABC0D29";
      doc["tempoEstimado"] = time;
    }

    else{
      doc["idQuery"] = id;
      doc["manobristaVolta"] = "AWD11";
      doc["horarioRecebimento"] = receivingTime;
      doc["horarioEstacionamento"] = parkingTime;
    }

    // Send the request body via POST method
    String requestBody;
    serializeJson(doc, requestBody);
    Serial.println(requestBody);
    int httpResponseCode = http.POST(requestBody);
    Serial.println(httpResponseCode);
    if(httpResponseCode>0){
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    }
}

// Function to get the time when the car is received
void getEnterTime() {
  // get actual hour
  enterTime = getDate();
  firstDateHour = timeClient.getHours();
  firstDateMinutes = timeClient.getMinutes();
}

// Function to connect to the beacon FTM
void connectFTM(){
  // Connect to AP that has FTM Enabled
  WiFi.begin(ssidFTM, passwordFTM);
  while (WiFi.status() != WL_CONNECTED) {
    lcd.print("Conectando...");
    delay(1000);
    lcd.clear();
  }
}

// Function to print in lcd wifi successful connection
void lcdPrintConnectedWifi() {
  lcd.clear();
  lcd.print("Wi-fi conectado!");
  digitalWrite(buzzer, HIGH); // turn on buzzer
  tone(buzzer, 2000, 300);
  lcd.setCursor(4, 1);
  lcd.println(enterTime);
  digitalWrite(buzzer, LOW); // turn off buzzer
  cardReadOnce = 1;
  delay(1000);
}

// Function to print in lcd the temperature and distance to the beacon
void lcdPrintTemperatureDistance() {
  lcd.clear();
  sensors_event_t humidity, temp;
  //get the temperature of the sensor readings
  aht.getEvent(&humidity, &temp);
  float temperature = temp.temperature;
  messageTemperature(temperature);
  delay(2000);
  lcd.clear();
  // Get the distances
  lcd.setCursor(0, 0);
  getFtmReport();
  delay(2000);
  lcd.clear();
}

// Function to print in lcd wifi successful disconnection
void lcdPrintDisconnectedWifi() {
  digitalWrite(buzzer, HIGH); // turn on buzzer
  tone(buzzer, 4440, 200);
  digitalWrite(buzzer, LOW); //turn off buzzer
  lcd.setCursor(0, 0);
  lcd.print("Wi-fi desconectado");
  WiFi.begin(ssid, password);
}

// Function to get the time when the car is parked
void getExitTime() {
  exitTime = getDate();
  lcd.println(exitTime);
  secondDateHour = timeClient.getHours();
  secondDateMinutes = timeClient.getMinutes();
  int timeHour = secondDateHour - firstDateHour;
  int timeMinutes = secondDateMinutes - firstDateMinutes;
  totalTime;
  if (timeHour == 0) {
    totalTime = timeMinutes;
  }
  else {
    totalTime = timeMinutes + (timeHour*60);
  }
}

void setup() {
  // Begin serial at port 115200
  Serial.begin(115200);
  SPI.begin();
  pinMode(buzzer, OUTPUT);
  reader = new RFIDreader(&rfidBase);
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
  reader->readingCard();
  if(reader->cardWasRead()){
    Serial.println(reader->CardWasRead());
    reader->resetReading();
    // When the RFID card is read for the first time, the esp will disconnect from local network and connect to FTM. The display will show the connection status, the hour of activation,
    // the temperature and the distances beetween the esp's.
    if (cardReadOnce == 0) {
      getEnterTime();
      WiFi.disconnect();
      ftmSemaphore = xSemaphoreCreateBinary();
      // Listen for FTM Report events
      WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
      connectFTM();
      // Print in the lcd the time and play a sound when Wifi is connected
      if (WiFi.status() == 3) {
        lcdPrintConnectedWifi();
      }
      // Print in the lcd if Wifi is not connected
      else if (WiFi.status() == WL_CONNECT_FAILED) {
        lcd.println("Falha na conexao");
      }
      delay(2000);
      lcdPrintTemperatureDistance();
      if (control == 1) {
        postDataToServer("retornoCarro", totalTime, "", "", control, "");
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
        lcdPrintDisconnectedWifi();
        lcd.setCursor(4, 1);
        getExitTime();
        cardReadOnce = 0;

        // Posting information in the database depending how many times the RFID was passed
        // RFID for the second time
        if (control == 0) {
          postDataToServer("insertRecebimento", totalTime, enterTime, exitTime, control, "");
          control += 1;
        }
        // RFID for the third time
        else if (control == 2){
          idQuery = getId();
          postDataToServer("insertRetirada", 0, enterTime, exitTime, control, idQuery);
          control = 0;
        }
      }
      delay(1000);
    }
    delay(1000);
    lcd.clear();
  }
}