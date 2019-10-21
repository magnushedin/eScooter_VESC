# 1 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Motor_controller/UART_Motor_controller.ino"
# 1 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Motor_controller/UART_Motor_controller.ino"
# 2 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Motor_controller/UART_Motor_controller.ino" 2
# 3 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Motor_controller/UART_Motor_controller.ino" 2
# 17 "/home/magnus/Arduino/Projects/eScooter_VESC/UART_Motor_controller/UART_Motor_controller.ino"
typedef union i2c_float_t {
  float float_value;
  byte byte_array[4];
} i2c_float;

typedef union i2c_int_t {
  int int_value;
  byte int_array[2];
} i2c_int;

VescUart UART;

bool blink_state = 0;

i2c_int cnt_main;
int cnt_irc = 0;


i2c_float throttle_current;
i2c_float brake_current;
int throttle_values[5] = {0};
int brake_values[5] = {0};
int throttle_value = 0;
int brake_value = 0;
int throttle_count = 0;
int brake_count = 0;

int throttle_zero = 210;
int brake_zero = 170;

i2c_float avgInputCurrent;
//float avgMotorCurrent = 0;
i2c_float ampHours;
i2c_float inpVoltage;
i2c_float rpm;
//float tachometer = 0;
//byte sensor_mode = 0;
//byte pwm_mode = 0;
//byte comm_mode = 0;

// TODO: initialize i2c_floats

float mapfloat(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}


void setup(void) {
  int tmp_input_sum = 0;
  cnt_main.int_value = 0;

  Serial.begin(115200);
  while(!Serial) {
  }
  UART.setSerialPort(&Serial);

  //pinMode(8, OUTPUT);


  __asm__ __volatile__ ("cli" ::: "memory"); //stop interrupts
  //set timer1 interrupt at 1Hz
  (*(volatile uint8_t *)(0x80)) = 0;// set entire TCCR1A register to 0
  (*(volatile uint8_t *)(0x81)) = 0;// same for TCCR1B
  (*(volatile uint16_t *)(0x84)) = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  (*(volatile uint16_t *)(0x88)) = 500;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  (*(volatile uint8_t *)(0x81)) |= (1 << 3);
  // Set CS12 and CS10 bits for 1024 prescaler
  (*(volatile uint8_t *)(0x81)) |= (1 << 2) | (1 << 0);
  // enable timer compare interrupt
  (*(volatile uint8_t *)(0x6F)) |= (1 << 1);
  __asm__ __volatile__ ("sei" ::: "memory");

  // Set throttle zero position
  for (int i = 0; i < 10; i++) {
    tmp_input_sum += analogRead(A7);
    delay(3);
  }
  throttle_zero = (tmp_input_sum / 10) + 3;

  // Set brake zero position
  tmp_input_sum = 0;
  for (int i = 0; i < 10; i++) {
    tmp_input_sum += analogRead(A7);
    delay(3);
  }
  brake_zero = (tmp_input_sum / 10) + 3;


  // IMPLEMENT: Initialize the brake in similar way as throttel

  Wire.begin(8); // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
}

//IMPLEMENT: Implement some kind of functionality for adapting max position of throttle and brake

//IMPLEMENT: Table for throttele and brake for setting exponential curves.

int input_mean(int *input_values, int filter_count)
{
  int tot = 0;
  for (int i = 0; i < filter_count; i++)
  {
    tot += input_values[i];
  }
  return tot/filter_count;
}


extern "C" void __vector_11 /* Timer/Counter1 Compare Match A */ (void) __attribute__ ((signal,used, externally_visible)) ; void __vector_11 /* Timer/Counter1 Compare Match A */ (void) //timer0 interrupt 2kHz
{
  //digitalWrite(BLINK_PIN, 1);
  throttle_values[throttle_count] = analogRead(A7);
  brake_values[brake_count] = analogRead(A2);

  brake_count++;
  if (brake_count >= 5) {
    brake_count = 0;
  }

  throttle_count++;
  if (throttle_count >= 5)
  {
    throttle_count = 0;
  }

  cnt_irc++;
  //digitalWrite(BLINK_PIN, 0);
}


// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent(int howMany)
{
  //digitalWrite(BLINK_PIN, 1);
  Serial.println(howMany);

  Wire.write(cnt_main.int_array[0]);
  Wire.write(cnt_main.int_array[1]);

  Wire.write(rpm.byte_array[0]);
  Wire.write(rpm.byte_array[1]);
  Wire.write(rpm.byte_array[2]);
  Wire.write(rpm.byte_array[3]);

  Wire.write(avgInputCurrent.byte_array[0]);
  Wire.write(avgInputCurrent.byte_array[1]);
  Wire.write(avgInputCurrent.byte_array[2]);
  Wire.write(avgInputCurrent.byte_array[3]);

  Wire.write(ampHours.byte_array[0]);
  Wire.write(ampHours.byte_array[1]);
  Wire.write(ampHours.byte_array[2]);
  Wire.write(ampHours.byte_array[3]);

  Wire.write(inpVoltage.byte_array[0]);
  Wire.write(inpVoltage.byte_array[1]);
  Wire.write(inpVoltage.byte_array[2]);
  Wire.write(inpVoltage.byte_array[3]);

  Wire.write(throttle_current.byte_array[0]);
  Wire.write(throttle_current.byte_array[1]);
  Wire.write(throttle_current.byte_array[2]);
  Wire.write(throttle_current.byte_array[3]);

  Wire.write(brake_current.byte_array[0]);
  Wire.write(brake_current.byte_array[1]);
  Wire.write(brake_current.byte_array[2]);
  Wire.write(brake_current.byte_array[3]);
  //digitalWrite(BLINK_PIN, 0);
}

void loop(void) {
  cnt_main.int_value++;

  //digitalWrite(BLINK_PIN, blink_state);
  //blink_state = !blink_state;

  // Read UART

  digitalWrite(13, 1);
  //if (UART.getVescValues())
  {
    avgInputCurrent.float_value = UART.data.avgInputCurrent;
    //avgMotorCurrent = UART.data.avgMotorCurrent;
    ampHours.float_value = UART.data.ampHours;
    inpVoltage.float_value = UART.data.inpVoltage;
    //sensor_mode = UART.data.sensor_mode;
    //pwm_mode = UART.data.pwm_mode;
    //comm_mode = UART.data.comm_mode;
    rpm.float_value = UART.data.rpm;
    //tachometer = UART.data.tachometer;
  }
  digitalWrite(13, 0);

  //delay(200);

  // Calculate mean values of input from each array
  __asm__ __volatile__ ("cli" ::: "memory");
  throttle_value = input_mean(throttle_values, 5);
  throttle_current.float_value = mapfloat(throttle_value, throttle_zero, 980, -0.3, 52);
  brake_value = input_mean(brake_values, 5);
  brake_current.float_value = mapfloat(brake_value, brake_zero, 860, 0, 40);
  __asm__ __volatile__ ("sei" ::: "memory");

  // Write throttle and brake values on UART
  if (brake_current.float_value > 0.5)
  {
    if (rpm.float_value > 10) {
      __asm__ __volatile__ ("cli" ::: "memory");
      UART.setBrakeCurrent(brake_current.float_value);
      __asm__ __volatile__ ("sei" ::: "memory");
    }
    else {
      __asm__ __volatile__ ("cli" ::: "memory");
      UART.setCurrent(0);
      __asm__ __volatile__ ("sei" ::: "memory");
    }
  }
  else
  {
    if (throttle_current.float_value > 0)
    {
      __asm__ __volatile__ ("cli" ::: "memory");
      UART.setCurrent(throttle_current.float_value);
      __asm__ __volatile__ ("sei" ::: "memory");
    }
    else
    {
      __asm__ __volatile__ ("cli" ::: "memory");
      UART.setCurrent(0);
      __asm__ __volatile__ ("sei" ::: "memory");
    }
  }

}
