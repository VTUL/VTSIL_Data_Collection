
#include "DAQ.h"

DAQ::DAQ()
{
	// Create a new instance of the VTEXDsa driver
	try
	{
        //initialize all variables
		driver = new IVTEXDsaPtr(__uuidof(VTEXDsa));
        channels = NULL;
		data = NULL;
		ts_sec = NULL;
		ts_frac = NULL;
		addl_data = NULL;
        numPhysicalChannels = 0;
        bufferSize = 60;
		daqID = "";

		bufferFull = false;
        numCollectedRecords = 0;
	}
	catch (...)
	{
		//Do something to intelligently deal with any other non-VTEX errors.

        //for now, just output an error message indicating where the error occured
        cout<<"Exception caught: DAQ class, constructor."<<endl;
	}
}

//deconstructor
DAQ::~DAQ()
{
    try
    {
        cout<<"DAQ deconstructor called"<<endl;

        close();
    }
    catch(...){
        //for now, just output an error message indicating where the error occured
        cout<<"Exception caught: DAQ class, deconstructor."<<endl;
    }
}

int DAQ::close()
{
    vector<Sensor_Data*>::iterator it;

    //clean up memory for the sensor data vector stored
    for(it = sensorData.begin(); it != sensorData.end(); it++)
    {
        if(*it != NULL)
        {
            delete *it;
            *it = NULL;
        }
    }
    sensorData.clear();

    driver->Measurement->Abort(); //CM 5/27/15 - stop measurment

    driver->StreamingData->DisableStreaming(); //CM - 12/2/14 - disable streaming

    driver->Close();

    ::CoUninitialize();

    return 0;
}

