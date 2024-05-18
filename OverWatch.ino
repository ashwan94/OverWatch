#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include "MQTTManager.h"

void setMQTT();

// WiFi 접속을 위한 세팅
const char* ssid = "next1";            // WiFi 이름
const char* password = "next18850";  // WiFi 비밀번호

// UTC data 받기 위한 세팅
char curTime[20];
String utcDate = "";  // UTC 의 date 을 yyyyMMdd 형식으로 저장
String utcTime = "";  // UTC 의 time 을 hhmmss 형식으로 저장

// Open API 의 data 받기 위한 세팅
const String weatherUrl = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getUltraSrtFcst";                                            // 기상청 API URL
const String fineDustUrl = "http://apis.data.go.kr/B552584/ArpltnInforInqireSvc/getCtprvnRltmMesureDnsty";                                       // 미세먼지 API URL
const String serviceKey = "?serviceKey=7v4ZOVQdsbFUNxnpjLAUHkPce9%2FWTWqr%2Faojc0K%2BakxRdGj7xcTFN%2BUwNNVwdr1Vpx0J3L7yWDWncMEZbP%2FjCQ%3D%3D";  // 왜인지 서비스키는 동일함


// ----------------- API common values --------------------
const String pageNo = "&pageNo=1";  // page 번호
String base_date = "";              // 요청 일자(최근 3일 이내만 가능)
String base_time = "";              // 요청 시간
String line = "";                   // API data 받을 변수

// ----------------- Weather API -------------------- 초단기예보 조회
const String dataType = "&dataType=JSON";           // 요청 data type
const String weatherNumOfRows = "&numOfRows=1000";  // 1 page 당 표시할 data 개수
const String nx = "&nx=67";                         // x 좌표, 경도, API 문서에서 지역에 대한 위치 정보 확인 가능
const String ny = "&ny=100";                        // y 좌표, 위도

// ---------------- FineDust API -------------------- 시도별 실시간 측정정보 조회
const String returnType = "&returnType=json";
const String fineDustNumOfRows = "&numOfRows=100";       // 1 page 당 표시할 data 개수
const String sidoName = "&sidoName=%EB%8C%80%EC%A0%84";  // 한글 인식 오류 떄문에 '대전' 을 인코딩함
const String ver = "&ver=1.0";



// MQTT 전송을 위한 Serialization
// Call by value 로 담은 parsing data
StaticJsonDocument<200> workingMemory;
JsonObject workJson = workingMemory.to<JsonObject>();  // 작동 로그 저장할 JSON
String workingLog = "";                                // serializeJson(workJson, workingLog) 를 위한 변수
String weatherData = "";                               // serializeJson(jo, weatherData) 를 위한 변수
String fineDustData = "";                              // serializeJson(jo, fineDustData) 를 위한 변수
// MQTT 전송은 API 호출 시 JsonArray 로 처리할 예정

// 시분할 처리
long currentMillis = 0;          // 현재 시간
long previousMillis = 0;         // 이전 시간
const long interval = 1 * 1000;  // 1초 간격



void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.println("WiFI 연결 요청");

  // WiFi 연결될때까지 시도
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // WiFi 연결 완료
  Serial.println("Connected to the WiFi network");

  // 현재 시간 가져오기
  configTime(9 * 3600, 0, "pool.ntp.org", "time.nist.gov");  // UTC + 9 = Korea Time
  while (!time(nullptr)) delay(500);

  Serial.println("시스템 부팅 완료!");

  // MQTT 세팅
  setupMQTT();
}

// UTC data 가져오기
void get_RealTime() {
  // 이전에 저장된 data 초기화
  utcDate = "";
  utcTime = "";
  time_t now = time(nullptr);
  struct tm* t;
  t = localtime(&now);

  sprintf(curTime, "%04d%02d%02d%02d%02d%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  for (int i = 0; i < strlen(curTime); i++) {  // char 로 선언됐다가 sprintf 가변인자로 값이 들어간 배열의 길이는 strlen 으로 구한다
    if (i < 8) {
      utcDate += curTime[i];
    } else {
      utcTime += curTime[i];
    }
  }
}

