#include "Areas/Timing/TimingController.h"
#include "include/NVMainRequest.h"
#include "include/NVMDataBlock.h"
#include <functional>
#include "include/NVMAddress.h"
#include <string>
#include <tuple>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <cstring>
#include <bitset>
#include <algorithm>
#include "Areas/interface-config.h"

using namespace NVM;

TimingController::TimingController( ){ };

TimingController::~TimingController( ){ };

void TimingController::UpdateTimingMap(std::string location, int mode){
    std::tuple<tiValue,tiValue,tiValue> timingValues = (writeModi.find(mode))->second;

    /* find the greatest location strictly less than the given location */
    auto it = timingMap.lower_bound(location);
    if (it != timingMap.begin() && location != it->first) {
        --it;
    }

    /* update the timing values with new write mode */
    timingMap[it->first] = timingValues;
    std::cout << "-- Updating Timing Area " << it->first << " with: " << "{" << std::get<0>( it->second) << ", " << std::get<1>( it->second) << ", " << std::get<2>( it->second) << "}" << std::endl;
}

void TimingController::SetWriteModi(){

    writeModi[0] = { INTERFACE_STANDARD_READTIME, INTERFACE_MODE_0_SETTIME, INTERFACE_MODE_0_RESETTIME};
    writeModi[1] = { INTERFACE_STANDARD_READTIME, INTERFACE_MODE_1_SETTIME, INTERFACE_MODE_1_RESETTIME};
    writeModi[2] = { INTERFACE_STANDARD_READTIME, INTERFACE_MODE_2_SETTIME, INTERFACE_MODE_2_RESETTIME};
    writeModi[3] = { INTERFACE_STANDARD_READTIME, INTERFACE_MODE_3_SETTIME, INTERFACE_MODE_3_RESETTIME};

    std::tuple<tiValue,tiValue,tiValue> modeValues;

    std::cout << "Configured timing write modi to: " << std::endl;

    for (auto it = writeModi.begin(); it != writeModi.end(); ++it){
        modeValues = it->second;
        std::cout << "- " << it->first << " : {" << std::get<0>(modeValues) << ", " << std::get<1>(modeValues) << ", " << std::get<2>(modeValues) << "}" << std::endl;
    }

}

tiValue TimingController::GetTimingValue(NVMainRequest *request){ 
    
    std::string location;
    int channel = request->address.GetChannel();
    int rank = request->address.GetRank();
    int bank = request->address.GetBank();
    int subarray = request->address.GetSubArray();
    int row = request->address.GetRow();
    int column = request->address.GetCol();
    location = std::to_string(channel) + " " + std::to_string(rank) + " " + std::to_string(bank) + " " + std::to_string(subarray) + " " + std::to_string(row) + " " + std::to_string(column);


    /* find the greatest location strictly less than the given location */
    auto it = timingMap.lower_bound(location);
    std::tuple<tiValue,tiValue,tiValue> timingValues;
    if (it != timingMap.begin() && location != it->first) {
        --it;
    }

    /* extract timing values for this address*/
    timingValues = it->second;

    /* return time based on operation type */
    if(request->type == READ){
        return (std::get<0>(timingValues));
    }
    else if (request->type == WRITE){
        return DetermineWritingTime(std::get<1>(timingValues), std::get<2>(timingValues), request);
    }
    else   
        return 0.0;
}


tiValue TimingController::DetermineWritingTime(tiValue setTime, tiValue resetTime, NVMainRequest *request){
    NVMDataBlock &data = request->data;
    uint64_t size = data.GetSize();
    uint8_t dataByte;
    bool oneWritten = false;
    bool zeroWritten = false;
    std::string binary;

    
    /* check if 0 or 1 or both is written*/
    for (int i = 0; i < size; i++){
        dataByte = data.GetByte(i);
        binary = std::bitset<8>((int) dataByte).to_string();
        /* check if 1s and 0s are written*/
        std::string::difference_type n = std::count(binary.begin(), binary.end(), '1');
        if ( (int) n == 0){
            zeroWritten = true;
        } else if ( (int) n == 8){
            oneWritten = true;
        } else {
            oneWritten = zeroWritten = true;
            /* both are written, max value can be returned */
            break;
        }
    }

    /* return max time based on written values */
    if (oneWritten && zeroWritten){
        return std::max(setTime, resetTime);
    } else if (oneWritten) {
        return setTime;
    } else {
        return resetTime;
    }

}


