#pragma once

#include "sensor_data.h"


//This class writes sensor data to a HDF5 file and moves the file to a specified location once a file is complete.
//  The sensor data is split-up in mutiple files each a specific time length as specified in the config file.
//  Default length is 1 minute (60 seconds);
class HDF5_Output
{
public:
    HDF5_Output();
    ~HDF5_Output();

    //CM 5/18/15 - pass necessary information to write sensor data to CSV file
	void writeDaqDataToFile(double* totalSensorData[], int totalDataRow, int totalDataCol, int timestamp, vector<string>& channelIDs, vector<pair<int, string>>& activeChannels);

	string getMoveDir(); //get the move directory
	void setMoveDir(string mvDir); //set the move directory
	
private:
    string currentFileName;    //CM 4/14/15 - the current open file that is being written to

	string moveDir;            //CM 4/16/15 - the directory to move output 
};
