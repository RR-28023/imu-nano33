#include <WiFiNINA.h> 
#include <PubSubClient.h>
#include <Arduino_LSM6DS3.h>
#include "credentials.h"

// Application parameters
char pubTopic[] = "sensors/imu1/values";
char subTopic[] = "sensors/imu1/mode";
const float threshold = 10.0;
const int sample_rate = 10; // In Hz

// Connectivity parameters
const char* ssid = SSID_WIFI;
const char* password = PW_WIFI;
const char* mqttServer = MQTT_SERVER;
const int mqttPort = MQTT_PORT;
const char* mqttUsername = MQTT_USRNM;
const char* mqttPassword = MQTT_PW;


WiFiSSLClient wifiClient; //SSL
PubSubClient client(wifiClient);

void setup_wifi()
//Connect to the wifi network 
{
  delay(10);  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

String mode = "ifmove";
void callback(char* topic, byte* payload, unsigned int length) {
  char payload_chr[length];
  Serial.print("Message arrived [");  
  Serial.print(topic);  
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    payload_chr[i] = (char)payload[i];
    //Serial.print((char)payload[i]);
  }
  Serial.println();
  String message = payload_chr;
  Serial.print(message);
  mode = message;
  Serial.println();
}

void reconnect() 
{
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "Nano33-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUsername, mqttPassword)) 
    {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(subTopic);
    } else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() 
{   
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);  
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  Serial.println("IMU initialized!");  
}

void publish_imu_values(float x_acc, float y_acc, float z_acc, 
                        float x_gyr, float y_gyr, float z_gyr){
  //Publish imu readings in InfluxDB Line Protocol format
  String temp_str;
  char mensaje[100];
  temp_str  = "imu x_acc=";
  temp_str += String(x_acc);
  temp_str += ",y_acc=";
  temp_str += String(y_acc);
  temp_str += ",z_acc=";
  temp_str += String(z_acc);
  temp_str += ",x_gyr=";
  temp_str += String(x_gyr);
  temp_str += ",y_gyr=";
  temp_str += String(y_gyr);
  temp_str += ",z_gyr=";
  temp_str += String(z_gyr);        
  temp_str.toCharArray(mensaje, temp_str.length() + 1);  
  client.publish(pubTopic, mensaje);      
}

int count = 0;
void loop() 
{  
  if (!client.connected()) 
  {
    reconnect();    
  }
  float x_acc, y_acc, z_acc;
  float x_gyr, y_gyr, z_gyr;  
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      IMU.readAcceleration(x_acc, y_acc, z_acc);
      IMU.readGyroscope(x_gyr, y_gyr, z_gyr);
      if (mode == "alltime") {  
        publish_imu_values(x_acc, y_acc, z_acc, x_gyr, y_gyr, z_gyr);    
      }
      if (mode == "ifmove") {
        bool check;
        check = (abs(x_gyr) > threshold || abs(y_gyr) > threshold || abs(z_gyr) > threshold);
        if(check || count > 0){
        publish_imu_values(x_acc, y_acc, z_acc, x_gyr, y_gyr, z_gyr);
        count += 1;
          // This ensures readings for 1 sec after going below the threshold:
          if (count > sample_rate) {count = 0;} 
        }        
      }
    }
  delay(1000/sample_rate);
  client.loop();  
}
