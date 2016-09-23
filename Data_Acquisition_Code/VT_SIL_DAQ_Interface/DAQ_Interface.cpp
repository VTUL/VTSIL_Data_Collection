
#include "DAQ_Interface.h"


//constructor
DAQ_Interface::DAQ_Interface()
{
    ::CoInitialize(NULL);

    try
	{
		runningProgram = true;
		preparedData = false;
		totalSensorData = NULL;

		configParser = new Config_Parser();
		output = new HDF5_Output();
		daqObject = NULL;
    }
    catch (...)
    {
        //Do something to intelligently deal with any other non-VTEX errors.

        //for now, just output an error message indicating where the error occured
        cout<<"Exception caught: DAQ Interface class, constructor."<<endl;
    }
}

//destructor
DAQ_Interface::~DAQ_Interface(){

    try
	{
		cout<<"Closing collection..."<<endl;
		close(); //CM 8/13/15 - close out the session for each DAQ - this releases any memory held by the DAQ(s)
    }
    catch(...)
	{
        //for now, just output an error message indicating where the error occured
        cout<<"Exception caught: DAQ Interface class, deconstructor."<<endl;
    }
}

int DAQ_Interface::loadConfigureFile()
{
	double srcSampleRate;
	//==== CM 6/1/15 - load the configuration file============================

	//CM 8/10/15 - update to use standard string
	configParser->parseConfigFileStr("daq_config.txt"); //Take the file name of the config file and parse it

	//CM 6/2/15 - grab the move path fromt he confog file and set the output move path to it
	movePath = configParser->getMovePath();
	output->setMoveDir(movePath);

	//store the record size and sample rate specified in the config file
	sampleRate = configParser->getSampleRate();
	srcSampleRate = sampleRate;
	recordSize = sampleRate; //CM 8/7/15 - each record will equal a second's worth of data (number of sample points per second)

	bufferSize = configParser->getSecondsPerFile(); //CM 8/7/15 - update buffer size based on how many seconds to be recorded per file.
	//              This will be the number of records to be collect per file.
	//              NOTE: THIS REMOVES HANDLING THE RECORD SIZE FROM THE USER

	blockSize = configParser->getBlockSize(); //get the default size of sample data stored in one file from one channel

	//set global variables used as flags (see library.h for definitions ======
	ALL_CHANNEL_DATA = configParser->getStoreAllChannelData();
	CHECK_RECORDS_ON_DAQ = configParser->getCheckRecordsOnDaq();

	ifstream file(RATE_MAP_FILE_NAME); // declare file stream to read the map csv file
	string value;
	string number;
	int index, count = 0, i = 0;
	double sampleRateArray[SAMPLE_RATE_NUM];//store the list of sample rate
	int frequencyArray[SAMPLE_RATE_NUM];//store the list of corresponding clock frequency
	int scalerArray[SAMPLE_RATE_NUM];//store the list of corresponding scaler
	while (getline(file, value))// read a string from one line of a csv file
	{
		istringstream is(value);
		if (i++ == 0) continue;
		getline(is, number, ',');
		sampleRateArray[count] = atof(number.c_str());
		getline(is, number, ',');
		frequencyArray[count] = atoi(number.c_str());
		getline(is, number, ',');
		getline(is, number, ',');
		scalerArray[count] = atoi(number.c_str());
		count++;
	}
	file.close();

	for (index = 0; index < count - 1; index++)
	{
		if (sampleRate >= sampleRateArray[0]) break;
		else if (sampleRate <= sampleRateArray[index] && sampleRate > sampleRateArray[index + 1]) break;
	}

	sampleRate = sampleRateArray[index];
	clockFrequency = frequencyArray[index];
	prescaler = scalerArray[index];
	//========================================================================

	//display the config info parsed from the config file
	cout << "Parameters from the config file:"
		<< endl
		<< "Clock Frequency: " << clockFrequency << endl
		<< "Prescaler: " << prescaler << endl
		<< "Sample Rate: " << sampleRate << endl
		<< "Length of file (in seconds): " << bufferSize << endl
		<< "Move Path: " << movePath << endl
		<< "Store all sensor data: " << ALL_CHANNEL_DATA << endl
		<< "Check records on DAQs: " << CHECK_RECORDS_ON_DAQ << endl
		<< endl;

	//converting current time to date, hour, minute and second
	time_t t = time(0);
	// convert now to string form
	struct tm timeinfo;
	localtime_s(&timeinfo, &t);

	char buf[80];
	strftime(buf, sizeof(buf), "%Y-%m-%d_%H_%M_%S", &timeinfo); //format timestamp of first data point in the file to a date/time

	string currentFileName = string(buf);       //create a string out of the formated timstamp
	currentFileName = movePath + currentFileName + "_config.txt"; //add file extention

	//Write a new file to store the expected sample rate, real sample rate, clock frequency and prescaler
	ofstream out(currentFileName.c_str());
	out << "expected sample rate = " << srcSampleRate << "\n";
	out << "real sample rate = " << sampleRate << "\n";
	out << "clock frequency = " << clockFrequency << "\n";
	out << "prescaler = " << prescaler << "\n";
	out.close();

	return 0;
}

