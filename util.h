#ifndef UTIL
#define UTIL

bool isValidate(String data);
void swap(String* a, String* b);
// WiFiMANAGER.h 에서 WiFi, RSSI 두 개의 data 가 "", null, empty 인지 검사
// 정상적이면 true, 아니면 false return
bool isValidate(String data) {
  return(data != NULL)
  && (!data.isEmpty())
  && (data != " ")
  && (data != "")
  && (data != "0")
  ;
}

// 전치 전도 함수
void swap(String* a, String* b) {
  String temp = *a;
  *a = *b;
  *b = temp;
}

#endif