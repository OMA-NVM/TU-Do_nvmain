/*******************************************************************************
* Copyright (c) 2012-2014, The Microsystems Design Labratory (MDL)
* Department of Computer Science and Engineering, The Pennsylvania State University
* All rights reserved.
* 
* This source code is part of NVMain - A cycle accurate timing, bit accurate
* energy simulator for both volatile (e.g., DRAM) and non-volatile memory
* (e.g., PCRAM). The source code is free and you can redistribute and/or
* modify it by providing that the following conditions are met:
* 
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
* 
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Author list: 
*   Matt Poremba    ( Email: mrp5060 at psu dot edu 
*                     Website: http://www.cse.psu.edu/~poremba/ )
*******************************************************************************/

#include "MemControl/MemoryArea/MemoryAreaController.h"
#include "src/EventQueue.h"
#include "include/NVMainRequest.h"
#include "Areas/Energy/EnergyController.h"
#include "Areas/Timing/TimingController.h"
#include <string>
#include "include/NVMHelpers.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <bitset>

using namespace NVM;

/*
 *  This is a simple memory controller that operates as follows:
 *
 *  - After each read or write is issued, the page is closed.
 *    For each request, it simply prepends an activate before the read/write and 
 *    appends a precharge
 *  - This memory controller leaves all banks and ranks in active mode 
 *    (it does not do power management)
 */
MemoryAreaController::MemoryAreaController( )
{
    std::cout << "Created a MemoryArea memory controller!" << std::endl;

    queueSize = 32;

    averageLatency = 0.0f;
    averageQueueLatency = 0.0f;
    averageTotalLatency = 0.0f;

    measuredLatencies = 0;
    measuredQueueLatencies = 0;
    measuredTotalLatencies = 0;

    mem_reads = 0;
    mem_writes = 0;

    rb_hits = 0;
    rb_miss = 0;

    psInterval = 0;

    energyController = EnergyController();
    timingController = TimingController();
    addressSet = false;
    areaNumbers = 0;
    channel = 0;
    rank = 0;
    bank = 0;
    subarray = 0;
    row = 0;
    column = 0;
    writeMode = 0;

    InitQueues( 1 );
}

void MemoryAreaController::SetConfig( Config *conf, bool createChildren )
{
    if( conf->KeyExists( "QueueSize" ) )
    {
        queueSize = static_cast<unsigned int>( conf->GetValue( "QueueSize" ) );
    }

    MemoryController::SetConfig( conf, createChildren );

    if( conf->KeyExists( "MemoryAreas" ))
    {
        /* only energy is selected */
        if (conf->GetString( "MemoryAreas" ) == "energy"){

            if( conf->KeyExists( "EnergyFile" ))
            {
                if ( conf->GetString( "EnergyFile" ) == ""){
                    std::cout << "NVMain: No energy configuration file set" << std::endl;
                } else {
                    std::string energyFile = conf->GetString( "EnergyFile" );

                    if (energyFile[0] != '/'){
                        energyFile = "/gem/nvmain/Areas/Energy/";
                        energyFile += conf->GetString( "EnergyFile" );
                    }
                    std::cout << "Using EnergyFile: " << energyFile << std::endl;
                    energyController.ReadConfig(energyFile);
                } 
            } else {

                energyController.SetDefaultValues();
                std::cout << "Missing energy configuration file, using default values." << std::endl;
            }
            /* set invalid time values to show trace writer time is not simulated*/
            timingController.SetInvalidValues();

        /* only time is selected */
        } else if (conf->GetString( "MemoryAreas" ) == "time"){

            if( conf->KeyExists( "TimingFile" ))
            {
                if ( conf->GetString( "TimingFile" ) == ""){
                    std::cout << "NVMain: No timing configuration file set" << std::endl;
                } else {
                    std::string timingFile = conf->GetString( "TimingFile" );

                    if (timingFile[0] != '/'){
                        timingFile = "/gem/nvmain/Areas/Timing/"; 
                        timingFile += conf->GetString( "TimingFile" );
                    }
                    std::cout << "Using TimingFile: " << timingFile << std::endl;
                    timingController.ReadConfig(timingFile);
                } 
            } else {
                timingController.SetDefaultValues();
                std::cout << "Missing time configuration file, using default values." << std::endl;
            }
            /* set invalid energy values to show trace writer energy is not simulated*/
            energyController.SetInvalidValues();

        /* energy and time is selected */
        } else if (conf->GetString( "MemoryAreas" ) == "both"){

            if( conf->KeyExists( "AreaFile" ))
            {
                if ( conf->GetString( "AreaFile" ) == ""){
                    std::cout << "NVMain: No energy and timing configuration file set" << std::endl;
                } else {
                    std::string areaFile = conf->GetString( "AreaFile" );

                    if (areaFile[0] != '/'){
                        areaFile = "/gem/nvmain/Areas/"; 
                        areaFile += conf->GetString( "AreaFile" );
                    }
                    std::cout << "Using AreaFile: " << areaFile << std::endl;

                    energyController.ReadJoinedConfig(areaFile);
                    timingController.ReadJoinedConfig(areaFile);
                    
                } 
            } else {
                timingController.SetDefaultValues();
                energyController.SetDefaultValues();
                std::cout << "Missing area configuration file, using default values." << std::endl;
            }
            
        } else {
            std::cout << "Config: Warning: Key MemoryArea has no valid value. Please use \"energy\", \"time\" or \"both\" as options." << std::endl;
            energyController.SetInvalidValues();
            timingController.SetInvalidValues();
        }
    }
    
    SetDebugName( "MemoryAreaController", conf );
}

