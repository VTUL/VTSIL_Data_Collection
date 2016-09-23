#pragma once

#include "Data_Output.h"

//This class writes sensor data to a CSV file and moves the file to a specified location once a file is complete.
//  The sensor data is split-up in mutiple files each a specific time length as specified in the config file.
//  Default length is 1 minute (60 seconds);
class CSV_Output : public Data_Output {

public:
    CSV_Output();
    ~CSV_Output();


    //CM 5/18/15 - pass necessary information to write sensor data to CSV file
    void writeDaqDataToFile(vector<Sensor_Data*>& sensorData, int recordSize, vector<string>& channelIDs, int numPhysicalChannels, vector<pair<int,string>>& activeChannels);

private:
    int outputCount;    //CM 3/18/15 - testing - keep track of the number of times a write to file is called

    int fileLength;  //CM 8/12/15 - The length of each file created in seconds
    bool headerWritten; //CM 5/19/15 - bool to  flag when the header is or is not written to the current CSV file
                        //  False: when not written, True: when written
                        //  This is used to detect when a new CSV file is created, then the header is written

    int numPoints; //used for debugging to see how many points are being written to a file

    string currentFileName;    //CM 4/14/15 - the current open file that is being written to

    //CM 7/9/15 - changing to use regular strings for speed-up, QString.toStdString() is too slow (by about 5x)
    string outputStdStr;
    string headerStdStr;

    //CM 8/12/15 - record the timestamp of the first record of each file
    double firstTimeSecond;
    double firstTimeFraction;

    int recordsCollected; //CM 8/6/15 - keep track of the number of records have been collected for the current file

};
