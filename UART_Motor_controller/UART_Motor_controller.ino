#include "VescUart.h"
#include <Wire.h>

#define THROTTLE_PIN A7
#define BRAKE_PIN A2
#define THROTTLE_FILTER 5
#define BRAKE_FILTER 5
#define DEFAULT_THROTTLE_ZERO 210
#define DEFAULT_BRAKE_ZERO 170
#define BLINK_PIN 13
#define MAX_BRAKE_CURRENT 40
#define MAX_BRAKE_VOLTAGE 860
#define MAX_MOTOR_CURRENT 52
#define MAX_THROTTLE_VOLTAGE 980


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
int throttle_values[THROTTLE_FILTER] = {0};
int brake_values[BRAKE_FILTER] = {0};
int throttle_value = 0;
int brake_value = 0;
int throttle_count = 0;
int brake_count = 0;

int itIsTime = 0;

int throttle_zero = DEFAULT_THROTTLE_ZERO;
int brake_zero = DEFAULT_BRAKE_ZERO;

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


  cli(); //stop interrupts
         //set timer1 interrupt at 10Hz
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1 = 0;  //initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 262; // = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();

  // Set throttle zero position
  for (int i = 0; i < 10; i++) {
    tmp_input_sum += analogRead(THROTTLE_PIN);
    delay(3);
  }
  throttle_zero = (tmp_input_sum / 10) + 3;

  // Set brake zero position
  tmp_input_sum = 0;
  for (int i = 0; i < 10; i++) {
    tmp_input_sum += analogRead(THROTTLE_PIN);
    delay(3);
  }
  brake_zero = (tmp_input_sum / 10) + 3;


  // IMPLEMENT: Initialize the brake in similar way as throttel

  Wire.begin(8);                // join i2c bus with address #8
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


ISR(TIMER1_COMPA_vect) //timer0 interrupt 2kHz
{
  //digitalWrite(BLINK_PIN, 1);
  throttle_values[throttle_count] = analogRead(THROTTLE_PIN);
  brake_values[brake_count] = analogRead(BRAKE_PIN);

  brake_count++;
  if (brake_count >= BRAKE_FILTER) {
    brake_count = 0;
  }

  throttle_count++;
  if (throttle_count >= THROTTLE_FILTER)
  {
    throttle_count = 0;
  }

  cnt_irc++;
  //digitalWrite(BLINK_PIN, 0);

  itIsTime++;
}


// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent(int howMany)
{
  //digitalWrite(BLINK_PIN, 1);
  //Serial.println(howMany);

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

  if (itIsTime >= 4) {
    digitalWrite(BLINK_PIN, 1);
    if (UART.getVescValues())
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
    digitalWrite(BLINK_PIN, 0);
    itIsTime = 0;
  }


  // Calculate mean values of input from each array
  throttle_value = input_mean(throttle_values, THROTTLE_FILTER);
  throttle_current.float_value = mapfloat(throttle_value, throttle_zero, MAX_THROTTLE_VOLTAGE, -0.3, MAX_MOTOR_CURRENT);
  brake_value = input_mean(brake_values, BRAKE_FILTER);
  brake_current.float_value = mapfloat(brake_value, brake_zero, MAX_BRAKE_VOLTAGE, 0, MAX_BRAKE_CURRENT);

  // Write throttle and brake values on UART
  if (brake_current.float_value > 0.5)
  {
    if (rpm.float_value > 10) {
      UART.setBrakeCurrent(brake_current.float_value);
    }
    else {
      UART.setCurrent(0);
    }
  }
  else
  {
    if (throttle_current.float_value > 0)
    {
      UART.setCurrent(throttle_current.float_value);
    }
    else
    {
      UART.setCurrent(0);
    }
  }

}
