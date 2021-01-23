//#include <Wire.h>

#include "C:\Projects\eScooter_VESC\UART_Motor_controller\include\DisplayData.h"
#include "buffer.h"
#include "datatypes.h"
#include "vesc.h"
#include "Nextion.h"

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

/* Defines */
#define THROTTLE_PIN          (A9)
#define BRAKE_PIN             (A8)
#define PIN_LIGHT_FRONT       (2)
#define PIN_LIGHT_REAR        (3)

#define THROTTLE_FILTER       (10)
#define BRAKE_FILTER          (10)
#define DEFAULT_THROTTLE_ZERO (210)
#define DEFAULT_BRAKE_ZERO    (170)
#define MAX_BRAKE_CURRENT     (40)
#define MAX_BRAKE_VOLTAGE     (805)
#define MAX_MOTOR_CURRENT     (52)
#define MAX_THROTTLE_VOLTAGE  (980)

#define LIGHT_DEFAULT_ONOFF           (HIGH)


// From https://github.com/vedderb/bldc/blob/master/commands.c
#define VESC_DATA1   (0x000000A1UL) // FET temp, Erpm, Voltage
#define VESC_DATA2   (0x00010128UL) // Current, Erpm, Battery level, Fault code

#define CTRL_UPDATE_INTERVAL_MS (20)
#define MODE_CHANGE_COUNT       (1000 / CTRL_UPDATE_INTERVAL_MS)
#define SCREEN_SAVE_COUNT       (60000 / CTRL_UPDATE_INTERVAL_MS)

#define SEND_CTRL_COUNT         (5000 / CTRL_UPDATE_INTERVAL_MS)
#define READ_VESC_COUNT         (5000 / CTRL_UPDATE_INTERVAL_MS)
#define MAIN_LOOP_WRAP_COUNT    (SEND_CTRL_COUNT * READ_VESC_COUNT)


// --- Nextion display ---
NexNumber n_speed = NexNumber(0, 2, "n_speed"); //integer value
NexNumber x_volt = NexNumber(0, 3, "x_volt");
NexNumber x_fet_temp = NexNumber(3, 5, "x0");

NexNumber x_amp = NexNumber(0, 6, "x_amp");
NexProgressBar j_batt = NexProgressBar(0, 11, "j_batt");

NexDSButton bt_light = NexDSButton(0, 1, "bt_light");
NexDSButton bt_uartFront = NexDSButton(1, 8, "bt_uart_front");
NexDSButton bt_uartRear = NexDSButton(1, 9, "bt_uart_rear");