// 기상청 날씨 API 가져오기
void get_weather() {
  if ((WiFi.status() == WL_CONNECTED)) {  //Check the current connection status
    Serial.println("Requesting to Weather API data...");

    HTTPClient http;
    WiFiClient client;

    http.begin(client, weatherUrl + serviceKey + pageNo + weatherNumOfRows + dataType + base_date + base_time + nx + ny);  // 상세 URL 주소 입력 요망
    int httpCode = http.GET();                                                                                             // GET 방식으로 요청
    Serial.print("weather HTTP 응답 결과 : ");
    Serial.println(httpCode);
    workJson["weatherHttpResp"] = httpCode;
    workJson["weatherLocation"]["nx"] = nx.substring(4, 6);  // weather data 의 location 정보 저장
    workJson["weatherLocation"]["ny"] = ny.substring(4, 7);

    DynamicJsonDocument apiData(13000);          // 초단기예보 전체 API Data 가져올 동적 JSON 선언(초단기예보 API 는 12288 bytes 이므로 여유롭게 13000으로 크기 지정)
    StaticJsonDocument<200> stkMemory;           // MQTT 에 publish 할 필터링된 API Data 를 담을 정적 JSON 선언
    JsonObject jo = stkMemory.to<JsonObject>();  // 정적 JSON 의 type 을 JsonObject 로 변환

    if (httpCode > 0) {  // Http 응답이 있을 경우에만 작동
      const String json = http.getString();
      DeserializationError error = deserializeJson(apiData, json);

      if (!error) {
        const JsonArray item = apiData["response"]["body"]["items"]["item"];
        // Serial.println("조회된 item");
        // for(String j : item){
        //   Serial.println(j);
        // }
        StaticJsonDocument<200> parseData;
        JsonArray ja = parseData.to<JsonArray>();

        for (int i = 0; i < item.size(); i++) {
          if (i == 5 || i % 6 == 5 && i <= 35) {
            // base_date 로부터 5시간 30분 이후의 값만 필터링
            // 초단기예보는 기준시간으로부터 6시간 data 만 갖고 있음
            // 단기예보는 초단기예보로부터 모레까지 data 를 갖고 있음
            // 아침에 저녁 일기예보를 조회하려면 초단기예보가 아니라 단기예보 data 가 무조건 필요

            // API 로 받은 JSON 필터링된 data 들
            // jo["base_date"] = item[i]["baseDate"].as<String>();   // data 요청 날짜
            // jo["base_time"] = item[i]["baseTime"].as<String>();   // data 요청 시간
            jo["category"] = item[i]["category"].as<String>();    // 카테고리
            jo["fcstValue"] = item[i]["fcstValue"].as<String>();  // 카테고리별 상태 값
            // jo["fcst_date"] = item[i]["fcstDate"].as<String>();   // 예보 날짜
            // jo["fcst_time"] = item[i]["fcstTime"].as<String>();   // 예보 시간
            // jo["nx"] = item[i]["nx"].as<String>();                // 경도
            // jo["ny"] = item[i]["ny"].as<String>();                // 위도
            ja.add(jo);
          }
        }
        StaticJsonDocument<200> sendData;
        JsonObject j = sendData.to<JsonObject>();
        j["weather"] = ja;

        // Serial.println("필터링된 날씨 데이터");
        // for (String data : ja) {
        //   Serial.println(data);
        // }

        // 필터링 끝난 API data 를 String 에 Call by value
        // Array, JsonArray 는 얕은 복사이기 때문에 원본 값의 변화에 직접적으로 영향을 받는다
        serializeJson(j, weatherData);  // JsonObject 에 담긴 data 를 직렬화해서 String 인 weatherData 에 담음(깊은 복사)

        stkMemory.clear();
        apiData.clear();
        parseData.clear();
        sendData.clear();

        // ---------- MQTT 로 전송할 기상청 초단기예보 Data 수집 끝 ------------

      } else {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        get_weather();  // 성공할 때까지 재시도
      }

    } else {
      // API GET 에러 발생 시 될떄까지 재시도
      Serial.println("Error on HTTP request to Weather API Data");
      Serial.println("Server 에 재요청을 시도합니다.");
      get_weather();
    }
    http.end();  // http 자원 정리
  }
}

