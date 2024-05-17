#include <FrequencyTimer2.h>

#define UP { \
  {0, 0, 0, 0, 0, 0, 0, 0}, \
  {0, 0, 0, 1, 1, 0, 0, 0}, \
  {0, 0, 1, 1, 1, 1, 0, 0}, \
  {0, 1, 1, 1, 1, 1, 1, 0}, \
  {1, 1, 1, 1, 1, 1, 1, 1}, \
  {1, 1, 1, 1, 1, 1, 1, 1}, \
  {0, 0, 0, 0, 0, 0, 0, 0}, \
  {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define ZERO { \
  {0, 0, 0, 1, 1, 0, 0, 0},  \
  {0, 0, 1, 0, 0, 1, 0, 0},  \
  {0, 1, 0, 0, 0, 0, 1, 0},  \
  {0, 1, 0, 0, 0, 0, 1, 0},  \
  {0, 1, 0, 0, 0, 0, 1, 0},  \
  {0, 1, 0, 0, 0, 0, 1, 0},  \
  {0, 0, 1, 0, 0, 1, 0, 0},  \
  {0, 0, 0, 1, 1, 0, 0, 0}  \
}

#define DOWN { \
  {0, 0, 0, 0, 0, 0, 0, 0}, \
  {0, 0, 0, 0, 0, 0, 0, 0}, \
  {1, 1, 1, 1, 1, 1, 1, 1}, \
  {1, 1, 1, 1, 1, 1, 1, 1}, \
  {0, 1, 1, 1, 1, 1, 1, 0}, \
  {0, 0, 1, 1, 1, 1, 0, 0}, \
  {0, 0, 0, 1, 1, 0, 0, 0}, \
  {0, 0, 0, 0, 0, 0, 0, 0}  \
}

int tact = A5;
byte col = 0;
byte leds[8][8];

int pins[17] = {-1, 5, 4, 3, 2, 14, 15, 16, 17, 13, 12, 11, 10, 9, 8, 7, 6};

int cols[8] = {pins[13], pins[3], pins[4], pins[10], pins[6], pins[11], pins[15], pins[16]};
int rows[8] = {pins[9], pins[14], pins[8], pins[12], pins[1], pins[7], pins[2], pins[5]};

const int numPatterns = 7;
byte patterns[numPatterns][8][8] = { UP, DOWN, ZERO };

int pattern = 0;


void setup() {
  for(int i = 1; i <= 16; i++) {
    pinMode(pins[i], OUTPUT);
  }

  for(int i = 0; i < 8; i++) {
    digitalWrite(cols[i], HIGH);
  }

  for(int i = 0; i < 8; i++) {
    digitalWrite(rows[i], HIGH);
  }
  
  clearLeds();

  FrequencyTimer2::setOnOverflow(display);

  pinMode(tact, INPUT);
  setPattern(pattern);

}

void loop() {
  int readTact = digitalRead(tact);

  if(readTact == HIGH){
    if(pattern == 2) pattern = 0;
    else pattern++;
    delay(300);
  }

  setPattern(pattern);

}

void clearLeds(){
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      leds[i][j] = 0;
    }
  }
}

void setPattern(int pattern){
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      leds[i][j] = patterns[pattern][i][j];
    }
  }
}

void display() {
  digitalWrite(cols[col], HIGH);
  col++;
  if(col == 8){
    col = 0;
  }
  for(int row = 0; row <= 7; row++) {
    if(leds[col][7 - row] == 1) {
      digitalWrite(rows[row], HIGH);
    }else{
      digitalWrite(rows[row], LOW);
    }
  }
  digitalWrite(cols[col], LOW);
}