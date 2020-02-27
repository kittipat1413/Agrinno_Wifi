#include <Wire.h>
#include "DFRobot_SHT20.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>

#define LED 13

const char* ssid     = "Avilon-IOT";
const char* password = "avilonIDC";
const char* mqtt_server = " mqtt.netpie.io";

WiFiClient espClient;
ESP8266WebServer server(80); //Server on port 80
PubSubClient client(espClient);

char buffer[80];
int timer = 0;
uint32_t uart_timer = 0;
DFRobot_SHT20    sht20;           //D1-->SCL, D2-->SDA


void handleRoot() {
  if (WiFi.status() == WL_CONNECTED){
  server.send(200, "text/plain","WiFi connected");
  }
  else{
  server.send(200, "text/plain","WiFi disconnect");  
  }
}

void WIFI_Connect()
{
  WiFi.disconnect();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
    // Wait for connection
  for (int i = 0; i < 25; i++)
  {
    if ( WiFi.status() != WL_CONNECTED ) {
      server.handleClient();
      Serial.print ( "." );
      delay ( 250 );
    }
  }
}

void netpie_connect() {
  uint8_t reset_cnt = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect netpie kittipat
    if (client.connect("6b66fb85-8aa7-4185-a5de-96648b51d4df","ig86u3waqqiThz7yeozByVA4SpSSs9Vd","p#939uPJ7^^IORm.PNXV)fCoJ@@3+QNS")) {
      reset_cnt = 0;
      Serial.println("connected");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      reset_cnt++;
         if(reset_cnt >= 100){
         ESP.reset();
        }
    }
      delay(100);
      Serial1.write("OK");
      delay(100);  
    }   
  }

void setup() {
  
    Wire.begin(5,4);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);
    delay(100);
    
    Serial.begin(115200);
    Serial.println("\nStarting...");
    // Open serial communications and wait for port to open:
    Serial1.begin(115200);
  
    WiFi.mode(WIFI_AP_STA); 
    WiFi.softAP("Agrinno_wifi_esp", "12345678");  //Start HOTspot removing password will disable security
    server.on("/", handleRoot);      //Which routine to handle at root location
    server.begin();                  //Start 
    /* Initial WIFI, this is just a basic method to configure WIFI on ESP8266.                       */
    /* You may want to use other method that is more complicated, but provide better user experience */
    Serial.println("WiFi connecting ......"); 
    if (WiFi.begin(ssid, password)) {
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
            delay(100);
            Serial1.write("OK");
        }
    }

    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

  Serial.println("netpie connecting ......");
  client.setServer(mqtt_server, 1883);
  netpie_connect();
  Serial.println("netpie connected");

  Serial.println("SHT20 Example!");
  sht20.initSHT20();                                  // Init SHT20 Sensor
  delay(100);
  sht20.checkSHT20();                                 // Check SHT20 Sensor
  delay(100);
  
  digitalWrite(LED, LOW);
  delay(100);
}

void loop() {
   server.handleClient();
   delay(100);
   
   if ((millis() > uart_timer + (2 * 1000)) || (millis() < uart_timer )) {
    uart_timer = millis();
    Serial1.write("OK");
   }
   
     if (WiFi.status() != WL_CONNECTED)
    { 
      Serial.println("\nLoss connection...");
      delay(1000);
      Serial1.write("OK");
      delay(100);
      digitalWrite(LED, HIGH);
      delay(100);
      WIFI_Connect();
    }
     else
    {
      //Serial.println("WiFi connected");  
    /* To check if the microgear is still connected */  
    if (client.connected()) {
        
        //Serial.println("Netpie connected");
        /* Call this method regularly otherwise the connection may be lost */
        client.loop();

        if (timer >= 5000) {
            float humi = sht20.readHumidity();                  // Read Humidity
            float temp = sht20.readTemperature();               // Read Temperature
            Serial.println("Writefeed..."); 
            Serial.print(" Temperature:");
            Serial.print(temp, 1);
            Serial.print("C");
            Serial.print(" Humidity:");
            Serial.print(humi, 1);
            Serial.print("%");
            Serial.println();
                    
            sprintf(buffer, "{\"data\" : { \"temp\": %.2f , \"humi\": %.2f } }",temp,humi);
            client.publish("@shadow/data/update",buffer );
            delay(100);
            
            digitalWrite(LED, HIGH);
            delay(100);
            digitalWrite(LED, LOW);
            delay(100);
            digitalWrite(LED, HIGH);
            delay(100);
            digitalWrite(LED, LOW);
            delay(100);
            digitalWrite(LED, HIGH);
            delay(100);
            digitalWrite(LED, LOW);
            
            timer = 0;
        } 
        else timer += 100;
    }
    else {
        //Serial.println("netpie connection lost...");
        if (timer >= 8000) {
            Serial.println("netpie-reconnect");
            netpie_connect();
            timer = 0;
        }
        else timer += 100;
    }
    delay(100);
    }
}
