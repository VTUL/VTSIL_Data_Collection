#pragma once

#include "DAQ.h"

//This class controlls the action of the DAQ(s) the system is connected to.
//  This includes:
//      - Initializing them based on the hostnames of the daq(s) provided
//      - controlling the collection/writing cycle of all daq(s)
class DAQ_Interface
{

public:
    DAQ_Interface(); //constructor, takes the max to read in before writing to disk
	~DAQ_Interface(); //desctutor

    void daqBootStrap(string ipAddresses); //take in all the newtwork IDs of DAQs, initialize them all, then start collecting

	int initialize(string ipAddresses); //initialize a set of daqs

	int loadConfigureFile();

	void collectDaqData(); //call data collection for all the DAQs

	void writeDataToFile();

    int close(); //close out the session for all daq(s)

private:

	bool runningProgram;

	bool preparedData;

	vector<string> channelIDs; //CM 7/10/15 - change to using string insteam of QString
	vector<pair<int, string>> activeChannels; //Store all the channels' name connected to sensors

	int recordSize; //CM 5/18/15 - the number of data points in a record, used to parse the data stream appropriately for each channel's data
	double sampleRate; //CM 5/21/15 - record the sample rate of the DAQ (across the board for all sensors connected to the DAQ)
	int bufferSize;//the amount of data that is being store before a write to disk occcurs
	int blockSize;//default number of sample data stored in one file from one channel
	string movePath; //CM 6/2/15 - the local path where data files are moved once completed
	int clockFrequency;//clock frequency
	int prescaler;//prescaler

	double **totalSensorData;
	int totalDataRow;
	int totalDataCol;
	double timeStamp;

	//Used to mutual lock the two different threads
	std::mutex mutex;

	Config_Parser* configParser; //CM 6/1/15 - parser to load parameters for the DAQ

	DAQ* daqObject;//The single pointer pointing to a DAQ

	HDF5_Output* output; //HDF5 file output object
};