void MemoryAreaController::RegisterStats( )
{
    AddStat(mem_reads);
    AddStat(mem_writes);
    AddStat(rb_hits);
    AddStat(rb_miss);
    AddStat(averageLatency);
    AddStat(averageQueueLatency);
    AddStat(averageTotalLatency);
    AddStat(measuredLatencies);
    AddStat(measuredQueueLatencies);
    AddStat(measuredTotalLatencies);

    MemoryController::RegisterStats( );
}

bool MemoryAreaController::RequestComplete( NVMainRequest * request )
{
    /* 
     * Only reads and writes are sent back to NVMain and 
     * checked for in the transaction queue. 
     */
    if( request->type == READ 
        || request->type == READ_PRECHARGE 
        || request->type == WRITE 
        || request->type == WRITE_PRECHARGE
        )
    {
        request->status = MEM_REQUEST_COMPLETE;
        request->completionCycle = GetEventQueue()->GetCurrentCycle();

        /* Update the average latencies based on this request for READ/WRITE only. */
        averageLatency = ((averageLatency * static_cast<double>(measuredLatencies))
                           + static_cast<double>(request->completionCycle)
                           - static_cast<double>(request->issueCycle))
                       / static_cast<double>(measuredLatencies+1);
        measuredLatencies += 1;

        averageQueueLatency = ((averageQueueLatency 
                                * static_cast<double>(measuredQueueLatencies))
                                + static_cast<double>(request->issueCycle)
                                - static_cast<double>(request->arrivalCycle))
                            / static_cast<double>(measuredQueueLatencies+1);
        measuredQueueLatencies += 1;

        averageTotalLatency = ((averageTotalLatency * static_cast<double>(measuredTotalLatencies))
                                + static_cast<double>(request->completionCycle)
                                - static_cast<double>(request->arrivalCycle))
                            / static_cast<double>(measuredTotalLatencies+1);
        measuredTotalLatencies += 1;
    }

    NVMAddress adress = request->address;
    uint64_t physicalAdress = adress.GetPhysicalAddress();
    uint64_t bitAdress = adress.GetBitAddress();

    return MemoryController::RequestComplete( request );
}

bool MemoryAreaController::IsIssuable( NVMainRequest * /*request*/, FailReason * /*fail*/ )
{
    bool rv = true;

    /* Allow up to 16 read/writes outstanding. */
    if( transactionQueues[0].size( ) >= queueSize )
        rv = false;

    return rv;
}

