
#include "CSV_Output.h"

CSV_Output::CSV_Output()
{
	headerWritten = false; //CM 5/19/15

	fileLength = 60; //CM 4/14/15 - this is in seconds, start creating a new file every 1 minute (1 minute = 60 seconds)
	numPoints = 0;

	outputStdStr = "";
	headerStdStr = "";

	recordsCollected = 0;
}

//deconstructor
CSV_Output::~CSV_Output()
{
}

//Write sensor data to file
//CM 8/20/15 - use Sensor_Data class to store data and unpack here for writing to file
/*Parameters:
*  sensorData:         Data returned from the DAQ. This is a vector of Sensor_Data object pointers.
*  recordSize:         The number of samples in each record
*  sampleRate:         The number of samples per second
*  channelIDs:         Vector of IDs for each channel, format: 2!CH7 --> card in slot 2, channel 7 on that card
*  numPhysicalChannels The number of physical channels on the DAQ
*  activeChannels:     List of channels that have sensors connected to them. First parameter is the index of the channel in the list of channels on the DAQ.
*                          The second parameter is the channel ID
*/
void CSV_Output::writeDaqDataToFile(vector<Sensor_Data*>& sensorData, int recordSize, vector<string>& channelIDs, int numPhysicalChannels, vector<pair<int, string>>& activeChannels)
{
	recordsCollected = 0; //CM 8/20/15 - reset when writing a new file. This is used to see how many records are actually beeing written to a file.
	//        If this is less than the specified file length in the config file, using this variable will accurately say how long
	//        a file actually is.

	int currentWriteTime = int(currentTimeSeconds()); //CM 4/14/15 - get the current (now) time for comparison to the last write time

	//CM 7/15/15 - testing time required for iterating and storing data into a string ============
	clock_t start;
	start = clock();
	double duration = 0.0;
	//============================================================================================

	try
	{
		double *currentSensorDataDouble = NULL; //CM 8/10/15 - move definition outside of sensor loop

		size_t sensorIt; //iterator through the list of sensor objects representing samples returned from the DAQ
		size_t sensorDataSize = sensorData.size();
		//Iterate through each sensor object and pull out the data
		for (sensorIt = 0; sensorIt != sensorDataSize; sensorIt++)
		{
			Sensor_Data* currentSensorData = sensorData.at(sensorIt); //grab the current sensor data object

			//===============================================================
			//CM 8/12/15 - record the timestamp of the first record
			//If at the first sensor data entry then grab the timestamp
			if (sensorIt == 0)
			{

				//CM 5/27/15 - since the timestamp is the same for each channel for each record (and only processing one record at a time),
				//      Then just grab the timestamp of the first entry, i.e., index = 0
				int timeIndex = 0;
				::SafeArrayGetElement(currentSensorData->getTimeSeconds(), (LONG *)&timeIndex, &firstTimeSecond);
				::SafeArrayGetElement(currentSensorData->getTimeFraction(), (LONG *)&timeIndex, &firstTimeFraction);
			}
			//===============================================================

			//CM 8/7/15 - update to use safe array access ===========================================================
			currentSensorDataDouble = NULL;
			HRESULT hr;

			// Get a pointer to the elements of the array.
			hr = ::SafeArrayAccessData(currentSensorData->getData(), (void**)&currentSensorDataDouble);

			if (FAILED(hr)){
				cout << "Error geting access to SAFEARRAY of sensor data. Aborting..." << endl;
				return;
			}

			double d = 0.0; //hold each sample as the sensor data is iterated, once saved, this variable is
			//  set to the next sample from the sensor

			//Now to iterate through the sensor data. This is completed by "jumping" through the linear stream of data
			//  provided by the DAQ for each channel for each record. Even though I am not accessing it in linear fashion,
			//  I only see one sample once, which leads to linear performance.
			//  Format of linear stream:
			//      For record of size 4 and 3 channels:
			//              ------------------------------ record 1 ------------------------------ ----- record 2----------
			//              [[channel 1 ; 4 samples][channel 2 ; 4 samples][channel 3 ; 4 samples][channel 1 ; 4 samples]...]

			//iterate over all the available records
			int sensorDataNumRecords = currentSensorData->getNumRecords();
			for (int record = 0; record < sensorDataNumRecords; record++)
			{
				recordsCollected++; //CM 8/6/15 - record each record written out

				//iterate through each record, i.e., recordSize is the number of samples in a record,
				//  so if the record size is 20, then 20 samples are read for each channel for that specific record
				for (size_t i = 0; i < recordSize; i++)
				{
					//Only pull daata from channels with sensors
					if (ALL_CHANNEL_DATA == 0)
					{
						int channel = 0; //specify which channel data to get
						//iterate through all the active channels

						size_t activeChannelSize = activeChannels.size();
						//for(int specificChannel = 0; specificChannel < activeChannels.size();specificChannel++){
						for (size_t it = 0; it != activeChannelSize; it++)
						{
							//channel = activeChannels.at(specificChannel).first;
							channel = activeChannels.at(it).first;

							//====== CM 5/19/15 - if no header for the current file, create it on the first pass of the channel info =========
							if (headerWritten == false)
							{
								string currentChannelID = channelIDs.at(channel);
								//if the last channel, leave off the comma
								//CM 8/7/15 - if the last channel, then leave off the comma - check the next iterator to see if at the end
								if (it == activeChannelSize - 1)
								{
									string temp = "Data_" + currentChannelID + '\n'; //CM 8/7/15 - remove the timestamp from the header
									headerStdStr.append(temp);
									temp.clear();

									headerWritten = true; //header is written, do not write one again until the next CSV file
								}
								//otherwise, put the comma on since more channels are to come
								else
								{
									headerStdStr.append("Data_" + currentChannelID + ','); //CM 8/7/15 - remove the timestamp from the header
								}
							}

							//index calculation: (the current channel * the size of a record) + the current index in the record
							size_t index = (channel*recordSize) + i;

							d = (double)currentSensorDataDouble[index]; //CM 8/7/15 - update to use safe access

							//add current sample to string to be written to file
							//CM 5/19/15 - if at the last channel, do not add a comma
							//CM 8/7/15 - if the last channel, then leave off the comma - check the next iterator to see if at the end
							if (it == activeChannelSize - 1)
							{
								outputStdStr.append(to_string(d)); //CM 7/10/15 - using string
							}
							//Otherwise, still more channels to go, so add a comma
							else
							{
								outputStdStr.append(to_string(d) + ','); //CM 7/10/15 - using string
							}
						}
					}
					//otherwise, pull samples from all channels, despite if they have sensors connected to them
					else
					{
						//iterate through all channels (active and inactive) and pull the data
						for (int channel = 0; channel < numPhysicalChannels; channel++)
						{
							//====== CM 5/19/15 - if no header for the current file, create it on the first pass of the channel info =========
							if (headerWritten == false){
								//if the last channel, leave off the comma
								string currentChannelID = channelIDs.at(channel); //CM 7/10/15 - using string
								if (channel == numPhysicalChannels - 1)
								{
									headerStdStr.append("Data_" + currentChannelID + '\n'); //CM 8/7/15 - remove the timestamp from the header
									headerWritten = true; //header is written, do not write one again until the next CSV file
								}
								//otherwise, put the comma on since more channels are to come
								else
								{
									headerStdStr.append("Data_" + currentChannelID + ','); //CM 8/7/15 - remove the timestamp from the header
								}
							}
							//================================================================================================================
							//index calculation: (the current channel * the size of a record) + the current index in the record
							size_t index = (channel*recordSize) + i;//+(record*data_count);

							d = (double)currentSensorDataDouble[index]; //CM 8/7/15 - update to use safe access

							//CM 5/19/15 - if at the last channel, do not add a comma
							if (channel == numPhysicalChannels - 1)
							{
								outputStdStr.append(to_string(d)); //CM 7/10/15 - using string
							}
							//Otherwise, still more channels to go, so add a comma
							else
							{
								outputStdStr.append(to_string(d) + ','); //CM 7/10/15 - using string
							}
						}
					}
					outputStdStr.append("\n"); //signal end of line once an entry in a record for all channels is recorded
				}
			}
			::SafeArrayUnaccessData(currentSensorData->getData()); //CM 8/10/15 - close access to current safe array
			//===========================================================================================================
		}
	}
	catch (...)
	{
		//for now, just output an error message indicating where the error occured
		cout << "Exception caught: CSV_Output class, wrtieDaqDataToFile." << endl;
	}

	//========================================================================================
	// CM 8/18/15 - writing to file after iterating over data so writing can occur after parsing data since I changed my logic to collect all
	//  the records for one file in one collection sequence in DAQ.cpp
	//  There "should" not be a problem with this since I collect the same number of records in one collection sequence
	//  (before coming to this function) as the fileLength, hence recordsCollected "should" always equal fileLength
	//  when control reaches here.

	//CM 8/12/15 - converting timestamp from DAQs to current date and time =============
	time_t t = firstTimeSecond;

	// convert now to string form
	struct tm timeinfo;
	localtime_s(&timeinfo, &t);

	char buf[80];

	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d_%H_%M_%S", &timeinfo); //format timestamp of first data point in the file to a date/time
	//==================================================================================

	currentFileName = string(buf);       //create a string out of the formated timstamp
	currentFileName = moveDir + currentFileName + ".csv"; //add file extention
	ofstream Out(currentFileName, ios::app); //CM 8/12/15 - Now open a file with the name being the timestampe of the first data point

	//====================================================================================================================
	//CM 8/12/15 - add header to store information necessary to calculate the exact time of each sample in the file

	//NOTE: use to_string() to retain the accuracy of the timestamp info, i.e., all significant figures are retained

	//header for file info: Time info for first sample in the file, sample rate, and length of the file in seconds
	Out << "Time Integer, Time Fraction, Sample Rate (samples per second), Length of file in seconds\n";

	//output the file info
	Out << to_string(firstTimeSecond) << ',' << to_string(firstTimeFraction) << ',' << recordSize << ',' << recordsCollected << '\n';

	//header to describe what the data in the file is
	Out << "Sensor Data organized by CSV columns. Each column represents the data from one channel on a specific DAQ\n";

	//====================================================================================================================

	Out << headerStdStr;
	Out << outputStdStr; //CM 5/27/15 - write the entire set of record(s) info to the file at once (i.e., only one file I/O)

	headerStdStr.clear();
	outputStdStr.clear();
	Out.flush(); //CM 7/23/15
	Out.close();

	//CM 7/15/15 - testing time required for iterating and storing data into a string ============
	duration = (clock() - start) / (double)CLOCKS_PER_SEC;
	cout << "Time to iterate over data and store sensor data into CSV file: " << duration << "s" << endl;

	headerWritten = false; //CM 5/19/15 - with the new file, signal the header needs to be written

	numPoints = 0;
}


