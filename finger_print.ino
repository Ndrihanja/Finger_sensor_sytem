#include <Arduino.h>
 #include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <StreamString.h>

#define Finger_Rx 14 //D5
#define Finger_Tx 12 //D6

WebSocketsClient webSocket;

const char *ssid     = "TRIBRATA";
const char *password = "Tribrata301";
bool connected = false;

SoftwareSerial mySerial(Finger_Rx, Finger_Tx);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id =1;

uint8_t fingerStoreId = 1;

uint8_t _fingerStoreId = 0;

uint8_t count_model = 0;

char *message;

void setup()  
{
  Serial.begin(9600);
  while (!Serial);
  //delay(100);
  Serial.println("\n\nAdafruit Fingerprint sensor enrollment");
  finger.begin(57600);
  WiFi.begin(ssid, password);
 
  while ( WiFi.status() != WL_CONNECTED ) {
      delay ( 500 );
      Serial.print ( "." );
    }
  Serial.print("Local IP: "); Serial.println(WiFi.localIP());
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
            
  /**__server address, port and URL__*/
    webSocket.begin("192.168.88.248",3001, "/");
 
  /**__ event handler __**/
    webSocket.onEvent(webSocketEvent);
  //restoreFingerSensor();
}


void loop()                     
{  
     
   webSocket.loop();
   if (connected){
      //Serial.println("Websocket connected");
       int idFingerPrint =  getFingerprintID();
       if(idFingerPrint == -1){
          //tsi ao anatiny ley finger sensor ley finger
            Serial.println("Enregistrement d'emprunt");
            getFingerprintEnroll();
        }
       if(idFingerPrint == 0){
           //Tsi manao ninina
        }
       if(idFingerPrint > 0){
          //ao anatiny ley finger sensor ley finger
          Serial.println("Presence");
          String data = "presence:";
          data += idFingerPrint;
          webSocket.sendTXT(data);
        }
   }

  //restoreFingerSensor();
  
 // getFingerprintEnroll();
}

uint8_t getFingerprintEnroll() {
    
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(fingerStoreId);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(fingerStoreId);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success! /**_VERIFICATION_**/
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(fingerStoreId);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   

  finger.getTemplateCount();
  Serial.print("\nNOMBRE ENREGISTREMENTS : ");Serial.println(finger.templateCount);Serial.println("\n");
  
  //Serial.print("ID "); Serial.println(fingerStoreId);
  //p = finger.storeModel(fingerStoreId);
  
  Serial.print("ID "); Serial.println(finger.templateCount + 1);
  p = finger.storeModel(finger.templateCount + 1);

  if (p == FINGERPRINT_OK) {
    _fingerStoreId = finger.templateCount + 1;
    String response = "enregistrement:";
    response += _fingerStoreId;
    webSocket.sendTXT(response);
    //webSocket.sendTXT("enregistrement:enregistrement");
    Serial.print("\nStored with id : ");Serial.print(_fingerStoreId);Serial.println("\n");
    fingerStoreId++;
    return _fingerStoreId;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
}
/********_CHECK_FINGER_PRINT_**************/

int getFingerprintID() {
  uint8_t p = finger.getImage();
  if(p == FINGERPRINT_OK) {
    p = finger.image2Tz();
    if(p == FINGERPRINT_OK) {
      id = finger.fingerFastSearch();
      if(id == FINGERPRINT_OK) {
         id = finger.fingerID;
         Serial.print("ID : ");
         Serial.println(finger.fingerID);
      } else {
          Serial.print("Cette éléve n'existe pas encore");
          return -1;
      }
    }   
    return id;
  } else {
  return 0;
  }
}

/*****_RESTORE_FINGER_SENSOR_MEMORY_******/
void restoreFingerSensor(){
 finger.emptyDatabase(); 
}

 /********______WEBSOCKET______**********/
 void webSocketEvent( WStype_t type, uint8_t *payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            connected = false;
            // Serial.println("Websocket disconnected");
            break;
        case WStype_CONNECTED: {
            connected = true;
           // Serial.println("Websocket connected");
            webSocket.sendTXT("status:Connected");
        }
            break;
        case WStype_TEXT:
            webSocket.sendTXT("Reçus");
            Serial.println("Une nouvelle message");
            Serial.printf("Message: %s\n", payload);
            break;
        case WStype_BIN:
            hexdump(payload, length);
            break;
                case WStype_PING:
                        // pong will be send automatically
                       Serial.println("Ping");
                        break;
                case WStype_PONG:
                        // answer to a ping we send
                         Serial.println("Ping");
                        break;
    }
}