int DAQ::initialize(string ipAddress, int sampleRate, int recordSize, int clockFrequency, int prescaler, int bufferSize)
{
    try
    {
		daqID = ipAddress;
        _bstr_t resource_str(daqID.c_str());

		try
		{
			// Initialize the driver
            driver->Initialize(resource_str, VARIANT_FALSE, VARIANT_TRUE, "");
            //::SysFreeString(resource_str);

            //==========================================================================================
            //CM 6/8/15 - adding sync code for time and connecting to multiple DAQs in on driver session
            /*
            Note: To enable LAN Synchronization, all EMX-2500 controllers must be set to use PTP as the time source
            */

            // The ReferenceOscillator Source and TimestampSource must be set to 'System'
            driver->ReferenceOscillator->Source = VTEXDsaReferenceOscillatorSourceSystem;
            driver->ReferenceOscillator->TimestampSource = VTEXDsaReferenceOscillatorTimestampSourceSystem;

            // Set the CoordinationLine to "LAN,PXI0" and call SendSoftwareSync
            driver->Sync->CoordinationLine = "LAN,PXI0";
            driver->Sync->SendSoftwareSync();

            VARIANT_BOOL is_synchronized = driver->Sync->IsSynchronized;
            if(is_synchronized)
                cout << "LAN Synchronization Enabled" << endl;
            else
                cout << "There was an issue enabling LAN Synchronization" << endl;


			// "ALL" is a channel group which allows you to modify all of the channels at once
			/**/
			driver->Channels->Item["ALL"]->Enabled = true; //Enable the channels
			driver->Channels->Item["ALL"]->AutoRangeMode = VTEXDsaAutoRangeModeOff;
			driver->Channels->Item["ALL"]->Range = 2;
			driver->Channels->Item["ALL"]->Function = VTEXDsaChannelFunctionIEPE; // Set the channels to measure voltage
			driver->Channels->Item["ALL"]->Coupling = VTEXDsaCouplingAC; //CM 11/4/14 - set to AC
			driver->Channels->Item["ALL"]->IEPECurrent = 0.004; //CM 11/4/14 - signal using IEPE and set amp value
			/**/

            driver->Measurement->NumRecordsPerTrigger = 0; //CM 4/13/15 - set to 0 when performing continuous collection, otherwise, set to >= 1

            //set daq driver parameters based on what is in the config file           
            driver->Measurement->Sampling->RecordSize = recordSize;
            driver->Measurement->Sampling->ClockFrequency = clockFrequency;
            driver->Measurement->Sampling->Prescaler = prescaler;
			driver->Measurement->Sampling->SampleRate = sampleRate;
            //========================================================================

            //CM 5/21/15 - outputting the set-up of the system ======================
            //get the number of physical channels
            numPhysicalChannels = driver->Channels->GetNumChannels();
            double clockFreq = 0.0;
            long preScaler = 0.0;
            driver->Measurement->Sampling->get_ClockFrequency(&clockFreq);
            driver->Measurement->Sampling->get_Prescaler(&preScaler);
            this->sampleRate = driver->Measurement->Sampling->SampleRate;
			this->recordSize = recordSize;
			this->bufferSize = bufferSize;
            cout<<"==================================================================================="<<endl;
            cout<<"System set-up for DAQ: "<<daqID<<endl;
            cout<<"Clock Frequency: "<<clockFreq<<endl;
            cout<<"Prescaler: "<<preScaler<<endl;
			cout << "Actual sampling rate: " << this->sampleRate << " samples a second" << endl
                     <<"Record size: "<<recordSize<<endl
                     <<"Number of physical channels: " << numPhysicalChannels
                     <<endl<<endl
                     <<"Sampling every "<<recordSize/sampleRate<<" of a second ==> "<<endl
                     <<'\t'<<"Record size / sampling rate = "<<recordSize<<'/'<<sampleRate<<" = "<<recordSize/sampleRate<<" of a second"<<endl
                     <<"\tHence, recording Samply rate * number of channels = "<<sampleRate*numPhysicalChannels<<" samples every second"<<endl
                     <<endl;
            cout<<"==================================================================================="<<endl;
            //========================================================================

            driver->StreamingData->EnableStreaming(); //CM 12/2/14 - enable streaming

            // Initiate the measurement, i.e., get a measurement
            driver->Measurement->Initiate();
		}
		catch (_com_error& e)
		{
            //ouput what section of the error handling code the error was caught
            cout<<"Exception caught: DAQ class, initialize, VTEX error section."<<endl;

			if (e.Error() == E_IVI_INSTRUMENT_STATUS) // If the error is IVI_INSTRUMENT_STATUS, we need to check the error queue on the instrument
			{
				BSTR error = NULL;
				long error_code;
				driver->Utility->ErrorQuery(&error_code, &error);
                wcerr << "Error Code: " << error_code << endl;
                wcerr << error << endl;

				::SysFreeString(error);
			}
			else
			{
                wcerr << e.ErrorMessage() << endl;
			}
		}
	}
	catch (...)
	{
		//Do something to intelligently deal with any other non-VTEX errors.
        //for now, just output an error message indicating where the error occured
        cout<<"Exception caught: DAQ class, initialize."<<endl;
	}
	return 0;
}

