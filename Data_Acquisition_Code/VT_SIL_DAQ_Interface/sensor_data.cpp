
#include "sensor_data.h"

Sensor_Data::Sensor_Data(int r, SAFEARRAY* d, SAFEARRAY* ts, SAFEARRAY* tf)
{
    numRecords = r;
    data = d;
    timeSeconds = ts;
    timeFraction = tf;

    d = NULL;
    ts = NULL;
    tf = NULL;
}

Sensor_Data::~Sensor_Data()
{
    // Clean up our SafeArrays and BSTRs and close the connection
    ::SafeArrayDestroy(data);
    ::SafeArrayDestroy(timeSeconds);
    ::SafeArrayDestroy(timeFraction);
}

//implement all the setters and getters

int Sensor_Data::getNumRecords(){
    return numRecords;
}

SAFEARRAY* Sensor_Data::getData(){
    return data;
}

SAFEARRAY* Sensor_Data::getTimeSeconds(){
    return timeSeconds;
}

SAFEARRAY* Sensor_Data::getTimeFraction(){
    return timeFraction;
}

void Sensor_Data::setNumRecords(int r){
    numRecords = r;
}

void Sensor_Data::setData(SAFEARRAY* d){
    data = d;
}

void Sensor_Data::setTimeSeconds(SAFEARRAY* ts){
    timeSeconds = ts;
}

void Sensor_Data::setTimeFraction(SAFEARRAY* tf){
    timeFraction = tf;
}
