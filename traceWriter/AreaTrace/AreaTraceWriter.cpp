#include "traceWriter/AreaTrace/AreaTraceWriter.h"
#include <iomanip>
#include <tuple>
#include <string>
#include "Areas/Areas.h"

using namespace NVM;

AreaTraceWriter::AreaTraceWriter() {
    totalReads= 0;
    totalWrites = 0;
    totalEnergy = 0;
    totalTime = 0;
}

AreaTraceWriter::~AreaTraceWriter() {
    std::cout << "Simulation ended - Writing data in tracefile" << std::endl;
    if (trace.is_open())
    {
       Flush_trace_file(statisticsTrace);
    }
    if (csvTrace.is_open())
    {
       Flush_trace_file_csv(csvTrace);
    }
    if (this->GetEcho())
    {
       Flush_trace_file(std::cout);
    }
}


void AreaTraceWriter::SetTraceFile(std::string file)
{
    // Note: This function assumes an absolute path is given, otherwise
    // the current directory is used.

    traceFile = file;

    /* save statistics in an own statistics file*/
    std::string filename = traceFile;
    std::string extension = "";

    // find the last occurrence of '.' '/' and '\'
    size_t posPoint = filename.find_last_of(".");
    size_t posSlash = filename.find_last_of("/");
    size_t posBSlash = filename.find_last_of("\\");

    // if '.' is last behind '\' or '/', split the string
    if (posPoint == std::string::npos){
        traceStatisticsFile = filename + "-statistics";
        traceCSVFile = traceStatisticsFile + "-csv";
    } else {
        
        if ((posSlash != std::string::npos && posSlash > posPoint) || (posBSlash != std::string::npos && posBSlash > posPoint)){
            traceStatisticsFile = filename + "-statistics";
            traceCSVFile = traceStatisticsFile + "-csv";
        } else {
            extension = filename.substr(posPoint); //+1
            traceStatisticsFile = filename.substr(0,posPoint) + "-statistics" + extension;
            traceCSVFile = filename.substr(0,posPoint) + "-statistics-csv" + extension;
        }        
    }

    trace.open(traceFile.c_str());

    statisticsTrace.open(traceStatisticsFile.c_str());

    csvTrace.open(traceCSVFile.c_str());

    if (!trace.is_open())
    {
        std::cout << "Warning: Could not open trace file " << file
                  << ". Output will be suppressed." << std::endl;
    }

    if (!statisticsTrace.is_open()){
        std::cout << "Statistics File could not be opened" << std::endl;
    }

    if (!csvTrace.is_open()){
        std::cout << "CSV File could not be opened" << std::endl;
    }

    /* Write version number of this writer. */
    trace << "NVMV1-Area" << std::endl;
    statisticsTrace << "NVM1-Area" << std::endl;
}

std::string AreaTraceWriter::GetTraceFile() { return traceFile; }

bool AreaTraceWriter::SetNextAccess(TraceLine *nextAccess)
{
    bool rv = false;

    if (trace.is_open())
    {
        WriteTraceLine(trace, nextAccess);
        UpdateMap(nextAccess);
        rv = trace.good();
    }

    if (this->GetEcho())
    {
        WriteTraceLine(std::cout, nextAccess);
        rv = true;
    }

    return rv;
}


/* write statistics file */
void AreaTraceWriter::Flush_trace_file(std::ostream &stream){

    std::tuple<unsigned int, unsigned int, enValue, tiValue> statistics;
    stream << "Memory Layout: Channel, Rank, Bank, Sub-Array, Row, Column" << "\n";

    /* write each entry of the statistics map into file*/
    for(auto it = memoryMap.begin(); it != memoryMap.end(); ++it)
    {
        stream.precision(10);
        statistics = it->second;
        stream << "Location: " << it->first << " Reads: " << std::get<0>(statistics) << " Writes: " << std::get<1>(statistics);
        
        if (std::get<2>(statistics) >= 0){
            stream << " Energy: " << std::get<2>(statistics); 
            totalEnergy = totalEnergy + (long) std::get<2>(statistics);
        } else {
            totalEnergy = -1;
        }
        
        if (std::get<3>(statistics) >= 0){
            stream << " Time: " << std::get<3>(statistics);
            totalTime = totalTime + (long) std::get<3>(statistics);
        } else {
            totalTime = -1;
        }
        
        stream << "\n";
    }

    /* write total values into file */
    stream << "Total Reads: " << totalReads << " \nTotal Writes: " << totalWrites << " \n";

    unsigned long comparator = -1;

    /* only write energy or time if they were used */
    if (totalEnergy != comparator){
        stream << "Total Energy: " << totalEnergy << "\n";
    }
    if (totalTime != comparator){
        stream << "Total Time: " << totalTime << "\n";
    }
}

