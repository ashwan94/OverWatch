#ifndef MQTT_MANAGER
#define MQTT_MANAGER

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//브로커 주소
const char* mqtt_server = "nextit.or.kr";
//포트번호
const int mqtt_port = 21883;
//메세지 버퍼 크기
const int MSG_BUFFER_SIZE = 200;

WiFiClient wifiClient; //MQTT가 wifi를 사용할 수 있게 해주는거
PubSubClient pubClient(wifiClient); //MQTT가 바로 이놈

//MQTT 설정
void setupMQTT();
void receivedMQTTCallback(char* topic, byte* payload, unsigned int length);



void setupMQTT()
{
  pubClient.setServer(mqtt_server, mqtt_port);
  pubClient.setCallback(receivedMQTTCallback);
}

//자동 재연결 기능
void reconnect() 
{
  // Loop until we're reconnected
  while (!pubClient.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "AnSeungHwan";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (pubClient.connect(clientId.c_str()))
     {
      Serial.println("connected");
      // Once connected, publish an announcement...
      pubClient.publish("OverWatchPub", "It's forecast");
      // ... and resubscribe
      pubClient.subscribe("OverWatchSub");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(pubClient.state());
      Serial.println(" try again in 3 seconds");
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
}

//여기서 메세지 쏘기
void publish(String topicStr, String payloadStr)
{
  Serial.println("MQTT 로 넘어옴");
  Serial.println(topicStr);
  Serial.println(payloadStr);

  int topicLength = topicStr.length() + 1;
  int payloadLength = payloadStr.length() + 1;

  char *topic = new char[topicLength + 2];
  char *payload = new char[payloadLength + 2];

  Serial.println("뭐가 변환된 값일까?");
  Serial.println(topic);
  Serial.println(*topic);
  Serial.println(payload);
  Serial.println(*payload);
  
  topicStr.toCharArray(topic, topicLength);
  payloadStr.toCharArray(payload, payloadLength);

  pubClient.loop(); //서버한테 나 드간다?라고 말하기
  pubClient.publish(topic, payload); //서버에 쏘기

  String log = String("publish topic :" + topicStr + " msg : " + payloadStr);
  Serial.println(log);

  delete topic;
  delete payload;
}

//서버로부터 메세지 수신, 원격제어 가능
void receivedMQTTCallback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // TO-DO
}


#endif