//take in all the newtwork IDs of DAQs, initialize them all, then start collecting
void DAQ_Interface::daqBootStrap(string ipAddresses)
{
	loadConfigureFile();// load all the data stored in the configuration file
	initialize(ipAddresses); //initialize all the DAQs

	//Allocating a memory buffer for the Sensor Data in one DAQ
	//The number of Channels with sensors
	if (ALL_CHANNEL_DATA == 0)
		totalDataRow = activeChannels.size();
	else
		totalDataRow = channelIDs.size();

	totalDataCol = bufferSize * recordSize;//The number of total Samples in one session
	//Storing the Sensor Data in a 2-dimentional array, allocating the memory space
	totalSensorData = new double*[totalDataRow];
	for (int i = 0; i < totalDataRow; i++)
	{
		totalSensorData[i] = new double[totalDataCol];
		memset(totalSensorData[i], 0, sizeof(double)* totalDataCol);
	}

	startTimer(); //start timer for testing how long collection takes

	thread threadCollectData(&DAQ_Interface::collectDaqData, this);
	thread threadWriteFile(&DAQ_Interface::writeDataToFile, this);

	threadCollectData.join();
	threadWriteFile.join();
}

//iterate through all the DAQ addresses (daq IDs which are hostnames, e.g., daq1.local)
//  and initialize them
int DAQ_Interface::initialize(string ipAddresses)
{
	daqObject = new DAQ(); //create daq object with the default buffer size , i.e., number of records to grab before flushing for next collection sequence
	daqObject->initialize(ipAddresses, sampleRate, recordSize, clockFrequency, prescaler, bufferSize); //initialize with the given network ID (IP address)
	daqObject->getActiveChannels(channelIDs, activeChannels);//Get All the Channel IDs and Active Channel IDs

    return 0;
}

//call data collection for all the DAQs
void DAQ_Interface::collectDaqData()
{
    //CM 3/18/15 - adding wile loop for continuous data collection,
    //  collect until the buffer is full, then write to file and repeat
	while (runningProgram)
	{
		runningProgram = daqObject->continuousRawData(); //CM 4/13/15 - testing writing all data unprocessed to file (i.e., not broken up into channels)

		mutex.lock();//Using mutex to lock the thread
		if (!preparedData)
		{
			//CM 8/18/15 - once collected data from the DAQ(s), write out to file =======================
			daqObject->writeDataToGlobal(totalSensorData, &timeStamp, activeChannels); //CM 5/27/15 - change to each DAQ object using their own output object (this allows each
			//    DAQ to have a file open for writing its data)
			cout << "Collect Data Finished" << endl;
			preparedData = true;
		}
		mutex.unlock();
		daqObject->clearBuffer(); //CM 3/18/15 - clear the buffer for this channel after writing to file (or wherever the data is being sent)
    }
}

void DAQ_Interface::writeDataToFile()
{
	while (runningProgram)
	{
		mutex.lock();//Using mutex to lock the thread

		if (preparedData)
		{
			output->writeDaqDataToFile(totalSensorData, totalDataRow, totalDataCol, timeStamp, channelIDs, activeChannels);
			cout << "Write Data Finished" << endl;
			preparedData = false;
		}

		mutex.unlock();
		Sleep(5000);
	}
}

int DAQ_Interface::close()
{
	if (configParser != NULL) delete configParser;
	if (daqObject != NULL) delete daqObject;
	if (output != NULL) delete output;

	channelIDs.clear();
	activeChannels.clear();

	for (int i = 0; i < totalDataRow; i++)
	{
		if (totalSensorData[i] != NULL)
		{
			delete []totalSensorData[i];
			totalSensorData[i] = NULL;
		}
	}
	if (totalSensorData != NULL)
	{
		totalSensorData;
		totalSensorData = NULL;
	}
    return 0;
}
