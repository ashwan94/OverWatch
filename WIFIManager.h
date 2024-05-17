//중복 import 방지용 코드
#ifndef WIFI_MANAGER
#define WIFI_MANAGER

#include "util.h"

//자체 WEB Server 구축용
#include <ESP8266WebServer.h>
#include <EEPROM.h>

//HTML 소스 코드
#include "HTMLPage.h"

// HTML 소스 모음 클래스
HTMLPage html; //객체 생성

//최대 20개까지 스캔
//신호 강도 기준
#define WIFI_MAX_SIZE 10

//각자의 보드 WIFI 설정
const char* ap_ssid = "AN"; //표출되는 이름
const char* ap_password = "49920038"; //비번

//WEB 서버 클래스
ESP8266WebServer server(80); //server라는 객체 생성

//스캔한 wifi를 받아둘 배열 만들기
String wifiList[WIFI_MAX_SIZE];
//스캔한 wifi의 강도를 저장할 배열 만들기
String rssiList[WIFI_MAX_SIZE];

//최종 연결된 기존에 있던 wifi
//인터넷이 가능한 wifi에 들러붙어 기생
String connectedSSID = "AN";
String connectedPassword = "49920038";

int connectedSSIDAddress = 32;
int connectedPasswordAddress = 128;

//함수 선언
int scanWiFiList();
void handleRoot();
void handleForm();
String prepareSelectWifiList();

//wifi 스캔함수
//찾은 갯수를 반환
int scanWiFiList()
{
  int n = WiFi.scanNetworks(); //근처에 있는 와이파이들 갯수
  //최대 갯수만 넣기
  int k = 0; //멀쩡한 WIFI 갯수
  for(int i = 0; i < WIFI_MAX_SIZE && i < n; i++)
  {
    String s = WiFi.SSID(i);
    String r = String(WiFi.RSSI(i));

    //둘다 멀쩡해야 됨
    if(isValidate(s) && isValidate(r))
    {
      wifiList[k] = WiFi.SSID(i);
      rssiList[k] = WiFi.RSSI(i);
      k++; //검증될 때만 k값 증가
    }
  }

  //오름차순으로 정렬하기
  //정렬 알고리즘은 자율선택
  //String을 int로 변환필요 toInt();
  //int를 String           String(x)
  //-40 -50 -55 -60 -70
  //일단 전체를 돌고 돌아야함
  for(int i = 0; i < WIFI_MAX_SIZE -1 && i < n - 1; i++)
  {
    bool isSwap = false;
    for(int j = 0; j < WIFI_MAX_SIZE -1 && j < n - 1; j++)
    {
      //비교했더니 다음꺼가 더 크다면 전치
      if(rssiList[j].toInt() < rssiList[j + 1].toInt())
      {
        swap(&rssiList[j], &rssiList[j + 1]);
        swap(&wifiList[j], &wifiList[j + 1]);
        isSwap =  true;
      }
    }
    if(!isSwap)
    {
      break;
    }
  }

  return k; //검증된 wifi 갯수만 반환
}


//맨처음 접속시 보여줄 문자열(=HTML코드) 출력
void handleRoot()
{
  scanWiFiList(); //하드웨어적으로 스캔결과 보관

  //맨처음 접속시 보여줄 문자열(=HTML코드) 출력
  server.send(200, "text/html", prepareSelectWifiList());
}

//Form 데이터 받아서 처리
//지정한 ssid, pw 받아서 저장
void handleForm()
{
  String apName = server.arg("apName");
  String apPw = server.arg("apPw");

  //잘 받았나 시리얼로 확인하기
  Serial.print("apName = ");
  Serial.println(apName);
  Serial.print("apPw = ");
  Serial.println(apPw);

  //받은거 저장하기
  //이후 계속 이거로 사용
  if(isValidate(apName) && isValidate(apPw))
  {
    connectedSSID = apName;
    connectedPassword = apPw;

    EEPROM.put(connectedSSIDAddress, apName);
    EEPROM.put(connectedPasswordAddress, apPw);
    EEPROM.commit();
    server.send(200, "text/html", html.getReturn);
    Serial.println("EEPROM saved");
  }
  else
  {
    //getRetry
    server.send(200, "text/html", html.getRetry);
    Serial.println("wifi ssid not validate");
  }
  
}

//HTML에 스캔 결과를 띄워주는 함수
String prepareSelectWifiList()
{
  String strList ="<ul>";
  for(int i =0; i< WIFI_MAX_SIZE; i++){
      if(wifiList[i] != "")
      {
        strList += "<li><a name='" + wifiList[i] + "' onclick='select(this.name)'>" + wifiList[i] + "</a> </li>";
      }
  }
  strList += "</ul>";

  //위에서 만든 <li> 코드를 이제 완성된 HTML 코드로 만들어줌
  String htmlPage = html.getWIFIList(strList);
  
  return htmlPage;

}

String getEEPROM(int address)
{
  char temp[20];
  EEPROM.get(address, temp);
    
  String tempStr(temp);

  tempStr.trim();
  return tempStr;
}

String getSSIDEEPROM()
{
  return getEEPROM(connectedSSIDAddress);
}

String getPWEEPROM()
{
  return getEEPROM(connectedPasswordAddress);
}

#endif