NexTouch *nex_listen_list[] =
{
    &bt_light,
    &bt_uartFront,
    &bt_uartRear,
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

bool uartRear_on = true;
bool uartFront_on = true;
bool light_on = LIGHT_DEFAULT_ONOFF;

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

void buttonBtLight_Pop_Cbk(void *ptr)
{
    light_on = !light_on;
    Serial.println("Changing light");
    digitalWrite(PIN_LIGHT_FRONT, light_on);
    digitalWrite(PIN_LIGHT_REAR, light_on);
}

void buttonBtUartRear_Pop_Cbk(void *ptr)
{
    uartRear_on = !uartRear_on;
    Serial.println("Rear UART on/off");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
}

void buttonBtUartFront_Pop_Cbk(void *ptr)
{
    uartFront_on = !uartFront_on;
    Serial.println("Front UART on/off");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
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
    int tmp[3];
    tmp[0] = analogRead(THROTTLE_PIN);
    tmp[1] = analogRead(THROTTLE_PIN);
    tmp[2] = analogRead(THROTTLE_PIN);

    throttle_values[throttle_count] = input_mean(tmp, 3);
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
    throttle_current= mapfloat(throttle_value, throttle_zero, MAX_THROTTLE_VOLTAGE, -0.5, MAX_MOTOR_CURRENT);
    brake_value = input_mean(brake_values, BRAKE_FILTER);
    brake_current = mapfloat(brake_value, brake_zero, MAX_BRAKE_VOLTAGE, 0, MAX_BRAKE_CURRENT);
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


static void readVescData(uint32_t mask, HardwareSerial* serialPort)
{
    //Serial.println("Requesting data from VESC");
    processVescRxData();
    vescRequestData(mask, serialPort);
}


void setup(void) {
    int setup_cnt = 0;
    bool tmp = LOW;
    int tmp_input_sum = 0;

    uartRear_on = true;
    uartFront_on = true;

    nexInit();
    bt_light.attachPop(buttonBtLight_Pop_Cbk, &bt_light);
    bt_uartRear.attachPop(buttonBtUartRear_Pop_Cbk, &bt_uartRear);
    bt_uartFront.attachPop(buttonBtUartFront_Pop_Cbk, &bt_uartFront);
    //bt_uart.attachPush(buttonBtUart_Pop_Cbk, &bt_uart);

    Serial.begin(115200);  // debug serial
    // While(!Serial){;} Don't wait for Serial0, this will hang the Teensy waiting if no computer is connected to the USB.

    Serial3.begin(115200);  // VESC 1
    //while(!Serial3) {;}

    Serial5.begin(115200); // VESC 2
    //while(!Serial5) {;}
    
    // VESC
    vescInit();

    // Set throttle zero position
    for (int i = 0; i < 100; i++) {
        tmp_input_sum += analogRead(THROTTLE_PIN);
        delay(5);
    }
    throttle_zero = (tmp_input_sum / 100) + 3;

    // Set brake zero position
    tmp_input_sum = 0;
    for (int i = 0; i < 100; i++) {
        tmp_input_sum += analogRead(THROTTLE_PIN);
        delay(5);
    }
    brake_zero = (tmp_input_sum / 100) + 3;

    //Wire.begin(8);                // join i2c bus with address #8
    //Wire.onRequest(requestEvent); // register event

    // Set Light defalut value at start
    digitalWrite(PIN_LIGHT_FRONT, light_on);
    digitalWrite(PIN_LIGHT_REAR,  light_on);
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
            readVescData(VESC_DATA1, &Serial3);
            n_speed.setValue((int)Ctx.rpm);
            x_volt.setValue((int)(DisplayData.VescData.inpVoltage)*10);
            Serial.print("speed, volt: ");
            Serial.print(Ctx.rpm);
            Serial.print(", ");
            Serial.println(DisplayData.VescData.inpVoltage*10);
        }
        else {
            readVescData(VESC_DATA2, &Serial3);
            x_amp.setValue((int)DisplayData.VescData.avgInputCurrent*10);
            x_fet_temp.setValue((int)DisplayData.VescData.fetTemp*100);
            j_batt.setValue((int)DisplayData.VescData.batteryLevel);
            //Serial.print("Amp: ");
            //Serial.println(DisplayData.VescData.avgInputCurrent*10);
        }
        read_vesc_count++;
    }

    // Write throttle and brake values on UART
    if ((loopCnt % SEND_CTRL_COUNT) == 0) {
      //Serial.println("Control loop");
        if (brake_current > 1.5)
        {
            if (uartRear_on) {
              vescSetBrakeCurrent(brake_current, &Serial5);
            }
            if (uartFront_on) {
              vescSetBrakeCurrent(brake_current, &Serial3);
            }
            /*
            Serial.print("Brake current: ");
            Serial.print(brake_current);
            Serial.print(" (");
            Serial.print(brake_value);
            Serial.println(")");
            */
        }
        else
        {
            if (throttle_current > 0)
            {
                if (uartRear_on) {
                    vescSetCurrent(throttle_current, &Serial5);
                    Serial.println("Writing on Serial 5");
                }
                if (uartFront_on) {
                    vescSetCurrent(throttle_current, &Serial3);
                    Serial.println("Serial 3");
                }
                /*
                Serial.print("Requested throttle current: ");
                Serial.println(throttle_current);
                for (int i=0; i<THROTTLE_FILTER; i++) {
                    Serial.print(throttle_values[i]);
                    Serial.print(", ");
                }
                */
                /*
                Serial.print(input_mean(throttle_values, THROTTLE_FILTER));
                Serial.print(", ");
                Serial.print(throttle_zero);
                Serial.println("");
                */
            }
            else
            {
                if (uartRear_on) {
                    vescSetCurrent(0, &Serial5);                  
                }
                if (uartFront_on) {
                    vescSetCurrent(0, &Serial3);                  
                }

                /*
                Serial.print("Requested throttle current < 0: ");
                Serial.println(throttle_current);
                */
            }
        }
    }
    
    loopCnt++;
    if (loopCnt >= MAIN_LOOP_WRAP_COUNT) {
        loopCnt = 0;
    }
}