/*
 *  This method is called whenever a new transaction from the processor issued to
 *  this memory controller / channel. All scheduling decisions should be made here.
 */
bool MemoryAreaController::IssueCommand( NVMainRequest *request )
{
    /* Allow up to 16 read/writes outstanding. */
    if( !IsIssuable( request ) )
        return false;

    if( request->type == READ )
        mem_reads++;
    else
        mem_writes++;

    

    /* set energy and timing values of request */
    (*request).SetEnergy(energyController.GetEnergyValue(request));
    (*request).SetTime(timingController.GetTimingValue(request));

    /* check if the request is relevant for the interface */
    UpdateAreas(request);

    Enqueue( 0, request );

    /*
     * Return whether the request could be queued. Return false if the queue is full.
     */
    return true;
}


/* converts data from little endian to big endian */
std::string MemoryAreaController::ConvertDataString(NVMDataBlock &data){
    std::stringstream dataStream;
    data.Print(dataStream);
    std::string dataString = dataStream.str();
    std::string convertedData = "";
    std::string byte = "";

    for (int i = 0; i < dataString.size() ; i = i + 2){
        byte = {dataString[i], dataString[i+1]};
        convertedData =  byte + convertedData;
    }

    return convertedData;
}

/* dynamically updates the write mode of areas at runtime */
void MemoryAreaController::UpdateAreas(NVMainRequest *request){

    uint64_t address = request->address.GetPhysicalAddress();
    NVMDataBlock &data = request->data;
    std::string compareValue = "0300000000003333";//"3333333333333333";
    std::string dataValue;
    std::stringstream dataStream;
    data.Print(dataStream);
    dataValue = dataStream.str();

    if (!addressSet){ 

        /* set address of interface */
        if (compareValue == dataValue && request->type == WRITE){
            interfaceAddress = address;
            addressSet = true;
            std::cout << "Address of interface array is: 0x" << std::hex << interfaceAddress << std::dec << std::endl; 
        }

    } else {

        if (address == interfaceAddress && request->type == WRITE){

            /* convert data to big endian */
            dataValue = ConvertDataString( data );

            /* extract values from memory word */
            channel = std::stoi(dataValue.substr(0,1), nullptr, 16);
            rank = std::stoi(dataValue.substr(1,1), nullptr, 16);
            bank = std::stoi(dataValue.substr(2,1), nullptr,16);
            subarray = std::stoi(dataValue.substr(3,1), nullptr, 16);
            writeMode = std::stoi(dataValue.substr(15,1), nullptr, 16);

            /* extract and convert row and column */
            std::string binary = std::bitset<44>(std::stoi(dataValue.substr(4,11), nullptr, 16)).to_string();
            row = std::stoi(binary.substr(0,22), nullptr, 2);
            column = std::stoi(binary.substr(22,22), nullptr, 2);

            /* compute location*/
            std::string location = std::to_string(channel) + " " + std::to_string(rank) + " " + std::to_string(bank) + " " + std::to_string(subarray) + " " + std::to_string(row) + " " + std::to_string(column);
            std::cout << std::dec << "Updating Area from location: " << location << " to writeMode: " << writeMode << std::endl;

            /* update write mode in areas*/
            energyController.UpdateEnergyMap(location, writeMode);
            timingController.UpdateTimingMap(location, writeMode);    
        
        } 

    }
}

void MemoryAreaController::Cycle( ncycle_t steps )
{
    NVMainRequest *nextReq = NULL;

    /* Simply get the oldest request */
    if( !FindOldestReadyRequest( transactionQueues[0], &nextReq ) )
    {
        /* No oldest ready request, check for non-activated banks. */
        (void)FindClosedBankRequest( transactionQueues[0], &nextReq );
    }

    if( nextReq != NULL )
    {
        IssueMemoryCommands( nextReq );
    }

    CycleCommandQueues( );

    MemoryController::Cycle( steps );
}

void MemoryAreaController::CalculateStats( )
{
    MemoryController::CalculateStats( );
}

