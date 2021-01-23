#include <Arduino.h>
#line 1 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
//#include <Wire.h>

#include "C:\Projects\eScooter_VESC\UART_Motor_controller\include\DisplayData.h"
#include "buffer.h"
#include "datatypes.h"
#include "vesc.h"
#include "Nextion.h"

#define OTHER_BRAKE_PIN            (A2)
#define ANALOG_MAX                 (800)
#define BRAKE_ANALOG_MAX           (600)
#define THROTTLE_DEADZONE          (25)
#define THROTTLE_MIN               (10)    // Amps * 10
#define THROTTLE_MAX               (400)   // Amps * 10
#define BRAKE_DEADZONE             (10)
#define BRAKE_MIN                  (60)    // Amps * 10
#define BRAKE_MAX                  (400)   // Amps * 10
#define THROTTLE_FILTER_COUNT      (8)
#define THROTTLE_FILTER_MIN_COUNT  (4)
#define THROTTLE_FILTER_DIFF       (40)
#define VOLTAGE_FILTER_COUNT       (20)
#define BATT_LEVEL_FILTER_COUNT    (20)
#define BRAKE_FILTER_COUNT         (4)

#define THROTTLE_PIN          (A1)
#define BRAKE_PIN             (A5)
#define THROTTLE_FILTER       (5)
#define BRAKE_FILTER          (5)
#define DEFAULT_THROTTLE_ZERO (210)
#define DEFAULT_BRAKE_ZERO    (170)
#define MAX_BRAKE_CURRENT     (40)
#define MAX_BRAKE_VOLTAGE     (860)
#define MAX_MOTOR_CURRENT     (52)
#define MAX_THROTTLE_VOLTAGE  (980)



// From https://github.com/vedderb/bldc/blob/master/commands.c
#define VESC_DATA1   (0x000000A1UL) // FET temp, Erpm, Voltage
#define VESC_DATA2   (0x00010128UL) // Current, Erpm, Battery level, Fault code

#define CTRL_UPDATE_INTERVAL_MS (20)
#define MODE_CHANGE_COUNT       (1000 / CTRL_UPDATE_INTERVAL_MS)
#define SCREEN_SAVE_COUNT       (60000 / CTRL_UPDATE_INTERVAL_MS)

#define SEND_CTRL_COUNT         (50 / CTRL_UPDATE_INTERVAL_MS)
#define READ_VESC_COUNT         (50 / CTRL_UPDATE_INTERVAL_MS)
#define MAIN_LOOP_WRAP_COUNT    (SEND_CTRL_COUNT * READ_VESC_COUNT)


NexNumber n_speed = NexNumber(0, 2, "n_speed"); //integer value
NexNumber x_volt = NexNumber(0, 3, "x_volt");

NexNumber x_amp = NexNumber(0, 6, "x_amp");
NexDSButton bt_light = NexDSButton(0, 1, "bt_light");

NexTouch *nex_listen_list[] =
{
    &bt_light,
    NULL
};


float throttle_current;
float brake_current;
int throttle_values[THROTTLE_FILTER] = {0};
int brake_values[BRAKE_FILTER] = {0};
int throttle_value = 0;
int brake_value = 0;
int throttle_count = 0;
int brake_count = 0;

int throttle_zero = DEFAULT_THROTTLE_ZERO;
int brake_zero = DEFAULT_BRAKE_ZERO;


typedef struct
{
    DisplayDataType type;
    VescDataType VescData;
    CtrlDataType CtrlData;
} DisplayDataStructType;

typedef struct
{
    int initialThrottleValue;
    int initialBrakeValue;
    int initialOtherBrakeValue;
    int rawThrottleValue;
    int rawBrakeValue;
    int rawOtherBrakeValue;
    long throttleValue;
    long brakeValue;
    long otherBrakeValue;
    long screenSaveCnt;
    int modeCnt;
    float rpm;
    DisplayDataType prevDisplayType;
} CtxType;

static CtxType Ctx;
static DisplayDataStructType DisplayData; // local storage of display data


bool light_on = LOW;
#line 106 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void buttonBtLight_Pop_Cbk(void *ptr);
#line 113 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
float mapfloat(long x, long in_min, long in_max, long out_min, long out_max);
#line 119 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
int input_mean(int *input_values, int filter_count);
#line 131 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
unsigned char getFilteredBatteryValue(unsigned char newValue);
#line 151 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
unsigned int getFilteredVoltageValue(unsigned int newValue);
#line 170 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void readSensors();
#line 199 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void requestEvent(int howMany);
#line 240 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void extractVescData(unsigned char buf[], int bufLen);
#line 277 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void processVescRxData();
#line 293 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
static void readVescData(uint32_t mask);
#line 300 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void setup(void);
#line 336 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void loop(void);
#line 106 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
void buttonBtLight_Pop_Cbk(void *ptr)
{
    light_on = !light_on;
    digitalWrite(LED_BUILTIN, light_on);
}


float mapfloat(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}


int input_mean(int *input_values, int filter_count)
{
    int tot = 0;
    for (int i = 0; i < filter_count; i++)
    {
       tot += input_values[i];
    }
    return tot/filter_count;
}


