#include "U8glib.h"
#include "Wire.h"
#include "TimerOne.h"

#define THROTTLE_PIN A7
#define BRAKE_PIN A2
#define THROTTLE_FILTER 5
#define DEFAULT_THROTTLE_ZERO 210
#define BLINK_PIN 13
#define MIN_BRAKE_VOLTAGE 170
#define MAX_BRAKE_VOLTAGE 860
#define BYTES_TO_RECEIVE 26 // from motor controller
#define ID_MOTOR_CONTROLLER 8

typedef union i2c_float_t {
  float float_value;
  byte byte_array[4];
} i2c_float;

typedef union i2c_int_t {
  int int_value;
  byte byte_array[2];
} i2c_int;

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE); // I2C / TWI 

int cnt_main = 0;
int cnt_screen_update = 0;

i2c_int cnt_motor_ctrl;
i2c_int cnt_motor_ctrl_prev;

int cnt_print_switch = 0;

char wheel[10] = "-/|\\";
int wheel_cnt = 0;
char tmp[10] = "";

i2c_float avgInputCurrent;
i2c_float ampHours;
i2c_float inpVoltage;
i2c_float rpm;
i2c_float throttle_current;
i2c_float brake_current;

// TODO: initiate i2c_float variables


void draw() {
  // graphic commands to redraw the complete screen should be placed here  
  //u8g.setFont(u8g_font_osb21);
  //u8g.drawStr( 0, 22, "VESC data");

  // box
  u8g.drawBox(0, 0, avgInputCurrent.float_value*10, 12);
  
  // row one
  u8g.setFont(u8g_font_fur14);
  //u8g.setPrintPos(0, 32);
  //u8g.print(cnt_motor_ctrl.int_value);

  switch (wheel_cnt % 2) {
    case 0:
      u8g.drawStr(0, 32, "*\0");
      break;
    case 1:
      u8g.drawStr(0, 32, " \0");
      break;
  }

  u8g.setPrintPos(60, 32);
  u8g.print(avgInputCurrent.float_value);
  //u8g.print(cnt_screen_update);

  // row two
  u8g.setFont(u8g_font_fur14);
  u8g.setPrintPos(0, 48);
  u8g.print(ampHours.float_value);

  u8g.setPrintPos(60, 48);
  u8g.print(throttle_current.float_value);

  // row three
  //u8g.setFont(u8g_font_fur14);
  u8g.setPrintPos(0, 64);
  u8g.print(inpVoltage.float_value);

  u8g.setPrintPos(60, 64);
  u8g.print(brake_current.float_value);
}

void setup(void) {
  int tmp_throttle_sum = 0;
  // assign default color value
  u8g.setColorIndex(1);

  Wire.begin();

  Serial.begin(115200);
  while(!Serial) {
    u8g.setFont(u8g_font_unifont);
    u8g.drawStr( 30, 42, "Waiting for connection");
  }
}

void loop(void) {
  cnt_main++;

  if (cnt_main > 0) {
    cnt_screen_update++;
    Wire.requestFrom(ID_MOTOR_CONTROLLER, BYTES_TO_RECEIVE); // request bytes from slave device

    cnt_motor_ctrl.byte_array[0] = Wire.read();
    cnt_motor_ctrl.byte_array[1] = Wire.read();

    rpm.byte_array[0] = Wire.read();
    rpm.byte_array[1] = Wire.read();
    rpm.byte_array[2] = Wire.read();
    rpm.byte_array[3] = Wire.read();

    avgInputCurrent.byte_array[0] = Wire.read();
    avgInputCurrent.byte_array[1] = Wire.read();
    avgInputCurrent.byte_array[2] = Wire.read();
    avgInputCurrent.byte_array[3] = Wire.read();

    ampHours.byte_array[0] = Wire.read();
    ampHours.byte_array[1] = Wire.read();
    ampHours.byte_array[2] = Wire.read();
    ampHours.byte_array[3] = Wire.read();

    inpVoltage.byte_array[0] = Wire.read();
    inpVoltage.byte_array[1] = Wire.read();
    inpVoltage.byte_array[2] = Wire.read();
    inpVoltage.byte_array[3] = Wire.read();

    throttle_current.byte_array[0] = Wire.read();
    throttle_current.byte_array[1] = Wire.read();
    throttle_current.byte_array[2] = Wire.read();
    throttle_current.byte_array[3] = Wire.read();

    brake_current.byte_array[0] = Wire.read();
    brake_current.byte_array[1] = Wire.read();
    brake_current.byte_array[2] = Wire.read();
    brake_current.byte_array[3] = Wire.read();

    cnt_main = 0;
  }

  if (cnt_motor_ctrl_prev.int_value != cnt_motor_ctrl.int_value)
  {
    cnt_motor_ctrl_prev.int_value = cnt_motor_ctrl.int_value;
    wheel_cnt++;
  }

  // picture loop
  u8g.firstPage();
  do {
    draw(); } while( u8g.nextPage() );
}

