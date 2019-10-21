# 1 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Screen_controller/UART_Screen_controller.ino"
# 1 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Screen_controller/UART_Screen_controller.ino"
# 2 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Screen_controller/UART_Screen_controller.ino" 2
# 3 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Screen_controller/UART_Screen_controller.ino" 2
# 4 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Screen_controller/UART_Screen_controller.ino" 2
# 15 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Screen_controller/UART_Screen_controller.ino"
typedef union i2c_float_t {
  float float_value;
  byte byte_array[4];
} i2c_float;

typedef union i2c_int_t {
  int int_value;
  byte byte_array[2];
} i2c_int;

U8GLIB_SH1106_128X64 u8g(0); // I2C / TWI 

int cnt_main = 0;
int cnt_screen_update = 0;

i2c_int cnt_motor_ctrl;

int cnt_print_switch = 0;

i2c_float avgInputCurrent;
i2c_float ampHours;
i2c_float inpVoltage;
i2c_float rpm;
i2c_float throttle_current;
i2c_float brake_current;

// TODO: initiate i2c_float variables


void draw(int screen_page) {
  // graphic commands to redraw the complete screen should be placed here  
  //u8g.setFont(u8g_font_osb21);
  //u8g.drawStr( 0, 22, "VESC data");

  // box
  u8g.drawBox(0, 0, avgInputCurrent.float_value, 12);

  // row one
  u8g.setFont(u8g_font_fur14);
  u8g.setPrintPos(0, 32);
  u8g.print(cnt_motor_ctrl.int_value);

  u8g.setPrintPos(60, 32);
  u8g.print(cnt_screen_update);

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

  if (cnt_main > 1) {
    cnt_screen_update++;
    Wire.requestFrom(8, 26 /* from motor controller*/); // request bytes from slave device

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

  // picture loop
  u8g.firstPage();
  do {
    draw(1); } while( u8g.nextPage() );
}
