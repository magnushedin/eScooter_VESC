#ifndef DisplayData_h
#define DisplayData_h

#define DISPLAY_DATA_DEBUG_COUNT (6u)

typedef enum
{
    DISPLAY_error,
    DISPLAY_speed,
    DISPLAY_vesc,
    DISPLAY_ctrl,
    DISPLAY_amps,
    DISPLAY_serial,
    DISPLAY_debug,
    DISPLAY_graph,
    DISPLAY_tc,
    DISPLAY_lights,
    DISPLAY_maps,
    DISPLAY_blank,
    DISPLAY_ctrl_rear,
    DISPLAY_vesc_rear
} DisplayDataType;

typedef struct
{
    long speed;                  // in km/h * 10
    unsigned int voltage;
    unsigned char batteryLevel;  // in percent 0-100
} SpeedDataType;

typedef struct
{
    float avgInputCurrent;
    long rpm;
    float fetTemp;
    unsigned int inpVoltage;
    unsigned char batteryLevel;
    unsigned char faultCode;
    float speed;
    float ahUsed;
} VescDataType;

typedef struct
{
    int rawThrottleValue;
    int rawBrakeValue;
    int rawOtherBrakeValue;
    long throttleValue;
    long brakeValue;
    long otherBrakeValue;
} CtrlDataType;

typedef struct
{
    float amps;
} AmpsDataType;

typedef struct
{
    unsigned char rbCapacity;
    unsigned char rbWatermark;
    unsigned int rxOverflows;
    unsigned int txOverflows;
} SerialStatsType;

typedef struct
{
    long data[DISPLAY_DATA_DEBUG_COUNT];
} DebugDataType;

typedef struct
{
    unsigned long max;
    unsigned long value;
    bool slowUpdate;
    bool floating;
} GraphDataType;

typedef struct
{
    bool on;
} TcDataType;

typedef struct
{
    bool on;
} LightsDataType;

typedef struct
{
    unsigned char numberOfMaps;
    unsigned char map;
} MapsDataType;

typedef struct
{
    DisplayDataType type;
    union {
        SpeedDataType SpeedData;
        VescDataType VescData;
        CtrlDataType CtrlData;
        AmpsDataType AmpsData;
        SerialStatsType SerialData;
        DebugDataType DebugData;
        GraphDataType GraphData;
        TcDataType TcData;
        LightsDataType LightsData;
        MapsDataType MapsData;
        CtrlDataType CtrlDataRear;
        VescDataType VescDataRear;
    } u;
} DisplayDataContainerType;

#endif
