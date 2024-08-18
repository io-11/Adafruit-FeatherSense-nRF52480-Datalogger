/*
 * nrf52840 - Bluetooth Data Logger inspired by MJRoBot (Marcelo Rovai) https://www.hackster.io/mjrobot/sensor-datalogger-50e44d
 * Generic external data via I2C (in this case temperature sensor LM75A by Texas Instruments)
 * 
 * Guido Facco
 * 9/7/2024
*/

#include <bluefruit.h> // adafruit ble
#include <Arduino.h>  //serve?
#include "Wire.h" //Libreria arduino per gestione I2C


#define LM75A_ADDRESS 0x48 // Indirizzo I2C del sensore LM75A
#define TEMPERATURE_REGISTER 0x00 //Indirizzo del registro temperatura dove ci sono i dati

int minPeriod=350;  //definisco il periodo minimo in millisecondi di lettura dei dati. 
                    //Dipende dal sensore che si è collegato. Nel mio caso LM75A ha un periodo minimo di 300ms.
                    //Lo metto all'inizio in modo da permettere facili modifiche

/* Online GUID / UUID Generator:
https://www.guidgenerator.com/online-guid-generator.aspx
64cf715d-f89e-4ec0-b5c5-d10ad9b53bf2
*/

// UUid for Service (Universal Unique IDentifier)
const char* UUID_serv = "64cf715d-f89e-4ec0-b5c5-d10ad9b53bf2";

// UUids for temperature data
const char* UUID_temp   = "64cf715e-f89e-4ec0-b5c5-d10ad9b53bf2";



// BLE identificazione
BLEDis bledis;
// BLE Service instanziazione
BLEService tempServ = BLEService(UUID_serv); 

// BLE Characteristics instanziazione
BLECharacteristic  tempSens = BLECharacteristic(UUID_temp);
    //BLEFloatCharacteristic  chAY(UUID_ay,  BLERead|BLENotify);


//dichiaro le variabili dei led
int LEDR=13;
int LEDB=4;
int connect = 0;  //indica lo stato di connessione 0-non_conn  1-conn

void setup() 
{
  Wire.begin(); //attivo I2C
  Serial.begin(115200);
  setReadRegister();  //funzione definita e scritta a fine pagina , serve per indicare quale registro voglio leggere




  /////////RIMUOVERE SE NON CONNESSO A PC
 while (!Serial); 
  /////////////     






  Serial.println("");
  Serial.println("--------------------------------------------------------------");
  Serial.println("BLE - Data Logger(Setup)");
  
  bool err=false; //flag di errore

  pinMode(LEDR, OUTPUT); // onboard led red set out
  pinMode(LEDB, OUTPUT); // onboard led blue (connection status led) set out
  digitalWrite(LEDR, LOW); // led red off
  digitalWrite(LEDB, LOW); // led blue off

  // init BLE
    
  if (!Bluefruit.begin()) 
  {
    Serial.println("BLE: failed");
    err=true;
  }
  Bluefruit.setName("Datalogger nRF52840");
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Serial.println("BLE: ok");

  // error: flash led forever
  if (err)
  {
    Serial.println("Init error. System halted (non è stato possibile inizializzare ble)");
    while(1)
    {
      digitalWrite(LEDR, LOW);  //led off
      delay(500); 
      digitalWrite(LEDR, HIGH); // led on
      delay(500);
 
    } 
  }

  // BLE service
  // correct sequence:
  // set BLE name > advertised service > add characteristics > add service > set initial values > advertise

 // Configure and Start Device Information Service
  bledis.setManufacturer("Unipd(GF)");
  bledis.setModel("Bluefruit Feather Sense nRF52840");
  bledis.begin();
  
  // attivare advertised Service
  
  tempServ.begin();
  
  // attivare characteristics to the Service
  tempSens.setProperties(CHR_PROPS_NOTIFY);    //non può ricevere dati dall'esterno
  tempSens.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);  //sola lettura da un dispositivo esterno
  tempSens.setFixedLen(4);    //dati di 4 byte (long)
  tempSens.begin();
     
  // start advertising
 // Set up and start advertising
  startAdv();
  Serial.println("Advertising started");
  Serial.println("Bluetooth device active, waiting for connections...");
  
}


  
int check=0;  //variabile ausiliaria usata per mostrare solo una volta(==0) l'avviso di riconnessione sul serial monitor
int i=0;
float t=0;


void loop() 
{
  //queso if permette di visualizzare 1 sola volra il messaggio di riconnessione sul serial monitor quando la connessione è persa
  if(!Bluefruit.connected() && connect>=1 && check==0)
  {
    // central disconnected:
    Serial.println("Disconnected from the device...  Reconnecting...");
    check=1;
    connect=0;
  }

  long preMillis = millis();  //tempo di riferimento va dichiarato fuori dal while

  //questo invece esegue il necessario per leggere il dato dal registro del sensore e inviarlo tramite BLe
  while (Bluefruit.connected()) 
  {    
    if(connect==0)
    {
      connect=1;
      delay(1000);//per evitare conflitti su connessione ble
    }
    check=0;

    //eseguo la lettura ogni minPeriod ma con una funzione non bloccante 

    
    if(millis()>=preMillis+minPeriod) //se il tempo letto ora è maggiore a quello precedente+il periodo indicato all'inizio, entro nel corpo dell'if
    {
    preMillis=millis(); //aggiorno il valore del tempo salvato
    float t= readTemperature(); //non è una funzione di libreria , ma è definita e scritta sotto
    Serial.print(i);
    Serial.print("  Temperature: ");
    Serial.print(t);
    Serial.println(" °C");
    tempSens.notify32(t); 
    i++;
    }
    
  } // still here while central connected

  
} 



////////////////////////////////////////////////////////////////////////////////////////////////////////
// Legge la temperatura e trasforma i bit nel valore della temperatura in °C
float readTemperature() 
{
  Wire.requestFrom(LM75A_ADDRESS, 2);//prepara la comunicazione con il sensore mandando l'indirizzo del sensore seguito da un 1(read) e si aspetta 2 byte di trasmissione, dopo i quali manda uno stop
  //while (Wire.available() < 2);
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

  int16_t temperature = (msb << 8) | lsb;
  float temperatureC = temperature / 256.0; // Conversione dei dati grezzi in gradi Celsius
  return temperatureC;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
// Setta il registro in cui voglio leggere i dati della temperatura
void setReadRegister()
{
  Wire.beginTransmission(LM75A_ADDRESS); //prepara la comunicazione con il sensore mandando l'indirizzo del sensore seguito da un0 0(write)
  Wire.write(TEMPERATURE_REGISTER); //scrive nel sensore l'indirizzo del registro da leggere
  Wire.endTransmission();//chiude questa comunicazione
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
// Setta e fa partire l'advertising del BLE
void startAdv(void)
{  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  
  /* Start Advertising
  * - Enable auto advertising if disconnected
  * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
  * - Timeout for fast mode is 30 seconds
  * - Start(timeout) with timeout = 0 will advertise forever (until connected)
  * 
  * For recommended advertising interval
  * https://developer.apple.com/library/content/qa/qa1931/_index.html   
  */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Lettura e scrittura nel serial plotter se connesso del nome del dispositivo a cui ci si è connessi
void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

