#include <bluefruit.h> // adafruit ble
#include <Adafruit_Sensor.h> //per usare i sensori sulla scheda
#include <Adafruit_BMP280.h>  //libreria sensore per temp
#include <Arduino.h>  
#include <Wire.h> //I2C

#define LM75A_ADDRESS 0x48 // Indirizzo I2C del sensore LM75A
#define TEMPERATURE_REGISTER 0x00 //Indirizzo del registro temperatura dove ci sono i dati




//dichiaro le variabili dei led
int LEDR=13;
int LEDB=4;
int connect = 0;

void setup() 
{
  Wire.begin(); //attivo I2C
  Serial.begin(115200);

  /////////COMMENTARE SE NON CONNESSO A PC
  while (!Serial); 
  /////////////     
  Serial.println("");
  Serial.println("--------------------------------------------------------------");
  Serial.println("Test I2C keysight\n");
  

  pinMode(LEDR, OUTPUT); // onboard led red 
  digitalWrite(13, LOW); // led red off
}

  
  
  void loop() 
  {

   LEDR=1;
    int16_t temperatureRaw = readTemperature(); //non è una funzione di libreria , ma è definita sotto
    float temperatureC = temperatureRaw / 256.0; // Conversione dei dati grezzi in gradi Celsius

    Serial.print("Temperature: ");
    Serial.print(temperatureC);
    Serial.println(" C");
    delay(350);//la max velocità è di 300ms ma mi tengo un pò di margine
  }

      


 int16_t readTemperature() 
 {
  Wire.beginTransmission(LM75A_ADDRESS); //prepara la comunicazione con il sensore mandando l'indirizzo del sensore seguito da un0 0(write)
  Wire.write(TEMPERATURE_REGISTER); //scrive nel sensore l'indirizzo del registro da leggere
  Wire.endTransmission();//chiude questa comunicazione

  Wire.requestFrom(LM75A_ADDRESS, 2);//prepara la comunicazione con il sensore mandando l'indirizzo del sensore seguito da un 1(read) e si aspetta 2 byte di trasmissione, dopo i quali manda uno stop
  //while (Wire.available() < 2);
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

  int16_t temperature = (msb << 8) | lsb;
  return temperature;
  }