// 미세먼지 API data 가져오기
void get_fineDust() {
  if ((WiFi.status() == WL_CONNECTED)) {
    Serial.println("Requesting to FineDust API data...");

    HTTPClient http;
    WiFiClient client;

    http.begin(client, fineDustUrl + serviceKey + returnType + fineDustNumOfRows + pageNo + sidoName + ver);  // 상세 URL 주소 입력 요망
    http.addHeader("Content-Type", "application/json");                                                       // HTTP GET header 정보 추가

    int httpCode = http.GET();  // GET 방식으로 요청
    Serial.print("fineDust HTTP 응답 결과 : ");
    Serial.println(httpCode);
    workJson["fineDustHttpResp"] = httpCode;

    DynamicJsonDocument apiData(6500);           // API data 크기가 약 6,200 이므로 여유롭게 6,500 으로 설정
    StaticJsonDocument<200> stkMemory;           // MQTT 에 publish 할 필터링된 API Data 를 담을 정적 JSON 선언
    JsonObject jo = stkMemory.to<JsonObject>();  // 정적 JSON 의 type 을 JsonObject 로 변환

    if (httpCode > 0) {  // Http 응답이 있을 경우에만 작동
      const String json = http.getString();

      DeserializationError error = deserializeJson(apiData, json);

      if (!error) {
        const JsonArray items = apiData["response"]["body"]["items"];
        jo["sidoName"] = items[11]["sidoName"].as<String>();
        jo["stationName"] = items[11]["stationName"].as<String>();
        jo["dataTime"] = items[11]["dataTime"].as<String>();
        jo["khaiGrade"] = items[11]["khaiGrade"].as<String>();
        jo["pm10Grade"] = items[11]["pm10Grade"].as<String>();
        jo["pm25Grade"] = items[11]["pm25Grade"].as<String>();

        serializeJson(jo, fineDustData);  // JsonObject 에 담긴 data 를 직렬화해서 String 인 fineDustData 에 담음(깊은 복사)

        stkMemory.clear();
        apiData.clear();

        // ---------- MQTT 로 전송할 에어코리아 미세먼지 Data 수집 끝 ------------

      } else {
        // JSON Parsing 성공할때까지 재시도
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        get_fineDust();
      }
    } else {
      // API GET 에러 발생 시 될떄까지 재시도
      Serial.println("Error on HTTP request to FineDust API Data");
      Serial.println("Server 에 재요청을 시도합니다.");
      get_fineDust();
    }
    http.end();  // http 자원 정리
  }
}

void showLED() {
  // Serial.println("LED 에 출력할 data");
  // Serial.println("수신된 날짜 data : " + utcDate);
  // Serial.println("수신된 시간 data : " + utcTime);
  // Serial.println("수신된 날씨 data : " + weatherData);
  // Serial.println("수신된 미세먼지 data : " + fineDustData);
}

