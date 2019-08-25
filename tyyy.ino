#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Adafruit_VC0706.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>   
#include <ArduinoJson.h>
const int resetConfigPin = 3;

#define min(a,b) ((a)<(b)?(a):(b))

char ssid[] = "12121212";    
char password[] = "Ozone1897"; 

const char* host = "https://api.telegram.org";
const int httpsPort = 443;

const char* token = "651488659:AAHy0lnKesu-WbNiHsoZw14Y-2eCUsiJPKs";
const char* chat_id = "587375626";

const char* boundry = "<delimitador_conteudo>";



#define BOTtoken "700671012:AAE-b7GgDMBDSSisQCAJCoT7h6bVp45casM" 
SoftwareSerial cameraconnection = SoftwareSerial(4, 5);
Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);
File myFile;
File imgFile;
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
static bool hasSD = false;
byte getNextByte();
bool isMoreDataAvailable();

int Bot_mtbs = 1000; 
long Bot_lasttime;  
bool Start = false;

void receiveDataFromTelegram()
{
    // Espera por tempo de resposta do Servidor
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Response From Telegram!");
        client.stop();
        return;
      }
    }    
      
    Serial.println();
    Serial.println("Receiving from telegram...");        

    int responseContentLength = 0;
    while (client.available()) {

      String line = client.readStringUntil('\r');
      client.read(); // lê o caracter '\n'

      Serial.println(line);

      if (line.startsWith("Content-Length:")) {
        int index = line.indexOf(':');
        responseContentLength = line.substring(index + 1).toInt();
      }
      
      if (line.length() == 0)
        break;
    }
    
    while (responseContentLength > 0)
    {
      char ch = client.read();      
      Serial.print(ch);
      responseContentLength--;
    }  

    Serial.println();
    Serial.println("closing connection");    
}


void handleNewMessages(int numNewMessages) {
   Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));
  

  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    

    if (text == "/send_test_action") {
        bot.sendChatAction(chat_id, "typing");
        delay(4000);
        bot.sendMessage(chat_id, "Did you see the action message?");

    }

    if (text == "/start") {
      String welcome = "Koneksi Berhasil..";
      welcome += "\n\n";
      welcome += "/foto : untuk ambil gambar.\n";
      bot.sendMessage(chat_id, welcome);
    }
    if (text == "/foto") {
      bot.sendChatAction(chat_id, "typing");
      Serial.println("Ngambil Gambar");
      jepret();
      Serial.println("Selesai");
      myFile = SD.open("IMAGE00.jpg");
      if (myFile) {
        Serial.print("IMAGE00.jpg");
        Serial.print("....");
        String sent = bot.sendPhotoByBinary(chat_id, "image/jpg", myFile.size(),
            isMoreDataAvailable,
            getNextByte)
            ;
        if (sent){ Serial.println("Terkirim"); }
        else {
          Serial.println("was not sent");
        }
        myFile.close();
      }
      else {
        // if the file didn't open, print an error:
        Serial.println("error opening photo");
      }
    }
  }
}