/* write statistics csv file*/
void AreaTraceWriter::Flush_trace_file_csv(std::ostream &stream){

    std::tuple<unsigned int, unsigned int, enValue, tiValue> statistics;
    for(auto it = memoryMap.begin(); it != memoryMap.end(); ++it)
    {
        stream.precision(10);
        statistics = it->second;
        stream << it->first << ", " << std::get<0>(statistics) << ", " << std::get<1>(statistics);
        
        if (std::get<2>(statistics) >= 0){
            stream << ", " << std::get<2>(statistics); 
            totalEnergy = totalEnergy + (long) std::get<2>(statistics);
        } else {
            totalEnergy = -1;
        }
        
        if (std::get<3>(statistics) >= 0){
            stream << ", " << std::get<3>(statistics);
            totalTime = totalTime + (long) std::get<3>(statistics);
        } else {
            totalTime = -1;
        }
        
        stream << "\n";
    }
}

/* update statistics map with each traceline*/
void AreaTraceWriter::UpdateMap(TraceLine *line){
   
    /* get memory location*/
    std::string location;
    int channel = line->GetAddress().GetChannel();
    int rank = line->GetAddress().GetRank();
    int bank = line->GetAddress().GetBank();
    int subarray = line->GetAddress().GetSubArray();
    int row = line->GetAddress().GetRow();
    int column = line->GetAddress().GetCol();
    location = std::to_string(channel) + " " + std::to_string(rank) + " " + std::to_string(bank) + " " + std::to_string(subarray) + " " + std::to_string(row) + " " + std::to_string(column);
    
    /* get time and energy*/
    enValue energy = line->GetEnergy();
    tiValue time = line->GetTime();
    std::tuple<unsigned int, unsigned int, enValue, tiValue> statistics;

    auto it = memoryMap.find(location);

    /* if location already in map, update value*/
    if (it != memoryMap.end()){
        statistics = it->second;
        if (line->GetOperation() == READ) {
            memoryMap[location] = {( std::get<0>(statistics) + 1 ), std::get<1>(statistics), ( std::get<2>(statistics) + energy ), ( std::get<3>(statistics) + time )};
            totalReads++;
        }
        else if (line->GetOperation() == WRITE) {
            memoryMap[location] = { std::get<0>(statistics) , ( std::get<1>(statistics) + 1 ) , ( std::get<2>(statistics) + energy ), ( std::get<3>(statistics) + time )};
            totalWrites++;
        }
    } else { /* if new location create new entry*/
        if (line->GetOperation() == READ) {
            memoryMap[location] = {1, 0, energy, time};
            totalReads++;
        }
        else if (line->GetOperation() == WRITE) {
            memoryMap[location] = {0, 1, energy, time};
            totalWrites++;
        }
    }

}

void AreaTraceWriter::WriteTraceLine(std::ostream &stream, TraceLine *line)
{
    NVMDataBlock &data = line->GetData();

    /* Only print reads or writes. */
    if (line->GetOperation() != READ && line->GetOperation() != WRITE)
        return;

    /* Print memory cycle. */
    stream << line->GetCycle() << " ";

    /* Print the operation type */
    if (line->GetOperation() == READ) {
        stream << "R ";
    }
    else if (line->GetOperation() == WRITE) {
        stream << "W ";
    }

    /* Print address */
    stream << std::hex << "0x" << line->GetAddress().GetPhysicalAddress() << std::dec << " ";

    /* Print program counter */
    stream << std::hex << "0x" << line->get_program_counter() << std::dec << " ";

    /* Print data. */
    stream << data << " ";

    /* Print memory location*/
    stream << line->GetAddress().GetChannel() << ":" << line->GetAddress().GetRank() << ":" << line->GetAddress().GetBank() << ":" << line->GetAddress().GetSubArray() << ":" << line->GetAddress().GetRow() << ":" << line->GetAddress().GetCol() << " ";

    /* Print energy and time */
    stream.precision(8);

    if (line->GetEnergy() >= 0){
        stream << line->GetEnergy() << " ";
    }

    if (line->GetTime() >= 0){
        stream << line->GetTime() << " ";
    }

    stream << std::endl;
}
