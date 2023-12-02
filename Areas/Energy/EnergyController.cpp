#include "Areas/Energy/EnergyController.h"
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
#include <map>
#include "Areas/interface-config.h"

using namespace NVM;

EnergyController::EnergyController( ){ };

EnergyController::~EnergyController( ){};

void EnergyController::UpdateEnergyMap(std::string location, int mode){
    std::tuple<enValue,enValue,enValue> energyValues = (writeModi.find(mode))->second;

    /* find the greatest location strictly less than the given location */
    auto it = energyMap.lower_bound(location);
    if (it != energyMap.begin() && location != it->first) {
        --it;
    }

    /* update the energy values with new write mode */
    energyMap[it->first] = energyValues;
    std::cout << "-- Updating Energy Area " << it->first << " with values: " << "{" << std::get<0>( it->second) << ", " << std::get<1>( it->second) << ", " << std::get<2>( it->second) << "}" << std::endl;
        
}

void EnergyController::SetWriteModi(){

    writeModi[0] = { INTERFACE_STANDARD_READENERGY, INTERFACE_MODE_0_SETENERGY, INTERFACE_MODE_0_RESETENERGY};
    writeModi[1] = { INTERFACE_STANDARD_READENERGY, INTERFACE_MODE_1_SETENERGY, INTERFACE_MODE_1_RESETENERGY};
    writeModi[2] = { INTERFACE_STANDARD_READENERGY, INTERFACE_MODE_2_SETENERGY, INTERFACE_MODE_2_RESETENERGY};
    writeModi[3] = { INTERFACE_STANDARD_READENERGY, INTERFACE_MODE_3_SETENERGY, INTERFACE_MODE_3_RESETENERGY};

    std::tuple<enValue,enValue,enValue> modeValues;

    std::cout << "Configured energy write modi to: " << std::endl;

    for (auto it = writeModi.begin(); it != writeModi.end(); ++it){
        modeValues = it->second;
        std::cout << "- " << it->first << " : {" << std::get<0>(modeValues) << ", " << std::get<1>(modeValues) << ", " << std::get<2>(modeValues) << "}" << std::endl;
    }

}

enValue EnergyController::GetEnergyValue(NVMainRequest *request){ 

    std::string location;
    int channel = request->address.GetChannel();
    int rank = request->address.GetRank();
    int bank = request->address.GetBank();
    int subarray = request->address.GetSubArray();
    int row = request->address.GetRow();
    int column = request->address.GetCol();
    location = std::to_string(channel) + " " + std::to_string(rank) + " " + std::to_string(bank) + " " + std::to_string(subarray) + " " + std::to_string(row) + " " + std::to_string(column);

    /* find the greatest location strictly less than the given location*/
    auto it = energyMap.lower_bound(location);
    if (it != energyMap.begin() && location != it->first) {
        --it;
    }

    /* extract energy values for this location*/
    std::tuple<enValue,enValue,enValue> energyValues;
    energyValues = it->second;

    /* return energy based on operation type and size of data */
    if(request->type == READ){
        return (std::get<0>(energyValues)) * request->data.GetSize() * 8;
    }
    else if (request->type == WRITE){
        return DetermineWriteEnergy(std::get<1>(energyValues), std::get<2>(energyValues), request);
    }
    else   
        return 0.0;
}


enValue EnergyController::DetermineWriteEnergy(enValue setEnergy, enValue resetEnergy, NVMainRequest *request){
    NVMDataBlock &data = request->data;
    uint64_t size = data.GetSize();
    uint8_t dataByte;
    int ones = 0;
    int zeros = 0;
    std::string binary;

    /* convert every byte to binary and count 1s and 0s */
    for (int i = 0; i < size; i++){
        dataByte = data.GetByte(i);
        binary = std::bitset<8>((int) dataByte).to_string();
        /* count 1s */
        std::string::difference_type n = std::count(binary.begin(), binary.end(), '1');
        ones += (int) n;
        zeros += (8 - (int) n);
    }

    /* return complete write energy determined by set and reset energy*/
    return (ones * setEnergy ) + ( zeros * resetEnergy );
}


