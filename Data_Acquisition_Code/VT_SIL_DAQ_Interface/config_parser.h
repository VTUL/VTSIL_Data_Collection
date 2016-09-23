#pragma once

#include "library.h"

//This class reads in a configuration file for DAQs and pulls out the necessary information for the DAQ(s)
class Config_Parser
{
public:
	//Construction and Destruction functions
     Config_Parser(int sampR = 1600);

    ~Config_Parser();

    void parseConfigFileStr(string fName); //Take the file name of the config file and parse it

	//Return all the parameters:expected sample rate, secondsperfile(60), blockSize, folder path to store the compressed HDF5 file
	//whether to store all the channel data, whether to see if the DAQ is filling up and records are not being pulled fast enough
    double getSampleRate();
    int getSecondsPerFile();
	int getBlockSize();
    string getMovePath();
    bool getStoreAllChannelData();
    bool getCheckRecordsOnDaq();

	//Set all the parameters:expected sample rate, secondsperfile, blockSize, folder path to store the compressed HDF5 file
	//whether to store all the channel data, whether to see if the DAQ is filling up and records are not being pulled fast enough
    void setSampleRate(double sr);
    void setSecondsPerFile(int spf);
	void setBlockSize(int p);
    void setMovePath(string mvPath);
    void setStoreAllChannelData(bool sc);
    void setCheckRecordsOnDaq(bool cr);

private:

    double sampleRate;	//Expected sample read which would be read from the "daq_config.txt"
    int secondsPerFile; //CM 8/7/15 - number of seconds of data in each file created(60)
	int blockSize;	//The number of sample data stored in one file from one channel
    int recordsPerWrite; //number of records to collect before writing to file/disk
    bool storeAllChannelData; //flag to specify if storing data from all channels even if no sensors connected
                                //  value 0: only process channels with sensors
                                //  value 1: process all channels
    bool checkRecordsOnDaq; //Flag to toggle for checking to see if the DAQ is filling up and records are not being pulled fast enough
                            //   value 0: do not check (print out info),
                            //   value 1: print out records currently on DAQ that have not been pulled

	string movePathStr;	//The path of the folder which is used to store the compressed HDF5 files
};
