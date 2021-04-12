#include <FastGPIO.h>
#include "atascii.h"

#define CLOCK     2
#define RCLK      3
#define R_ADDR0   4
#define R_ADDR1   5
#define R_ADDR2   6
#define R_LATCH   7
#define RED_DATA  8
#define GRE_DATA  9
#define CLEAR     ?

#define NUM_COL   160
#define NUM_ROW   8

#define RED       1
#define GREEN     2
#define ORANGE    3

uint16_t pattern[NUM_ROW][NUM_COL / 8]; // each column is two bits, so 8 can fit in 16-bit integer
byte current_row = 0;
int i, j, k;

void setup() {
  pinMode(CLOCK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(R_ADDR0, OUTPUT);
  pinMode(R_ADDR1, OUTPUT);
  pinMode(R_ADDR2, OUTPUT);
  pinMode(R_LATCH, OUTPUT);
  pinMode(GRE_DATA, OUTPUT);
  pinMode(RED_DATA, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);

  FastGPIO::Pin<RCLK>::setOutputLow();

  char letter;
  int column, row, char_col;
  byte cur_char_row;
//  String text = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  String text = "Hello World!! 1234567890";
  for(column = 0; column < NUM_COL / 8; column++) {
    letter = text.charAt(column);
    draw_char_at_position(letter, column, 0, RED);
  }

  OCR0A = 0x7F; // setup Compare Register A to trigger when timer = 127
  TIMSK0 |= _BV(OCIE0A); // configure timer0 with Compare A. timer0 should be setup at 1000hz for Arduino
}

void loop() {
}

void draw_char_at_position(char character, int col_start, int row_start, uint16_t color) {
  col_start = constrain(col_start, 0, NUM_COL);
  row_start = constrain(row_start, 0, NUM_ROW);
  char cur_char_row;
  int char_col;
  for(int row = row_start; row < NUM_ROW; row++) {
    cur_char_row = pgm_read_byte(&atascii_font[character][row]);
    for(char_col = 0; char_col < 8; char_col++) {
      if((1 << char_col) & cur_char_row) {
        pattern[row][col_start] = ((pattern[row][col_start] << 2) | (uint16_t) color); // this is such a bad idea
      }
      else {
        pattern[row][col_start] = ((pattern[row][col_start] << 2) & 0b1111111111111100); // clear out the two bits for this column
      }
    }
  }
}

// Paint one row of display each time the timer fires
// One row every 1ms = full update in 8ms =~ refresh rate of 120hz
SIGNAL(TIMER0_COMPA_vect) {
  for(i = 0; i < (NUM_COL / 8); i++) {
    for(j = 0; j < 8; j++) {
      if(0b0000000000000001 & (pattern[current_row][i] >> (j << 1))) {
        FastGPIO::Pin<RED_DATA>::setOutputHigh();
      }
      if(0b0000000000000010 & (pattern[current_row][i] >> (j << 1))) {
        FastGPIO::Pin<GRE_DATA>::setOutputHigh();
      }
      FastGPIO::Pin<CLOCK>::setOutputHigh();
      FastGPIO::Pin<RED_DATA>::setOutputLow();
      FastGPIO::Pin<GRE_DATA>::setOutputLow();
      FastGPIO::Pin<CLOCK>::setOutputLow();
    }
  }

  FastGPIO::Pin<R_LATCH>::setOutputLow();
  FastGPIO::Pin<RCLK>::setOutputHigh();

  if(0b00000001 & current_row) {
    FastGPIO::Pin<R_ADDR0>::setOutputHigh();
  }
  else {
    FastGPIO::Pin<R_ADDR0>::setOutputLow();
  }

  if(0b00000010 & current_row) {
    FastGPIO::Pin<R_ADDR1>::setOutputHigh();
  }
  else {
    FastGPIO::Pin<R_ADDR1>::setOutputLow();
  }

  if(0b00000100 & current_row) {
    FastGPIO::Pin<R_ADDR2>::setOutputHigh();
  }
  else {
    FastGPIO::Pin<R_ADDR2>::setOutputLow();
  }

  FastGPIO::Pin<R_LATCH>::setOutputHigh();
  FastGPIO::Pin<RCLK>::setOutputLow();

  if(++current_row == NUM_ROW) {
    current_row = 0;
  }
}