void DAQ::getActiveChannels(vector<string>& channelIDs, vector<pair<int, string>>& activeChannels)
{
	// Read out all of the data from the FIFO with a timeout of 1000ms
	// Set the number of records to read to -1, which indicates that all records should be read, otherwise,
	//  specify how many records at a time you want, e.g., set to 1 currently to only process 1 record at a time
	//
	bool loop = true;
	
	while (loop)
	{
		driver->StreamingData->MemoryRead(1, &actual_records, &channels, &data, &ts_sec, &ts_frac, &addl_data); //CM - 12/1/14 - trying to get streamind data working

		//if have a record available
		if (actual_records > 0)
		{
			//if no records to grab then just continue
			if (data == NULL)
			{
				continue;
			}
			loop = false;

			//convert the BSTR of the channels to a form that is easily processed
			const char *p = const_cast<char *>(_com_util::ConvertBSTRToString(channels));
			//======================================================================================================
			//CM 7/10/15 - Removing use of QString for speed-up - conerting QString to string for writing
			//             to file is very costly.

			string channelsStr = string(p); //this is a string with channel IDs separated by commas
			stringstream ss(channelsStr);

			//now need to  parse the channel IDs that are deliminated by ','s
			while (ss.good())
			{
				string substr;
				getline(ss, substr, ',');
				channelIDs.push_back(substr); //add the channel ID to the vector
			}

			stringstream channelSS(channelsStr);
			int daqCardNo;
			int channelNo;
			char cardNo[8];
			string channelName;

			int daqIndex = 0;
			int lineNum = count(daqID.begin(), daqID.end(), '|');
			string *DaqName = new string[lineNum + 1];
			stringstream daqSS(daqID);

			string subStr;
			while (daqSS.good())
			{
				getline(daqSS, subStr, '|');
				stringstream subSS(subStr);
				string daqName;
				getline(subSS, daqName, '-');
				getline(subSS, daqName, '.');
				DaqName[daqIndex++] = daqName;
			}

			//======================================================================================================

			string currentChannel;
			int numChannelsWithNoSensor = 0;

			//iterate over the channels to find out which channel(s) have sensors
			for (int i = 0; i < numPhysicalChannels; i++)
			{
				currentChannel = channelIDs.at(i); //CM 7/10/15 - using string
				//if channel has no sensor
				if ((driver->Channels->Item[currentChannel.c_str()]->Overload->Status) & 0x20)
				{
					stringstream subSS(currentChannel);
					string channelID;
					getline(subSS, channelID, '!');
					daqCardNo = atoi(channelID.c_str());
					sprintf_s(cardNo, "%i", daqCardNo - daqCardNo / 100 * 100);
					getline(subSS, channelID, '!');
					channelID.replace(0, 2, "");
					channelNo = atoi(channelID.c_str());
					if (channelNo >= 10)
						channelName = DaqName[daqCardNo / 100] + "-" + cardNo + "_" + channelID;
					else
						channelName = DaqName[channelNo / 100] + "-" + cardNo + "_0" + channelID;
					channelIDs[i] = channelName;

					numChannelsWithNoSensor++;
				}
				//otherwise, channel has a sensor, so record which one
				else
				{
					stringstream subSS(currentChannel);
					string channelID;
					getline(subSS, channelID, '!');
					daqCardNo = atoi(channelID.c_str());
					sprintf_s(cardNo, "%i", daqCardNo - daqCardNo / 100 * 100);
					getline(subSS, channelID, '!');
					channelID.replace(0, 2, "");
					channelNo = atoi(channelID.c_str());
					if (channelNo >= 10)
						channelName = DaqName[daqCardNo / 100] + "-" + cardNo + "_" + channelID;
					else
						channelName = DaqName[daqCardNo / 100] + "-" + cardNo + "_0" + channelID;
					channelIDs[i] = channelName;
					//channelIDs.push_back(channelName); //add the channel ID to the vector

					pair<int, string> bundleStd = pair<int, string>(i, channelName); //change to using string
					activeChannels.push_back(bundleStd);
				}
			}
			delete[]DaqName;
			DaqName = NULL;

			cout << daqID << ":" << endl;
			cout << "Number of channels without sensors: " << numChannelsWithNoSensor << endl;
			cout << "=======================================================================" << endl;
		}
	}
}