/* reads the energy config and saves the energy values and address in the energyMap */
void EnergyController::ReadConfig( std::string filename ){
    std::string line;
    std::ifstream configFile( filename.c_str( ) );
    std::string subline;

    int channel;
    int rank;
    int bank;
    int subarray;
    int row;
    int column;
    enValue readEnergy;
    enValue setEnergy;
    enValue resetEnergy;
    std::tuple<enValue,enValue,enValue> energyValues;
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

            /* extract energy values */
            tokens = strtok( NULL, " " );
            readEnergy = stod(std::string( tokens )); 
            tokens = strtok( NULL, " " );
            setEnergy = stod(std::string( tokens )); 
            tokens = strtok( NULL, " " );
            resetEnergy = stod(std::string( tokens )); 
            energyValues = {readEnergy, setEnergy, resetEnergy};

            location = std::to_string(channel) + " " + std::to_string(rank) + " " + std::to_string(bank) + " " + std::to_string(subarray) + " " + std::to_string(row) + " " + std::to_string(column);
            
            /* save energyvalues with the corresponding location */
            energyMap[location] = energyValues;

        }

        SetWriteModi();

        /* if 0x0 is not defined add to map*/
        location = "0 0 0 0 0 0";
        if (energyMap.count(location) == 0){
            energyMap[location] = {0,0,0};
            std::cout << "Warning: Definition of memory areas does not include 0x0. Using 0 as default value." << std::endl;
        }
    }
    else
    {
        std::cout << "NVMain: Could not read energy configuration file: " 
            << filename << std::endl;
        exit(1);
    }
    std::cout << "Defined " << energyMap.size() << " energy areas." << std::endl;
}



/* reads the areas config and saves the energy values and address in the energyMap */
void EnergyController::ReadJoinedConfig( std::string filename){
    std::string line;
    std::ifstream configFile( filename.c_str( ) );
    std::string subline;

    int channel;
    int rank;
    int bank;
    int subarray;
    int row;
    int column;
    enValue readEnergy;
    enValue setEnergy;
    enValue resetEnergy;
    std::tuple<enValue,enValue,enValue> energyValues;
    std::string location = "0 0 0 0 0 0";
    u_int64_t address;


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
            readEnergy = stod(std::string( tokens )); 
            tokens = strtok( NULL, " " );
            /* skip time value */
            tokens = strtok( NULL, " " );
            setEnergy = stod(std::string( tokens )); 
            tokens = strtok( NULL, " " );
            /* skip time value */
            tokens = strtok( NULL, " " );
            resetEnergy = stod(std::string( tokens )); 
            tokens = strtok( NULL, " " );
            /* skip time value */
            energyValues = {readEnergy, setEnergy, resetEnergy};

            location = std::to_string(channel) + " " + std::to_string(rank) + " " + std::to_string(bank) + " " + std::to_string(subarray) + " " + std::to_string(row) + " " + std::to_string(column);

            /* save energyvalues with the corresponding location */
            energyMap[location] = energyValues;
        }
        /* if 0x0 is not defined add to map*/
        location = "0 0 0 0 0 0";
        if (energyMap.count(location) == 0){
            energyMap[location] = {0,0,0};
            std::cout << "Warning: Definition of memory areas does not include 0x0. Using 0 as default value." << std::endl;
        }

        SetWriteModi();
    }
    else
    {
        std::cout << "NVMain: Could not read energy configuration file: " 
            << filename << std::endl;
        exit(1);
    }
    std::cout << "Defined " << energyMap.size() << " energy and timing areas." << std::endl;
}

void EnergyController::SetDefaultValues(){
    std::string location1 = "0 0 0 0 0 0";
    std::string location2 = "0 0 1 0 0 0";
    energyMap = {{location1,{0.1,0.5,1.5}},{location2,{2.1,2.5,3.5}}};
}

void EnergyController::SetInvalidValues(){
    std::string location = "0 0 0 0 0 0";
    energyMap = {{location,{-1,-1,-1}}};
}

void EnergyController::SetExampleValues(){
    std::string location1 = "0 0 0 0 0 0"; 
    std::string location2 = "0 0 0 0 2 0";
    std::string location3 = "0 0 1 0 0 0";
    std::string location4 = "0 0 0 0 2 0";
    std::string location5 = "0 0 2 0 0 0";
    energyMap = {{location1,{0.1,1.1,1.1}},{location2,{5.1,6.1,6.1}},{location3,{10.1,11.1,11.1}},{location4, {15.1,16.1,16.1}},{location5, {20.1,21.1,21.1}}};
}

