#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include "RF24.h"
#include "ThingSpeak.h"
#include <WiFi.h>
WiFiClient  client;
#include <WiFiManager.h>  
#include "secrets.h"

#include "time.h"
// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

String SD_Message;

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key index number (needed only for WEP)
unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;


#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


RF24 radio(4,5); // (CE, CSN)
const byte address[6] = "1RF24"; // address / identifier

#define LED1  32
#define LED2  33
#define LED3  12

#define SW1   27
#define SW2   14

//CSV decode
  int Lsub[20];
  int Lind=1;
  long lastMsg = 0;
  char dummy;
  String data0,data1,data2,data3,data4,data5;
  float data0f,data1f,data2f,data3f,data4f,data5f;
  String string_text;

  int error_cnt=0;

  struct NRFstruct
  {
    String _sbeeid;
    String _scount;
    String _sbat;
    String _sweight;
    String _stemperature;
  };

  NRFstruct NRFreceive[8];

 int bufferlocation[8] = {5,6,7,8,9,10,11,12};

void setup() {
  Serial.begin(115200);
  Serial.println("Application started.....");
  Serial.println("========================");

  pinMode(LED1, OUTPUT);
  digitalWrite(LED1,HIGH);
  delay(300);
  digitalWrite(LED1,LOW);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED2,HIGH);
  delay(300);
  digitalWrite(LED2,LOW);
  pinMode(LED3, OUTPUT);
  digitalWrite(LED3,HIGH);
  delay(300);
  digitalWrite(LED3,LOW);

  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);

  //while(1)
  {
    
    if(!digitalRead(SW1)) Serial.println("SW1");
    if(!digitalRead(SW2)) Serial.println("SW2");
  }

  Init_SD();

  Serial.print("Initializing Display...");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
  { 
    Serial.println(F("\nSSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  Serial.println("Done!");
   
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Initializing..."));
  display.display();

  if (!radio.begin()) 
  {
    Serial.println(F("radio hardware is not responding!!"));
    display.println(F("NRF not responding!"));
    display.display();
    while (1) {}  // hold in infinite loop
  }
  //Serial.println(F("\n\n\n\n\n"));
  Serial.println(F("NRF24L01 started...."));
  Serial.println(F("=========================================="));
  display.println(F("NRF ... OK!"));
  display.display();
  radio.openReadingPipe(0,address); // set the address for pipe 0
  radio.startListening(); // set as receiver

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  if(!digitalRead(SW2))
  {
    digitalWrite(LED1,HIGH);
    wm.resetSettings();
    digitalWrite(LED1,LOW);
  }




  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result
  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
  if(!res)
  {
      //logging(4,"Failed to connect");
      //while(1);
      digitalWrite(LED3,HIGH);
      delay(10000);
      digitalWrite(LED3,LOW);
      ESP.restart();
  } 

  display.println(F("WiFi connected!"));
  display.display();

  ThingSpeak.begin(client);

  configTime(0, 0, ntpServer);
  Serial.print("TIME: ");Serial.println(getTime());

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/log.csv");
  if(!file) {
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/log.csv", "Epoch Time; data0; data1; data2; data3; data4; data5; \r\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();


}

void loop() 
{
  int i;
  // Connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    //logging(1,"Connecting to SSID");
    //logging(1,SECRET_SSID);

  
    
      while(WiFi.status() != WL_CONNECTED) 
      {
        WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
        Serial.print(".");
        delay(5000);
        error_cnt++;
        if (error_cnt > 20)
        {
          error_cnt = 0;
          digitalWrite(LED3,HIGH);
          delay(10000);
          digitalWrite(LED3,LOW);
          Serial.println("ERROR: Restart ESP.....");
          ESP.restart();
        }
      }
      Serial.println("\nConnected.");
    //logging(1,"Connected.");
    
  }


  if(radio.available())
  {
    char text[33] = {0}; 
    radio.read(&text, sizeof(text)-1);
    Serial.print(text);Serial.println(sizeof(text));
    string_text = String(text);
    //Serial.print(text);Serial.println(string_text.length());
    Serial.println("======Data received======");
    for(int i=0;i<string_text.length(); i++)
    {
      if(string_text.charAt(i)==';')
      {
        Lsub[Lind++]=i;
      }
    }
    data0 = string_text.substring(Lsub[0],Lsub[1]);
    data1 = string_text.substring(Lsub[1]+1,Lsub[2]);
    data2 = string_text.substring(Lsub[2]+1,Lsub[3]);
    data3 = string_text.substring(Lsub[3]+1,Lsub[4]);
    data4 = string_text.substring(Lsub[4]+1,Lsub[5]);
    data5 = string_text.substring(Lsub[5]+1,Lsub[6]);
    
    Lind=1;
    string_text="";
    Serial.println (data0);   // beeid
    Serial.println (data1);   // cnt
    Serial.println (data2);   // bat
    Serial.println (data3);   // weight
    Serial.println (data4);   // temp
    Serial.println (data5);   // spare
    Serial.println("==============================");

    // Serial.print("data0.toInt ");Serial.println(data0.toInt());
    for(i=0;i<=7;i++)
    {
    //  Serial.print("i..");Serial.println(i);
      if (bufferlocation[i]==data0.toInt()) break;
    }
    //Serial.print("bufferindex ");Serial.println(i);

    NRFreceive[i]._sbeeid = data0;
    NRFreceive[i]._scount = data1;
    NRFreceive[i]._sbat = data2;
    NRFreceive[i]._sweight = data3;
    NRFreceive[i]._stemperature = data4;

    // print the full dataarray
    for(i=0;i<=7;i++)
    {
      Serial.print(i);Serial.print(";");
      Serial.print(NRFreceive[i]._sbeeid);Serial.print(";");
      Serial.print(NRFreceive[i]._scount);Serial.print(";");
      Serial.print(NRFreceive[i]._sbat);Serial.print(";");
      Serial.print(NRFreceive[i]._sweight);Serial.print(";");
      Serial.println(NRFreceive[i]._stemperature);
    }


    if(data0.toInt() == bufferlocation[0])
    {
      display.clearDisplay();
      display.setTextSize(2);             // Normal 1:1 pixel scale
      display.setTextColor(WHITE);        // Draw white text
      display.setCursor(0,0);             // Start at top-left corner
      display.print(F("Stock "));
      display.println(NRFreceive[0]._sbeeid);
      
      display.setCursor(0,20);  
      display.print(F(" ")); display.print(NRFreceive[0]._sweight);display.println(F(" kg"));
      display.setCursor(0,38); 
      display.print(F(" ")); display.print(NRFreceive[0]._stemperature);display.println(F(" C"));
      display.setTextSize(1); 
      display.setCursor(0,57); 
      display.print(NRFreceive[0]._scount);display.print(F(" Bat ")); display.print(NRFreceive[0]._sbat);display.print(F("V"));


      // ThingSpeak.setField(1, data0.toFloat());
      // ThingSpeak.setField(2, data1.toFloat());
      // ThingSpeak.setField(3, data2.toFloat());
      // ThingSpeak.setField(4, data3.toFloat());
      // ThingSpeak.setField(5, data4.toFloat());
      // ThingSpeak.setField(6, WiFi.RSSI());

    
      // int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      // // Write value to Field 1 of a ThingSpeak Channel
      // //int httpCode = ThingSpeak.writeField(myChannelNumber, 1, rssi, myWriteAPIKey);
      // if (httpCode == 200) 
      // {
      //   Serial.println("Channel write successful.");
      //   display.print(F("   OK"));
      // }
      // else {
      //   Serial.println("Problem writing to channel. HTTP error code " + String(httpCode));
      //   display.print(F("  Err"));
      // }

      display.display();

      //Concatenate all info separated by commas
      SD_Message = String(getTime()) + ";" + NRFreceive[0]._sbeeid + ";" + NRFreceive[0]._scount + ";" + NRFreceive[0]._sweight + ";"+NRFreceive[0]._stemperature+";"+ NRFreceive[0]._sbat + "\r\n";
      
      Serial.print("Saving data: ");
      Serial.println(SD_Message);

      //Append the data to file
      appendFile(SD, "/log.csv", SD_Message.c_str());

    }
  }
}


void Init_SD(void)
{
  if(!SD.begin(17)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

}




// Variable to save current epoch time
unsigned long epochTime; 

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}


// Write to the SD card
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}