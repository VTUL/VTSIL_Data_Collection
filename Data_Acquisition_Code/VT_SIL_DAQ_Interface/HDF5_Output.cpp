
#include "HDF5_Output.h"

HDF5_Output::HDF5_Output()
{
}

//deconstructor
HDF5_Output::~HDF5_Output()
{
}

//get the move directory
string HDF5_Output::getMoveDir(){
	return moveDir;
}

//set the move directory
void HDF5_Output::setMoveDir(string mvDir){
	moveDir = mvDir;
}

//Write sensor data to file
//CM 8/20/15 - use Sensor_Data class to store data and unpack here for writing to file
/*Parameters:
 *  sensorData:         Data returned from the DAQ. This is a vector of Sensor_Data object pointers.
 *  recordSize:         The number of samples in each record
 *  channelIDs:         Vector of IDs for each channel, format: 2!CH7 --> card in slot 2, channel 7 on that card
 *  numPhysicalChannels The number of physical channels on the DAQ
 *  activeChannels:     List of channels that have sensors connected to them. First parameter is the index of the channel in the list of channels on the DAQ.
 *                          The second parameter is the channel ID
 */
void HDF5_Output::writeDaqDataToFile(double* totalSensorData[], int totalDataRow, int totalDataCol, int timeStamp, vector<string>& channelIDs, vector<pair<int, string>>& activeChannels)
{
    size_t i = 0;

    try
    {
		//CM 7/15/15 - testing time required for iterating and storing data into a string ============
		clock_t start = clock();
		double duration = 0.0;

        //CM 8/12/15 - converting timestamp from DAQs to current date and time =============
        time_t t = timeStamp;
        // convert now to string form
        struct tm timeinfo;
		localtime_s(&timeinfo, &t);

        char buf[80];
        // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
        // for more information about date/time format
        strftime(buf, sizeof(buf), "%Y-%m-%d_%H_%M_%S", &timeinfo); //format timestamp of first data point in the file to a date/time

        currentFileName = string(buf);       //create a string out of the formated timstamp
        currentFileName = moveDir + currentFileName + ".h5"; //add file extention
        // Create a new file using the default property lists.
        H5File outfile(currentFileName.c_str(), H5F_ACC_TRUNC);
        Group* group = new Group(outfile.createGroup("/Data"));

		hsize_t dimRow = sqrt(totalDataCol);
		hsize_t halfDataCol = totalDataCol / 2;
		while (dimRow < halfDataCol)
		{
			if (totalDataCol % dimRow == 0)
			{
				break;
			}
			dimRow++;
		}
        hsize_t dimCol = totalDataCol / dimRow;
        hsize_t dims[2] = {dimRow, dimCol};	// dataset dimensions
		hsize_t chunk_dims[2] = {CHUNK_SIZE, CHUNK_SIZE};	// chunk dimensions

        string currentChannelID;
        string datasetName;

        DataSpace *dataspace;
        DSetCreatPropList  *plist;
        DataSet *dataset;
        for(i = 0; i < totalDataRow; i++)
        {
            // Create the data space for the dataset.
            dataspace = new DataSpace(2, dims);

            // Modify dataset creation property to enable chunking
            plist = new DSetCreatPropList;
            plist->setChunk(2, chunk_dims);

            // Set ZLIB (DEFLATE) Compression using level 6.
            plist->setDeflate(6);

            if(ALL_CHANNEL_DATA == 0)
                currentChannelID = activeChannels.at(i).second;
            else
                currentChannelID = channelIDs.at(i);

            datasetName = "/Data/Data_" + currentChannelID;

            // Create the dataset.
			dataset = new DataSet(outfile.createDataSet(datasetName.c_str(), PredType::NATIVE_DOUBLE, *dataspace, *plist));

            // Write data to dataset.
            dataset->write(totalSensorData[i], PredType::NATIVE_DOUBLE);

            // Close objects and file.  Either approach will close the HDF5 item.
            delete dataspace;
            delete dataset;
            delete plist;
        }
        delete group;
        outfile.close();

		//CM 7/15/15 - testing time required for iterating and storing data into a string ============
		duration = (clock() - start) / (double)CLOCKS_PER_SEC;
		cout << endl;
		cout << "Time to iterate over data and write sensor data into HDF5 file: " << duration << "s" << endl;
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
       cout<<"Exception caught: HDF5_Output class, wrtieDaqDataToFile."<<endl;
    }
}


