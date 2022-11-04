#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variable to save time
String formattedDate;

// Functions to calculate the temperature according to the resistance
float getResistencia(int pin, float voltageUc, float adcResolutionUc, float resistenciaEmSerie);
float readTemperatureNTC(float resistenciaTermistor, float resistenciaResistorSerie, float voltageUc, float Beta);
float calcularCoeficienteBetaTermistor();

// Creating the lcd element from the adress 0x27 with 16 columns and two rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Wifi network name and password
const char* ssid = "Inteli-COLLEGE";
const char* password = "QazWsx@123";

// Ports
int button1 = 6;
int button2 = 7;
int buzzer = 10;


void setup() {
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  // LCD ports 
  Wire.begin(4, 5); // SDA, SCL

  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();

  // set cursor to first column, first row
  lcd.setCursor(0, 0);

  //Station mode: the ESP32 connects to an access point
  WiFi.mode(WIFI_STA);


  timeClient.begin();
  // Set offset time in seconds to adjust for timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(-10800); //GTM -3 = -10800

  Serial.begin(9600);
}

void loop() {

  float resistencia = getResistencia(13, 3.3, 4095.0, 10000.0);
  float beta = calcularCoeficienteBetaTermistor();
  float temperatura = readTemperatureNTC(resistencia,
                                         10000.0,
                                         3.3,
                                         beta);
  
  // Connect the Wifi when button is pressed
  if (digitalRead(button1) == LOW) {
    WiFi.begin(ssid, password);

    // Print in the lcd the connecting message while Wifi is connecting
    while (WiFi.status() != WL_CONNECTED) {
      lcd.print("Conectando...");
      delay(1000);
      lcd.clear();
    }
    
    // Print in the lcd the time and play a sound when Wifi is connected
    if (WiFi.status() == 3) {

      digitalWrite(buzzer, HIGH); // turn on buzzer
      tone(buzzer, 2000, 300);

      lcd.clear();
      lcd.print("Wi-fi conectado!");

      // The formattedDate comes with the following format:
      // 2018-05-28T16:00:13Z
      // We need to extract date and time
      formattedDate = getDate();

      lcd.setCursor(4, 1);
      lcd.println(formattedDate);

      //Serial.print("Temperatura:");
      //Serial.print(temperatura);
      //Serial.println("ºC");

      digitalWrite(buzzer, LOW); // turn off buzzer
      delay(1000);
    }

    // Print in the lcd if Wifi is not connected
    else if (WiFi.status() == 4) {
      lcd.println("Falha na conexao");
    }
    delay(2000);
    lcd.clear();
    messageTemperature(temperatura);
  }


  // Disconnect the Wifi when button is pressed
  if (digitalRead(button2) == LOW) {
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
    }
    delay(1000);
  }
}

// Function to get the actual time
String getDate(void) {

  // Ensure that we get a valid time
  // Sometimes the NTP Client retrieves 1970. To ensure that doesn't happen we need to force the update.
  while(!timeClient.update()) {
      timeClient.forceUpdate();
    }

  // Convert time to a readable format
  formattedDate = timeClient.getFormattedTime();
  return formattedDate;
}

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

float getResistencia(int pin, float voltageUc, float adcResolutionUc, float resistenciaEmSerie) {
  float resistenciaDesconhecida = 0;

  resistenciaDesconhecida =
    resistenciaEmSerie *
    (voltageUc /
     (
       (analogRead(pin) * voltageUc) /
       adcResolutionUc
     ) - 1
    );

  return resistenciaDesconhecida;
}

float readTemperatureNTC(float resistenciaTermistor,
                         float resistenciaResistorSerie,
                         float voltageUc,
                         float Beta) {
  // Local constants
  const double To = 298.15;    // Temperature in Kelvin for 25 degrees Celsius
  const double Ro = 10000.0;   // Thermistor's resistance in 25 degrees Celsius
  // Local variables
  double Vout = 0; // Voltage read from the thermistor's analog port.
  double Rt = 0; // Resistance read from the analog port.
  double T = 0; // Temperature in Kelvin.
  double Tc = 0; // Temperature in Celsius.
  Vout = resistenciaResistorSerie / (resistenciaResistorSerie + resistenciaTermistor) * voltageUc; //Calculation of the voltage read from the thermistor's analog port.
  Rt = resistenciaResistorSerie * Vout / (voltageUc - Vout); // Calculation of the analog port resistance.
  T = 1 /(1 / To + log(Rt / Ro) / Beta); // Application of the Beta parameter equation to obtain the Temperature in Kelvin.
  Tc = T - 273.15; // Kelvin to Celsius conversion.
  return Tc;
}

float calcularCoeficienteBetaTermistor() {
  float beta;
  const float T1 = 273.15;  // temperature value 0º C converted to Kelvin.
  const float T2 = 373.15;  // temperature value 100º C converted to Kelvin.
  const float RT1 = 27.219; // resistance value (in ohm) at temperature T1.
  const float RT2 = 0.974;  /// resistance value (in ohm) at temperature T2.
  beta = (log(RT1 / RT2)) / ((1 / T1) - (1 / T2));  // Beta calculation.
  return beta;
}
