#pragma once

#include <sstream>
#include <iostream>
#include <fstream>
#include <thread>         // this_thread::sleep_for
#include <chrono>
#include <conio.h> //CM 5/13/15 - include for kbhit to catch keyboard input without pausing the program
#include <vector>
#include <map>
#include <string>
#include <list>
#include <time.h>

#include <ctime>
#include <stdlib.h>
#include <stdio.h>

#include <mutex>

using namespace std;

#include <VTEXDsa.h>

//to provide support for safearrays
#import "IviDriverTypeLib.dll" no_namespace
#if _WIN64
#import "VTEXDsa_64.dll" no_namespace
#else
#import "VTEXDsa.dll" no_namespace
#endif

#include "H5Cpp.h"

#ifndef H5_NO_NAMESPACE
using namespace H5;
#endif

#ifndef TIMER_VARS
#define TIMER_VARS

extern time_t startTime,endTime; //Global timing variables for clocking user specified sections of code.
                                 //     NOTE: see note below about current usage.
extern int ALL_CHANNEL_DATA; //Pull all the samples on all channels despite whether or not a channel has a connected sensor
                             //   Value = 0: only pull sensor data from channels with sensors
                             //   Value = 1: pull all sensor data

extern int CHECK_RECORDS_ON_DAQ;    //Check to see if the DAQ is filling up and records are not being pulled fast enough
                                    //  Value = 0: do not show when records are being back-logged
                                    //  Value = 1: show when records are being back-logged

extern int PROGRESS_LINE_LENGTH; //CM 8/18/15 - during each collection phase, print out progress feedback (i.e., how much progress has passed)
                                 //              this variable sets the length of the progress bar (so no matter how long a collection cycle
                                 //              is, it will always take the same space. The longer the cycle, the slower the progress will show
extern int CHUNK_SIZE;	//Using this variable while compressing the HDF5 File

#endif

#define SAMPLE_RATE_NUM		75						//The line number in the RATE_MAP_FILE_NAME
#define RATE_MAP_FILE_NAME	"rate_map.csv"			//The file store the sample rate-clock frequency-prescaler mapping data

const double currentTimeSeconds(); //CM 4/14/15 - current timestamp in seconds
const string currentDateTime(); //CM 8/10/15 - current timestamp using standard string

//NOTE: I have not gotten the global start and end timers to work properly. I can get local ones to work just fine.
void startTimer(); //CM 5/18/15 - start global timer for timing how long operations take
double endTimer(); //CM 5/18/15 - end global timer for timing how long operations take

//to_bool function courteousy of Chris Jester-Young at: http://stackoverflow.com/questions/2165921/converting-from-a-stdstring-to-bool
inline bool to_bool(string const& s) {
    return s != "0";
}