/* reads the timing config and saves the timing values and address in the timingMap */
void TimingController::ReadConfig( std::string filename ){
    std::string line;
    std::ifstream configFile( filename.c_str( ) );
    std::string subline;

    int channel;
    int rank;
    int bank;
    int subarray;
    int row;
    int column;
    tiValue readTime;
    tiValue setTime;
    tiValue resetTime;
    std::tuple<tiValue,tiValue,tiValue> timingValues;
    std::string location = "0 0 0 0 0 0";

    if( configFile.is_open( ) ) 
    {
        while( !configFile.eof( ) ) 
        {
            getline( configFile, line );

            /* Ignore blank lines and comments beginning with ';'. */

            /* find the first character that is not space, tab, return */
            size_t cPos = line.find_first_not_of( " \t\r\n" );

            /* if not found, the line is empty. just skip it */
            if( cPos == std::string::npos )
                continue;
            /*
             * else, check whether the first character is the comment flag. 
             * if so, skip it 
             */
            else if( line[cPos] == ';' )
                continue;
            /* else, remove the redundant white space and the possible comments */
            else
            {
                /* find the position of the first ';' */
                size_t colonPos = line.find_first_of( ";" );

                /* if there is no ';', extract all */
                if( colonPos == std::string::npos )
                {
                    subline = line.substr( cPos );
                }
                else
                {
                    /* colonPos must be larger than cPos */
                    assert( colonPos > cPos );

                    /* extract the useful message from the line */
                    subline = line.substr( cPos, (colonPos - cPos) );
                }
            }

            /* parse the parameters and values */
            char *cline;
            cline = new char[subline.size( ) + 1];
            strcpy( cline, subline.c_str( ) );
            
            char *tokens = strtok( cline, " " );
            
            /* extract physical location */
            channel = stoi(std::string( tokens ));
            tokens = strtok( NULL, " " );
            rank = stoi(std::string( tokens ));
            tokens = strtok( NULL, " " );
            bank = stoi(std::string( tokens ));
            tokens = strtok( NULL, " " );
            subarray = stoi(std::string( tokens ));
            tokens = strtok( NULL, " " );
            row = stoi(std::string( tokens ));
            tokens = strtok( NULL, " " );
            column = stoi(std::string( tokens ));

            /* extract timing values */
            tokens = strtok( NULL, " " );
            readTime = stod(std::string( tokens )); 
            tokens = strtok( NULL, " " );
            setTime = stod(std::string( tokens )); 
            tokens = strtok( NULL, " " );
            resetTime = stod(std::string( tokens )); 
            timingValues = {readTime, setTime, resetTime};

            location = std::to_string(channel) + " " + std::to_string(rank) + " " + std::to_string(bank) + " " + std::to_string(subarray) + " " + std::to_string(row) + " " + std::to_string(column);
            
            /* save timing values with the corresponding location */
            timingMap[location] = timingValues;

        }

        SetWriteModi();

        /* if 0x0 is not defined add to map*/
        location = "0 0 0 0 0 0";
        if (timingMap.count(location) == 0){
            timingMap[location] = {0,0,0};
            std::cout << "Warning: Definition of memory areas does not include 0x0. Using 0 as default value." << std::endl;
        }
    }
    else
    {
        std::cout << "NVMain: Could not read energy configuration file: " 
            << filename << std::endl;
        exit(1);
    }
    std::cout << "Defined " << timingMap.size() << " timing areas." << std::endl;
}

