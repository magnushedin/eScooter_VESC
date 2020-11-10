#include <Arduino.h>
#line 1 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
#include <Wire.h>

//#include "\include\DisplayData.h"
#include "buffer.h"
#include "datatypes.h"
#include "vesc.h"
#include "Nextion.h"

#define THROTTLE_PIN          (A1)
#define BRAKE_PIN             (A5)
#define THROTTLE_FILTER       (5)
#define BRAKE_FILTER          (5)
#define DEFAULT_THROTTLE_ZERO (210)
#define DEFAULT_BRAKE_ZERO    (170)
#define BLINK_PIN             (13)
#define MAX_BRAKE_CURRENT     (40)
#define MAX_BRAKE_VOLTAGE     (860)
#define MAX_MOTOR_CURRENT     (52)
#define MAX_THROTTLE_VOLTAGE  (980)


NexNumber n_speed = NexNumber(0, 2, "n_speed"); //integer value
NexNumber x_volt = NexNumber(0, 3, "x_volt");
NexNumber x_amp = NexNumber(0, 6, "x_amp");
NexDSButton bt_light = NexDSButton(0, 1, "bt_light");

NexTouch *nex_listen_list[] =
{
  &bt_light,
  NULL
};

//VescUart UART;

bool blink_state = 0;

int cnt_irc = 0;


float throttle_current;
float brake_current;
int throttle_values[THROTTLE_FILTER] = {0};
int brake_values[BRAKE_FILTER] = {0};
int throttle_value = 0;
int brake_value = 0;
int throttle_count = 0;
int brake_count = 0;

int itIsTime = 0;

int throttle_zero = DEFAULT_THROTTLE_ZERO;
int brake_zero = DEFAULT_BRAKE_ZERO;

/*
i2c_float avgInputCurrent;
//float avgMotorCurrent = 0;
i2c_float ampHours;
i2c_float inpVoltage;
i2c_float rpm;
int tachometer = 0;
//byte sensor_mode = 0;
//byte pwm_mode = 0;
//byte comm_mode = 0;
*/

bool light_on = LOW;
#line 67 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void buttonBtLight_Pop_Cbk(void *ptr);
#line 75 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
float mapfloat(long x, long in_min, long in_max, long out_min, long out_max);
#line 81 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void setup(void);
#line 129 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
int input_mean(int *input_values, int filter_count);
#line 140 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void readSensors();
#line 166 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void requestEvent(int howMany);
#line 207 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void loop(void);
#line 67 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void buttonBtLight_Pop_Cbk(void *ptr)
{
  light_on = !light_on;
  digitalWrite(LED_BUILTIN, light_on);
}

// TODO: initialize i2c_floats

float mapfloat(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}


void setup(void) {
  int setup_cnt = 0;
  bool tmp = LOW;
  int tmp_input_sum = 0;

  nexInit();
  bt_light.attachPop(buttonBtLight_Pop_Cbk, &bt_light);

  Serial.begin(115200);  //For debugg
  //Serial.println("Setup");

  Serial2.begin(115200);  //VESC 1
  while(!Serial2) {
  }
  //UART.setSerialPort(&Serial2);
  //UART.setDebugPort(&Serial);

  // VESC
  vescInit(&Serial2);

  //pinMode(8, OUTPUT);

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
  Wire.onRequest(requestEvent); // register even
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


void readSensors() //timer0 interrupt 2kHz
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
/*
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
*/
}

void loop(void) {
  readSensors();

  nexLoop(nex_listen_list);
  
  //digitalWrite(BLINK_PIN, blink_state);
  //blink_state = !blink_state;

  // Read UART

/*
  if (itIsTime >= 10) {
    //digitalWrite(BLINK_PIN, 1);
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
      tachometer = UART.data.tachometer;
      
      x_volt.setValue((int)(inpVoltage.float_value*100));
      n_speed.setValue((int)rpm.float_value);
      x_amp.setValue((int)(ampHours.float_value*100));
      Serial.print("Amp: ");
      Serial.println(ampHours.float_value);
      Serial.println("Read from VESC success");
    }
    else
    {
      Serial.println("Read from VESC failed");
    }
    //digitalWrite(BLINK_PIN, 0);
    itIsTime = 0;
  }
  */


  // Calculate mean values of input from each array
  throttle_value = input_mean(throttle_values, THROTTLE_FILTER);
  throttle_current= mapfloat(throttle_value, throttle_zero, MAX_THROTTLE_VOLTAGE, -0.3, MAX_MOTOR_CURRENT);
  brake_value = input_mean(brake_values, BRAKE_FILTER);
  brake_current = mapfloat(brake_value, brake_zero, MAX_BRAKE_VOLTAGE, 0, MAX_BRAKE_CURRENT);

  // Write throttle and brake values on UART
  if (brake_current > 0.5)
  {
    vescSetBrakeCurrent(brake_current);
  }
  else
  {
    if (throttle_current > 0)
    {
      vescSetCurrent(throttle_current);
    }
    else
    {
      vescSetCurrent(0);
    }
  }

}

