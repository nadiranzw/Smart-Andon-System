#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

SoftwareSerial nodeMCU(D1,D2);

const char* ssid = "Ssstttt";
const char* password = "buatapa?";
const char* mqtt_server = "ee.unsoed.ac.id";


const char* host = "script.google.com";
const int httpsPort = 443;  //penghubung client ke internet dengan fitur keamanan
String Gas_ID = "AKfycbws-EOxneO-5GGILysvY8fjiXPMwaJz7WZLEpwFSGinfzyX_kbh-UiMtd9D-_RcfJh_xA";
WiFiClientSecure secure_client, sclient;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

String ip;
String chipid;
String dataIn = "";     
char input;        // data abstrak

String c;          // data counter
String Status;     // data status station 
String Durasi[3];  // data durasi

int8_t indexOfA, indexOfB, indexOfC, indexOfD, indexOfE;


//----------------------Alur data----------------------
// pecah data
void ParsetheData(){
  indexOfA = dataIn.indexOf('@');
  indexOfB = dataIn.indexOf('#');
  indexOfC = dataIn.indexOf('$');
  indexOfD = dataIn.indexOf('^');
  indexOfE = dataIn.indexOf('*');

  c = dataIn.substring(0, indexOfA);
  Status = dataIn.substring(indexOfA+1, indexOfB);
  Durasi[0] = dataIn.substring(indexOfB+1, indexOfC);
  Durasi[1] = dataIn.substring(indexOfC+1, indexOfD);
  Durasi[2] = dataIn.substring(indexOfD+1, indexOfE);
}

void recieveData(){
  while (nodeMCU.available()>0){
    input = nodeMCU.read();
    if(input=='\n'){
      break;
    }
    else {
      dataIn+=input;
      ParsetheData();
    }
  }
  Serial.println("Product : " + c);
  Serial.println("Status  : " + Status);
  Serial.println("Respon  : " + Durasi[0]);
  Serial.println("Total   : " + Durasi[1]);
  Serial.println("Handling: " + Durasi[2]);
  Serial.println("");

  snprintf(msg, MSG_BUFFER_SIZE, String(c).c_str());
  client.publish("wizurai/KP/Andon/product", msg);
  snprintf(msg, MSG_BUFFER_SIZE, String(Status).c_str());
  client.publish("wizurai/KP/Andon/Status", msg);
  snprintf(msg, MSG_BUFFER_SIZE, String(Durasi[0]).c_str());
  client.publish("wizurai/KP/Andon/respon", msg);
  snprintf(msg, MSG_BUFFER_SIZE, String(Durasi[1]).c_str());
  client.publish("wizurai/KP/Andon/total", msg);
  snprintf(msg, MSG_BUFFER_SIZE, String(Durasi[2]).c_str());
  client.publish("wizurai/KP/Andon/penanganan", msg);
  delay(500);
  dataIn = "";
}


void spreadsheet(){
  Serial.println("========");
  Serial.print("Connecting to ");
  Serial.println(host);

  //----------------------------------------------Connect to Google host
  if (!sclient.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  //------------------------------------Processing data and sending data

  String url = "/macros/s/" + Gas_ID;
  url += "/exec?product=" + c;
  url += "&status=" + Status;
  url += "&response=" + Durasi[0];
  url += "&handling=" + Durasi[2];
  url += "&total=" + Durasi[1];
  
  Serial.print("requesting URL: ");
  Serial.println(url);

  sclient.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: BuildFailureDetectorESP8266\r\n" + "Connection: close\r\n\r\n");
  
  Serial.println("request sent");
  //----------------------------------------

  //----------------------------------------Checking whether the data was sent successfully or not
  while (sclient.connected()) {
    String line = sclient.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = sclient.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
}

void connectWifi(){
  delay(100);
  // We start by connecting to a WiFi network
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  
  Serial.println("");
}

//------------------MQTT Configuration------------------

// Dipanggil ketika mengirim/menerima data mqtt
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  if ((char)payload[0] == '1') {
    digitalWrite(LED_BUILTIN, HIGH);
    //snprintf (msg, MSG_BUFFER_SIZE, "%d",KondisiLED);
  } 
  else {
    digitalWrite(LED_BUILTIN, LOW);
    //snprintf (msg, MSG_BUFFER_SIZE, "%d",KondisiLED);
  }
}

// Memastikan tetap terhubung dengan server mqtt
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("wizurai/KP/Andon/outTopic", "hello world");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  nodeMCU.begin(115200);
  connectWifi();
  secure_client.setInsecure();
  sclient.setInsecure();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {  
  recieveData();
  spreadsheet();
  if (!client.connected()){
    reconnect();
  }
  client.loop();
  unsigned long now = millis();
  if (now - lastMsg > 2000){
    lastMsg = now;

    ip = String(WiFi.localIP().toString());
    snprintf(msg, MSG_BUFFER_SIZE, ip.c_str());
    client.publish("wizurai/KP/Andon/IPAddress", msg);

    chipid = String(ESP.getFlashChipId());
    snprintf(msg, MSG_BUFFER_SIZE, chipid.c_str());
    client.publish("wizurai/KP/Andon/chipid", msg);
  }
}
