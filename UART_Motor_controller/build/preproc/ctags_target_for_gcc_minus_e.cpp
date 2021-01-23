# 1 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
//#include <Wire.h>

# 4 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino" 2
# 5 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino" 2
# 6 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino" 2
# 7 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino" 2
# 8 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino" 2
# 38 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
// From https://github.com/vedderb/bldc/blob/master/commands.c
# 51 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
NexNumber n_speed = NexNumber(0, 2, "n_speed"); //integer value
NexNumber x_volt = NexNumber(0, 3, "x_volt");

NexNumber x_amp = NexNumber(0, 6, "x_amp");
NexDSButton bt_light = NexDSButton(0, 1, "bt_light");

NexTouch *nex_listen_list[] =
{
    &bt_light,
    
# 60 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino" 3 4
   __null

# 61 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
};


float throttle_current;
float brake_current;
int throttle_values[(5)] = {0};
int brake_values[(5)] = {0};
int throttle_value = 0;
int brake_value = 0;
int throttle_count = 0;
int brake_count = 0;

int throttle_zero = (210);
int brake_zero = (170);


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


bool light_on = 0x0;
void buttonBtLight_Pop_Cbk(void *ptr)
{
    light_on = !light_on;
    digitalWrite(13, light_on);
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
    static unsigned char batteryValues[(20)] = {0};
    static unsigned int idx = 0;
    unsigned int sum = 0;

    batteryValues[idx++] = newValue;
    if (idx >= (20)) {
        idx = 0;
    }

    for (int i = 0; i < (20); i++) {
        sum += batteryValues[i];
    }

    return sum / (20);
}


// Set new value and return the average value of stored values
unsigned int getFilteredVoltageValue(unsigned int newValue)
{
    static unsigned int voltageValues[(20)] = {0};
    static unsigned int idx = 0;
    unsigned int sum = 0;

    voltageValues[idx++] = newValue;
    if (idx >= (20)) {
        idx = 0;
    }

    for (int i = 0; i < (20); i++) {
        sum += voltageValues[i];
    }

    return sum / (20);
}


void readSensors()
{
    //digitalWrite(BLINK_PIN, 1);
    throttle_values[throttle_count] = analogRead((A1));
    brake_values[brake_count] = analogRead((A5));

    brake_count++;
    if (brake_count >= (5)) {
      brake_count = 0;
    }

    throttle_count++;
    if (throttle_count >= (5))
    {
      throttle_count = 0;
    }

    //digitalWrite(BLINK_PIN, 0);

    // Calculate mean values of input from each array
    throttle_value = input_mean(throttle_values, (5));
    throttle_current= mapfloat(throttle_value, throttle_zero, (980), -0.3, (52));
    brake_value = input_mean(brake_values, (5));
    brake_current = mapfloat(brake_value, brake_zero, (860), 0, (40));
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
# 237 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
}


void extractVescData(unsigned char buf[], int bufLen)
{
    COMM_PACKET_ID packetId;
    int32_t ind = 0;
    uint32_t mask;

    if (buf == 
# 246 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino" 3 4
              __null
# 246 "c:\\Projects\\eScooter_VESC\\UART_Motor_controller\\UART_Motor_controller.ino"
                  ) {
        return;
    }

    packetId = (COMM_PACKET_ID)buf[0];
    ind++;

    switch(packetId) {
        case COMM_GET_VALUES_SETUP_SELECTIVE:
        mask = buffer_get_uint32((uint8_t*)buf, &ind); // Used to figure out which data is comming.

        switch (mask) {
            case (0x000000A1UL) /* FET temp, Erpm, Voltage*/:
            DisplayData.VescData.fetTemp = buffer_get_float16((uint8_t*)buf, 10.0, &ind);
            Ctx.rpm = buffer_get_float32((uint8_t*)buf, 1.0, &ind);
            DisplayData.VescData.inpVoltage = getFilteredVoltageValue((unsigned int)(buffer_get_float16((uint8_t*)buf, 10.0, &ind) * 10));
            break;

            case (0x00010128UL) /* Current, Erpm, Battery level, Fault code*/:
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
    bool tmp = 0x0;
    int tmp_input_sum = 0;

    nexInit();
    bt_light.attachPop(buttonBtLight_Pop_Cbk, &bt_light);

    Serial.begin(115200); //For debug

    Serial3.begin(115200); //VESC 1
    while(!Serial3) {;}

    // VESC
    vescInit(&Serial3);

    // Set throttle zero position
    for (int i = 0; i < 10; i++) {
        tmp_input_sum += analogRead((A1));
        delay(3);
    }
    throttle_zero = (tmp_input_sum / 10) + 3;

    // Set brake zero position
    tmp_input_sum = 0;
    for (int i = 0; i < 10; i++) {
        tmp_input_sum += analogRead((A1));
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
    if (loopCnt % (50 / (20)) == 0) {
        static unsigned char read_vesc_count = 0;

        // Using two messages to reduce load spikes on the serial bus.
        if ((read_vesc_count % 2) == 0) {
            readVescData((0x000000A1UL) /* FET temp, Erpm, Voltage*/);
            //delay(500);
            n_speed.setValue((int)Ctx.rpm);
            x_volt.setValue((int)(DisplayData.VescData.inpVoltage)*10);
            //Serial.println("VESC_DATA_1");
        }
        else {
            readVescData((0x00010128UL) /* Current, Erpm, Battery level, Fault code*/);
            x_amp.setValue((int)DisplayData.VescData.avgInputCurrent);
            //Serial.println("VESC_DATA_2");
        }
        read_vesc_count++;
    }

    loopCnt++;
    if (loopCnt >= ((50 / (20)) * (50 / (20)))) {
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
