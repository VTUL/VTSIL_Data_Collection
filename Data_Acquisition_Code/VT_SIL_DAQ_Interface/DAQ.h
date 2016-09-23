#pragma once

#include "config_parser.h"
#include "HDF5_Output.h"

class DAQ
{
public:
	DAQ();
	~DAQ();

	int initialize(string ipAddress, int sampleRate, int recordSize, int clockFrequency, int prescaler, int bufferSize); //initialize the driver session, i.e., connect to the DAQ specified by the ip address

    bool continuousRawData();  //CM 4/13/15 - test just gathing the raw, unprocessed linear stream of sensor data,
                                                                           //   e.g., do not seperate which data goes to which sensor, but keep linear raw data
    void appendToData(Sensor_Data* sd); //CM 5/18/15 - storing the safearray from the DAQ directly

	void getActiveChannels(vector<string>& channelIDs, vector<pair<int, string>>& activeChannels); //CM 5/18/15 - storing the safearray from the DAQ directly

	void writeDataToGlobal(double* totalSensorData[], double* timeStamp, vector<pair<int, string>> activeChannels); //write the data in each channel to the specified output object

	int close(); //Close the DAQ session gracefully

    bool clearBuffer(); //CM 3/18/15 - clear the buffer for each channel

private:
    IVTEXDsaPtr driver;     //driver object to interact with the DAQ
    long actual_records;    //the number of actual records return from the DAQ per each call to the DAQ for records
    BSTR channels;          //the channel names (identifiers) corresponding to the data returned
    SAFEARRAY *data;        //the sensor data returned
    SAFEARRAY *ts_sec;      //the seconds of the timestamp of the latest returned sensor data
    SAFEARRAY *ts_frac;     //the fractions of a second for the timestampe (goes with above)
    BSTR addl_data;         //any additional data available

    string daqID; //the unique ID of the DAQ, i.e., the network name for the daq
    long numPhysicalChannels; //number of physical channels

    long int bufferSize; //the amount of data that is being store before a write to disk occcurs
    bool bufferFull; //CM 5/11/15 - flag to signal if the buffer is full so the buffer can be saved and the buffer flushed

    vector<Sensor_Data*> sensorData; //CM 5/26/15 - the sensor data returned from a memory read of the DAQ, each object is the result
                                          //  of a memory read
	int recordSize; //CM 5/18/15 - the number of data points in a record, used to parse the data stream appropriately for each channel's data
	double sampleRate; //CM 5/21/15 - record the sample rate of the DAQ (across the board for all sensors connected to the DAQ)

    int numCollectedRecords; //keep track of the actual number of records collected
};