//collect raw, unprocessed data from the DAQ, this is store in raw format until writing to file is performed
bool DAQ:: continuousRawData()
{
    bool loop = true; //CM 3/19/15
    numCollectedRecords = 0;

    try
    {
        while (loop)
        {
            //=======================================================================================================
            //CM 8/13/15 - test to see if a keystroke was pressed --> use this to elegantly close the program
            char c;
            if(_kbhit()){
               c = _getch();
               printf("You have pressed: %c\n", c);

               if(c == 'e'){
                   //close();
                   cout<<"Exiting..."<<endl;
                   loop = false;

                   //CM 8/17/15 - set the buffer full flag so whatever is in the buffer will get written out before exiting
               }
            }
            //=======================================================================================================

            // Read out all of the data from the FIFO with a timeout of 1000ms
            // Set the number of records to read to -1, which indicates that all records should be read, otherwise,
            //  specify how many records at a time you want, e.g., set to 1 currently to only process 1 record at a time
            //
            driver->StreamingData->MemoryRead(1, &actual_records, &channels, &data, &ts_sec, &ts_frac, &addl_data); //CM - 12/1/14 - trying to get streamind data working

            //if have a record available
            if(actual_records > 0)
            {
                //if no records to grab then just continue
                if(data == NULL)
                {
                    continue;
                }

                numCollectedRecords = numCollectedRecords + actual_records;//count the real number of records seen before a write to disk is performed

                 //CM 8/18/15 - print out the progress ========================
                 //if the first record, then output the progress title
                 if(numCollectedRecords == 1)
                 {
                     cout<<"Collecting for "<<bufferSize<<" seconds:"<<endl;
                     cout<<"\tProgress (in seconds): "<<endl;
                     cout<<"\t \t";
                 }

                 //if not the last record of the file, add a comma
                 if(numCollectedRecords <= bufferSize - 1){

                    //if not reached the end of a progress line (define width of number of record numbers printed out)
                    if((numCollectedRecords % PROGRESS_LINE_LENGTH) != 0){
                        cout<<numCollectedRecords<<',';
                    }
                    //otherwise, go to the next line
                    else{
                        cout<<numCollectedRecords<<','<<endl<<"\t \t";
                    }
                 }
                 //otherwise, leave off the comma and end the line
                 else{
                     //if not reached the end of a progress line (define width of number of record numbers printed out)
                     //     for the last record
                     if((numCollectedRecords % PROGRESS_LINE_LENGTH) != 0)
                     {
                         //cout<<endl<<"\t \t"<<numRecordsRecorded;
                         cout<<numCollectedRecords<<endl;
                     }
                     //otherwise, go to the next line and print the last record number
                     else
					 {
                        cout<<numCollectedRecords<<"\t \t"<<endl;
                     }
                 }
                 //============================================================

                 Sensor_Data* sData = new Sensor_Data(actual_records, data, ts_sec, ts_frac);
                 appendToData(sData);

                 sData = NULL;
                 ts_sec = NULL;
                 ts_frac = NULL;
                 data = NULL;

                //check to see if the buffer is full, if so, then exit collection loop so data can be written to disk
                if (bufferFull)
                {
					loop = false; //end collection loop to write to disk

                    //see if buffer on DAQ is filling up ===============
                    if(CHECK_RECORDS_ON_DAQ == 1)
                    {
                        //if records are on DAQ, print out how many
                        if(driver->StreamingData->NumMemoryRecords > 0)
						{
                            cout<<daqID<<": Current number of records on DAQ to pull: "<<driver->StreamingData->NumMemoryRecords<<endl;
                        }
                    }
					return true;
				}
            }
            this_thread::sleep_for(chrono::milliseconds(5)); //CM 5/8/15 - give system a rest
        }
    }
    catch (_com_error& e)
    {
        if (e.Error() == E_IVI_INSTRUMENT_STATUS) // If the error is IVI_INSTRUMENT_STATUS, we need to check the error queue on the instrument
        {
            //for now, just output an error message indicating where the error occured
            cout<<"Exception caught: DAQ class, continuousRawData, if section of VTEX error handling."<<endl;

            BSTR error = NULL;
            long error_code;
            driver->Utility->ErrorQuery(&error_code, &error);
            wcerr << "Error Code: " << error_code << endl;
            wcerr << error << endl;

            ::SysFreeString(error);
        }
        else
        {
            wcerr << e.ErrorMessage() << endl;
            //for now, just output an error message indicating where the error occured
            cout<<"Exception caught: DAQ class, continuouseRawData, else clause of VTEX error handling."<<endl;
        }
        driver->Close(); // If an exception occurs, try to close the driver session gracefully
    }

    return false; //CM 8/18/15 - when the collection loop is ended before a full collection cycle is reached,
                  //                 then trigger collection needs to end, i.e., return false to signal to stop
                  //                 collection (see DAQ_Interface->collectDaqData()

}

//CM 5/13/15 - try storing the safearray from the DAQ directly
void DAQ::appendToData(Sensor_Data *sd)
{
   sensorData.push_back(sd); //store the new sensor data object

   //CM 5/11/15 - see if buffer is full, if so, then trip flag
   if(numCollectedRecords >= bufferSize)
   {
       bufferFull = true;
   }
}