void loop() {
  currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {

    get_RealTime();  // UTC 생성

    // 최초 실행 시 자동으로 API 에 요청할 date, time 초기화
    if (base_date == "" && base_time == "") {
      base_date = String("&base_date=") + utcDate;
      base_time = String("&base_time=") + utcTime;
      Serial.print("API 요청 날짜가 초기화되었습니다. -> ");
      Serial.println(base_date);
      Serial.print("API 요청 시간이 초기화되었습니다. -> ");
      Serial.println(base_time);
    }

    // 1일마다 API 에 요청하는 date 정보 갱신
    if (base_date.substring(11, 19) != utcDate) {

      base_date = String("&base_date=") + utcDate;
      base_time = String("&base_time=") + utcTime;
      Serial.print("API 요청 날짜가 업데이트되었습니다. -> ");
      Serial.println(base_date);
      workJson["apiReqDate"] = base_date.substring(11, 19);
    }

    // TODO resultMsg 가 NO_DATA 일 경우 API 재요청
    String hour = utcTime.substring(0, 2);    // UTC 시
    String minute = utcTime.substring(2, 4);  // UTC 분
    String second = utcTime.substring(4, 6);  // UTC 초

    // API 로 요청할 time 도 재설정해준다
    // 기상청에서 제공하는 API 는 매시간 갱신되지만, 1시간마다 다이나믹하게 날씨가 변하진 않으므로 6시간 기준으로 날씨 data GET
    // API 갱신이 매시 30분 이후이므로 넉넉하게 1분 텀을 줘서 성공적으로 data 를 받을 수 있도록 조치함
    // 00시가 됐을때 올바른 data 를 요청하려면 00:31 이상은 되어야 한다. 만약 그 전에 data 를 요청하게 되면 변경된 date 에 대해 요청하게 되므로 NO_DATA 를 얻게 된다
    // 00:31 에 시도했더니 null 값으로 return 된다. 00:33 으로 하니 해결됨
    // + 미세먼지는 1일 기준이므로 기상청 API 를 요청할때 1번만 data 를 가져오도록 함
    if (second == "30"
        && minute == "31") {  // 00, 05, 10, 15, 20시에만 API 호출
        //  && (hour == "00" || hour == "05" || hour == "10" || hour == "15" || hour == "20") // 테스트 후 갖다붙이기
      base_time = String("&base_time=") + hour + minute;
      Serial.println("기상청, 에어코리아 API 요청 -> " + hour + "시 " + minute + "분 " + second + "초");
      workJson["apiReqTime"] = hour + minute + second;

      // MQTT publish 요청
      reconnect();

      // Open API 요청
      get_weather();
      get_fineDust();
      showLED(); // 실시간, 기상/미세먼지 정보 전달
      serializeJson(workJson, workingLog);

      // MQTT publish 를 위한 JSON Array 생성 및 data 저장
      StaticJsonDocument<200> workingLogJson;    // workingLog 를 역직렬화
      StaticJsonDocument<200> weatherDataJson;   // weatherData 를 역직렬화
      StaticJsonDocument<200> fineDustDataJson;  // fineDust 를 역직렬화
      StaticJsonDocument<200> mqttJson;          // MQTT 에 전송할 JSON
      JsonObject wlJson = workingLogJson.to<JsonObject>();
      JsonObject wdJson = weatherDataJson.to<JsonObject>();
      JsonObject fdJson = fineDustDataJson.to<JsonObject>();
      JsonArray mqttData = mqttJson.to<JsonArray>();

      deserializeJson(wlJson, workingLog);
      deserializeJson(wdJson, weatherData);
      deserializeJson(fdJson, fineDustData);
      mqttData.add(wlJson);
      mqttData.add(wdJson);
      mqttData.add(fdJson);

      // Serial.println("조회된 JSON Data");
      // Serial.println(mqttData);

      // mqttData 내용 확인하기
      // key 의 개수만큼 for문이 작동한다
      // for (String data : mqttData) {
      //   Serial.println(data);
      // }

      // MQTT server 전송을 위한 Serialization
      char output[512];
      serializeJson(mqttData, output);
      // Serial.println("직렬화 결과값");
      // Serial.println(output);
      // Serial.println(strlen(output));  // 직렬화된 data 크기 측정

      // MQTT 로 모든 data 전송
      if (WiFi.status() == WL_CONNECTED && pubClient.connected()) {
        String sensorID = "OverWatch";
        String data = String("{\"sensorID\":\"" + sensorID + "\",\"datas\":" + output + "}");
        String rootTopic = "/IoT/Sensor/2ndClass/" + sensorID;
        Serial.print("Publishing data size : ");
        // Serial.println(data);
        Serial.println(data.length());  // MQTT 로 publish 하고자 하는 data 크기
        // PubSubClient 의 최대 크기는 256 byte 가 기본으로 설정됨

        publish(rootTopic, data);

        // 자원 정리
        wlJson.clear();
        wdJson.clear();
        fdJson.clear();
        mqttData.clear();
      }



      // TODO
      // 1. LED Maxtix 에 data 전달해서 LED 로 표현하기
      // 2. 배터리 설치
    }
    previousMillis = currentMillis;

    // showLED();

    // n회차부터 받을 작동 로그, API data 받기 위한 초기화
    workingLog = "";
    weatherData = "";
    fineDustData = "";
  }
}