/* reads the areas config and saves the energy values and address in the energyMap, as well as the timing values in the timingMap*/
void TimingController::ReadJoinedConfig( std::string filename){
    std::string line;
    std::ifstream configFile( filename.c_str( ) );
    std::string subline;

    int channel;
    int rank;
    int bank;
    int subarray;
    int row;
    int column;
    tiValue readTime;
    tiValue setTime;
    tiValue resetTime;
    std::tuple<tiValue,tiValue,tiValue> timingValues;
    std::string location = "0 0 0 0 0 0";

    if( configFile.is_open( ) ) 
    {
        while( !configFile.eof( ) ) 
        {
            getline( configFile, line );

            /* Ignore blank lines and comments beginning with ';'. */

            /* find the first character that is not space, tab, return */
            size_t cPos = line.find_first_not_of( " \t\r\n" );

            /* if not found, the line is empty. just skip it */
            if( cPos == std::string::npos )
                continue;
            /*
             * else, check whether the first character is the comment flag. 
             * if so, skip it 
             */
            else if( line[cPos] == ';' )
                continue;
            /* else, remove the redundant white space and the possible comments */
            else
            {
                /* find the position of the first ';' */
                size_t colonPos = line.find_first_of( ";" );

                /* if there is no ';', extract all */
                if( colonPos == std::string::npos )
                {
                    subline = line.substr( cPos );
                }
                else
                {
                    /* colonPos must be larger than cPos */
                    assert( colonPos > cPos );

                    /* extract the useful message from the line */
                    subline = line.substr( cPos, (colonPos - cPos) );
                }
            }

            /* parse the parameters and values */
            char *cline;
            cline = new char[subline.size( ) + 1];
            strcpy( cline, subline.c_str( ) );
            
            char *tokens = strtok( cline, " " );
            
            /* extract physical location */
            channel = stoi(std::string( tokens ));
            tokens = strtok( NULL, " " );
            rank = stoi(std::string( tokens ));
            tokens = strtok( NULL, " " );
            bank = stoi(std::string( tokens ));
            tokens = strtok( NULL, " " );
            subarray = stoi(std::string( tokens ));
            tokens = strtok( NULL, " " );
            row = stoi(std::string( tokens ));
            tokens = strtok( NULL, " " );
            column = stoi(std::string( tokens ));

            /* extract energy and time values */
            tokens = strtok( NULL, " " );
            /* skip energy value */
            tokens = strtok( NULL, " " );
            readTime = stod(std::string( tokens )); 
            tokens = strtok( NULL, " " );
            /* skip energy value */
            tokens = strtok( NULL, " " );
            setTime = stod(std::string( tokens ));
            tokens = strtok( NULL, " " );
            /* skip energy value */ 
            tokens = strtok( NULL, " " );
            resetTime = stod(std::string( tokens ));
            timingValues = {readTime, setTime, resetTime};

            location = std::to_string(channel) + " " + std::to_string(rank) + " " + std::to_string(bank) + " " + std::to_string(subarray) + " " + std::to_string(row) + " " + std::to_string(column);

            /* save timingvalues with the corresponding address */
            timingMap[location] = timingValues;
        }

        SetWriteModi();

        /* if 0x0 is not defined add to map*/
        location = "0 0 0 0 0 0";
        if (timingMap.count(location) == 0){
            timingMap[location] = {0,0,0};
        }
    }
    else
    {
        std::cout << "NVMain: Could not read energy configuration file: " 
            << filename << std::endl;
        exit(1);
    }
}


void TimingController::SetDefaultValues(){
    std::string location1 = "0 0 0 0 0 0";
    std::string location2 = "0 0 1 0 0 0";
    timingMap = {{location1,{60,120,50}},{location2,{80,140,70}}};
}

void TimingController::SetInvalidValues(){
    std::string location = "0 0 0 0 0 0";
    timingMap = {{location,{-1,-1,-1}}};
}


void TimingController::SetExampleValues(){
    std::string location1 = "0 0 0 0 0 0"; 
    std::string location2 = "0 0 0 0 2 0";
    std::string location3 = "0 0 1 0 0 0";
    std::string location4 = "0 0 0 0 2 0";
    std::string location5 = "0 0 2 0 0 0";
    timingMap = {{location1,{0.1,1.1,1.1}},{location2,{5.1,6.1,6.1}},{location3,{10.1,11.1,11.1}},{location4, {15.1,16.1,16.1}},{location5, {20.1,21.1,21.1}}};
}