//write the data in each channel to the specified output object stored in the daq object
void DAQ::writeDataToGlobal(double* totalSensorData[], double* timeStamp, vector<pair<int, string>> activeChannels)
{
	size_t i = 0;
	try
	{
		int sensorDataCount = 0;
		double *currentSensorDataDouble = NULL; //CM 8/10/15 - move definition outside of sensor loop

		size_t sensorIt; //iterator through the list of sensor objects representing samples returned from the DAQ
		//Iterate through each sensor object and pull out the data
		size_t sensorDataSize = sensorData.size();
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
				::SafeArrayGetElement(currentSensorData->getTimeSeconds(), (LONG *)&timeIndex, timeStamp);
			}
			//===============================================================


			//CM 8/7/15 - update to use safe array access ===========================================================
			currentSensorDataDouble = NULL;
			HRESULT hr;

			// Get a pointer to the elements of the array.
			hr = ::SafeArrayAccessData(currentSensorData->getData(), (void**)&currentSensorDataDouble);

			if (FAILED(hr))
			{
				cout << "Error geting access to SAFEARRAY of sensor data. Aborting..." << endl;
				return;
			}
			//=======================================================================================================

			double d = 0.0; //hold each sample as the sensor data is iterated, once saved, this variable is
			//  set to the next sample from the sensor

			//iterate over all the available records
			int sensorDataNumRecords = currentSensorData->getNumRecords();
			for (int record = 0; record < sensorDataNumRecords; record++)
			{
				int channel = 0; //specify which channel data to get

				//Only pull daata from channels with sensors
				if (ALL_CHANNEL_DATA == 0){

					//iterate through all the active channels
					size_t activeChannelSize = activeChannels.size();
					for (size_t it = 0; it != activeChannelSize; it++)
					{
						//channel = activeChannels.at(specificChannel).first;
						channel = activeChannels.at(it).first;

						//iterate through each record, i.e., recordSize is the number of samples in a record,
						//  so if the record size is 20, then 20 samples are read for each channel for that specific record
						for (i = 0; i < recordSize; i++)
						{
							//index calculation: (the current channel * the size of a record) + the current index in the record
							size_t index = (channel * recordSize) + i;

							d = (double)currentSensorDataDouble[index]; //CM 8/7/15 - update to use safe access
							totalSensorData[it][sensorDataCount + i] = d;
						}
					}
				}
				//otherwise, pull samples from all channels, despite if they have sensors connected to them
				else
				{
					//iterate through all channels (active and inactive) and pull the data
					for (channel = 0; channel < numPhysicalChannels; channel++)
					{
						//iterate through each record, i.e., recordSize is the number of samples in a record,
						//  so if the record size is 20, then 20 samples are read for each channel for that specific record
						for (size_t i = 0; i < recordSize; i++)
						{
							//================================================================================================================
							//index calculation: (the current channel * the size of a record) + the current index in the record
							size_t index = (channel * recordSize) + i;//+(record*data_count);

							d = (double)currentSensorDataDouble[index]; //CM 8/7/15 - update to use safe access
							totalSensorData[channel][sensorDataCount + i] = d;
						}
					}
				}
				sensorDataCount += sensorDataNumRecords * recordSize;
			}
			::SafeArrayUnaccessData(currentSensorData->getData()); //CM 8/10/15 - close access to current safe array
			currentSensorData = NULL;
			//===========================================================================================================
		}
		currentSensorDataDouble = NULL;
	}
	// catch failure caused by the H5File operations
	catch (FileIException error)
	{
		error.printError();
	}

	// catch failure caused by the DataSet operations
	catch (DataSetIException error)
	{
		error.printError();
	}

	// catch failure caused by the DataSpace operations
	catch (DataSpaceIException error)
	{
		error.printError();
	}
	catch (...)
	{
		//for now, just output an error message indicating where the error occured
		cout << "Exception caught: HDF5_Output class, wrtieDaqDataToFile." << endl;
	}
}

//CM 3/18/15 - empty the buffer (data parameter)so new data can be collected
bool DAQ::clearBuffer() //empty the buffer (data parameter)so new data can be collected
{
    try
    {
        //CM 7/23/15 - free the memory for the sensor data objects =====================
        vector<Sensor_Data*>::iterator it;

        //clean up memory for the sensor data vector stored
        for(it = sensorData.begin(); it != sensorData.end(); it++)
        {
            if(*it != NULL)
            {
                delete *it;
                *it = NULL;
            }
        }
        sensorData.clear();//reserve the buffer size again since clear() reduces the size of the buffer to 0
        bufferFull = false; //CM 5/11/15 - reset buffer full flag
    }
    catch(...)
    {
        //for now, just output an error message indicating where the error occured
        cout<<"Exception caught: DAQ class, clear buffer."<<endl;
    }

    return true;
}