void setup() {
  Serial.begin(38400);
  
 pinMode(SS, OUTPUT);
  if (!SD.begin(SS)) {
   Serial.println("Card failed, or not present");
    return;
  }
  else {
    hasSD = true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (cam.begin()) {
   Serial.println("Kamera terdeteksi");
  }
  else {
   Serial.println("Error");
    return;
  }
  char *reply = cam.getVersion();
  if (reply == 0) {
   Serial.print("Gagal Versi");
  }
}

void jepret() {
  Serial.println("Snap in 3 secs...");
  delay(3000);
 if (! cam.takePicture()) 
    Serial.println("Failed to snap!");
  else 
    Serial.println("Picture taken!");
  
  // Create an image with the name IMAGExx.JPG
  char filename[13];
  strcpy(filename, "IMAGE00.JPG");
  for (int i = 0; i < 100; i++) {
    filename[5] = '0' + i/10;
    filename[6] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }
  
  // Open the file for writing
  File imgFile = SD.open(filename, FILE_WRITE);

  // Get the size of the image (frame) taken  
  uint16_t jpglen = cam.frameLength();
  Serial.print("Storing ");
  Serial.print(jpglen, DEC);
  Serial.print(" byte image.");

  int32_t time = millis();
  pinMode(15, OUTPUT);
  // Read all the data up to # bytes!
  byte wCount = 0; // For counting # of writes
  while (jpglen > 0) {
    // read 32 bytes at a time;
    uint8_t *buffer;
    uint8_t bytesToRead;
    if (jpglen < 32)
        bytesToRead = jpglen;
      else
        bytesToRead = 32; // change 32 to 64 for a speedup but may not work with all setups!
    buffer = cam.readPicture(bytesToRead);
    imgFile.write(buffer, bytesToRead);
    if(++wCount >= 64) { // Every 2K, give a little feedback so it doesn't appear locked up
      Serial.print('.');
      wCount = 0;
    }
    //Serial.print("Read ");  Serial.print(bytesToRead, DEC); Serial.println(" bytes");
    jpglen -= bytesToRead;
  }
  imgFile.close();
  time = millis() - time;
  Serial.println("done!");
  Serial.print(time); Serial.println(" ms elapsed");
  sendPhotoToTelegram(filename);
cam.resumeVideo();
}
void receiveDataFromTelegram()
{
    // Espera por tempo de resposta do Servidor
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Response From Telegram!");
        client.stop();
        return;
      }
    }    
      
    Serial.println();
    Serial.println("Receiving from telegram...");        

    int responseContentLength = 0;
    while (client.available()) {

      String line = client.readStringUntil('\r');
      client.read(); // lê o caracter '\n'

      Serial.println(line);

      if (line.startsWith("Content-Length:")) {
        int index = line.indexOf(':');
        responseContentLength = line.substring(index + 1).toInt();
      }
      
      if (line.length() == 0)
        break;
    }
    
    while (responseContentLength > 0)
    {
      char ch = client.read();      
      Serial.print(ch);
      responseContentLength--;
    }  

    Serial.println();
    Serial.println("closing connection");    
}
void sendPhotoToTelegram(String filename)
{
    Serial.printf("Connecting Telegram %s:%d... ", host, httpsPort);

    if (!client.connect(host, httpsPort)) {      
      Serial.println("Failde To Conenct!");      
      return;
    }
    sendDataToTelegram(filename);
    receiveDataFromTelegram();
}

void sendDataToTelegram(String file_name)
{
    String start_request = "";
    String end_request = "";

//    String chat_id = String(bot.messages[0].chat_id);
//    String chat_id_1 = String(bot.messages[0].chat_id);

    start_request = start_request + "--" + boundry + "\r\n";
    start_request = start_request + "content-disposition: form-data; name=\"chat_id\"" + "\r\n";
    start_request = start_request + "\r\n";
    start_request = start_request + chat_id +"\r\n";

    start_request = start_request + "--" + boundry + "\r\n";
    start_request = start_request + "content-disposition: form-data; name=\"photo\"; filename=\"foto.jpg\"\r\n";
    start_request = start_request + "Content-Type: image/jpeg\r\n";
    start_request = start_request + "\r\n";

    end_request = end_request + "\r\n";
    end_request = end_request + "--" + boundry + "--" + "\r\n";
   
//  String file_name = "IMAGE00.jpg";
    
    Serial.print("Sending ");
    Serial.println(file_name);

    File file = SD.open(file_name);               
    int contentLength = (int)file.size() + start_request.length() + end_request.length();    
    
    String headers = String("POST /bot") + token + "/sendPhoto HTTP/1.1\r\n";
    headers = headers + "Host: " + host + "\r\n";
    headers = headers + "User-Agent: ESP8266" + String(ESP.getChipId()) + "\r\n";
    headers = headers + "Accept: */*\r\n";
    headers = headers + "Content-Type: multipart/form-data; boundary=" + boundry + "\r\n";
    headers = headers + "Content-Length: " + contentLength + "\r\n";
    headers = headers + "\r\n";
    headers = headers + "\r\n";

    Serial.println();
    Serial.println("Mengirim data ke Telegram ...");        

    Serial.print(headers);        
    client.print(headers);
    client.flush();

    Serial.print(start_request);
    client.print(start_request);
    client.flush();

    Serial.println("sendFile");
    sendFile(&file);
    
    file.close();
    client.flush();

    Serial.print(end_request);
    client.print(end_request);
    client.flush();
}
void loop() {
  
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    Bot_lasttime = millis();
  }
}
bool isMoreDataAvailable(){
  return myFile.available();
}

byte getNextByte(){
  return myFile.read();
}
void sendFile(Stream* stream)
{
    size_t bytesReaded;
    size_t bytesSent; 
    do {
        uint8_t buff[1024];
        bytesSent = 0;
        bytesReaded = stream->readBytes(buff, sizeof(buff));
        if (bytesReaded) {
            bytesSent = client.write(buff, bytesReaded);
            client.flush();
        }        
    } while ( (bytesSent == bytesReaded) && (bytesSent > 0) );
}