// Set new value and return the average value of stored values
unsigned char getFilteredBatteryValue(unsigned char newValue)
{
    static unsigned char batteryValues[BATT_LEVEL_FILTER_COUNT] = {0};
    static unsigned int idx = 0;
    unsigned int sum = 0;

    batteryValues[idx++] = newValue;
    if (idx >= BATT_LEVEL_FILTER_COUNT) {
        idx = 0;
    }

    for (int i = 0; i < BATT_LEVEL_FILTER_COUNT; i++) {
        sum += batteryValues[i];
    }

    return sum / BATT_LEVEL_FILTER_COUNT;
}


// Set new value and return the average value of stored values
unsigned int getFilteredVoltageValue(unsigned int newValue)
{
    static unsigned int voltageValues[VOLTAGE_FILTER_COUNT] = {0};
    static unsigned int idx = 0;
    unsigned int sum = 0;

    voltageValues[idx++] = newValue;
    if (idx >= VOLTAGE_FILTER_COUNT) {
        idx = 0;
    }

    for (int i = 0; i < VOLTAGE_FILTER_COUNT; i++) {
        sum += voltageValues[i];
    }

    return sum / VOLTAGE_FILTER_COUNT;
}


void readSensors()
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

    //digitalWrite(BLINK_PIN, 0);

    // Calculate mean values of input from each array
    throttle_value = input_mean(throttle_values, THROTTLE_FILTER);
    throttle_current= mapfloat(throttle_value, throttle_zero, MAX_THROTTLE_VOLTAGE, -0.3, MAX_MOTOR_CURRENT);
    brake_value = input_mean(brake_values, BRAKE_FILTER);
    brake_current = mapfloat(brake_value, brake_zero, MAX_BRAKE_VOLTAGE, 0, MAX_BRAKE_CURRENT);
}


// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent(int howMany)
{

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


void extractVescData(unsigned char buf[], int bufLen)
{
    COMM_PACKET_ID packetId;
    int32_t ind = 0;
    uint32_t mask;

    if (buf == NULL) {
        return;
    }

    packetId = (COMM_PACKET_ID)buf[0];
    ind++;

    switch(packetId) {
        case COMM_GET_VALUES_SETUP_SELECTIVE:
        mask = buffer_get_uint32((uint8_t*)buf, &ind); // Used to figure out which data is comming.

        switch (mask) {
            case VESC_DATA1:
            DisplayData.VescData.fetTemp = buffer_get_float16((uint8_t*)buf, 10.0, &ind);
            Ctx.rpm = buffer_get_float32((uint8_t*)buf, 1.0, &ind);
            DisplayData.VescData.inpVoltage = getFilteredVoltageValue((unsigned int)(buffer_get_float16((uint8_t*)buf, 10.0, &ind) * 10));
            break;

            case VESC_DATA2:
            DisplayData.VescData.avgInputCurrent = buffer_get_float32((uint8_t*)buf, 100.0, &ind);
            Ctx.rpm = buffer_get_float32((uint8_t*)buf, 1.0, &ind);
            DisplayData.VescData.batteryLevel = getFilteredBatteryValue((unsigned char)(buffer_get_float16((uint8_t*)buf, 1000.0, &ind) * 100));
            DisplayData.VescData.faultCode = buf[ind++];
            break;
        }
        //DisplayData.VescData.rpm = Ctx.rpm;
        break;
    }
}


void processVescRxData()
{
    if (vescRxDataAvailable() > 0) {
        unsigned char msgBuf[64];
        int len;

        memset(msgBuf, 0, sizeof(msgBuf));
        len = vescTryDecodePacket(msgBuf, sizeof(msgBuf));

        if (len > 0) {
            extractVescData(msgBuf, len);
        }
    }
}


static void readVescData(uint32_t mask)
{
    processVescRxData();
    vescRequestData(mask);
}


void setup(void) {
    int setup_cnt = 0;
    bool tmp = LOW;
    int tmp_input_sum = 0;

    nexInit();
    bt_light.attachPop(buttonBtLight_Pop_Cbk, &bt_light);

    Serial.begin(115200);  //For debug

    Serial3.begin(115200);  //VESC 1
    while(!Serial3) {;}

    // VESC
    vescInit(&Serial3);

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

    //Wire.begin(8);                // join i2c bus with address #8
    //Wire.onRequest(requestEvent); // register even
}


void loop(void) {
    static unsigned long lastTS = 0;
    static unsigned long loopCnt = 0;
    readSensors();

    nexLoop(nex_listen_list);
    //Serial.println(Ctx.rpm);
    //Serial.println(DisplayData.VescData.inpVoltage);

    // Read VESC data
    if (loopCnt % READ_VESC_COUNT == 0) {
        static unsigned char read_vesc_count = 0;

        // Using two messages to reduce load spikes on the serial bus.
        if ((read_vesc_count % 2) == 0) {
            readVescData(VESC_DATA1);
            //delay(500);
            n_speed.setValue((int)Ctx.rpm);
            x_volt.setValue((int)(DisplayData.VescData.inpVoltage)*10);
            //Serial.println("VESC_DATA_1");
        }
        else {
            readVescData(VESC_DATA2);
            x_amp.setValue((int)DisplayData.VescData.avgInputCurrent);
            //Serial.println("VESC_DATA_2");
        }
        read_vesc_count++;
    }

    loopCnt++;
    if (loopCnt >= MAIN_LOOP_WRAP_COUNT) {
        loopCnt = 0;
    }

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


