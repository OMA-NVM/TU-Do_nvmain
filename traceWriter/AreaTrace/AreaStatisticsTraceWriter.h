#ifndef __AREASTATISTICSTRACEWRITER_H__
#define __AREASTATISTICSTRACEWRITER_H__

#include "traceWriter/GenericTraceWriter.h"
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include "src/AddressTranslator.h"
#include <tuple>
#include "Areas/Areas.h"

namespace NVM {

class AreaStatisticsTraceWriter : public GenericTraceWriter
{
  public:
    AreaStatisticsTraceWriter( );
    ~AreaStatisticsTraceWriter( );
    
    void SetTraceFile( std::string file );
    std::string GetTraceFile( );
    
    bool SetNextAccess( TraceLine *nextAccess );
    bool SetNextAccess( TraceLine *nextAccess, AddressTranslator transl );
    
  
  private:
    std::string traceFile;
    std::ofstream trace;

    std::string traceCSVFile;
    std::ofstream csvTrace;

    unsigned int totalWrites;
    unsigned int totalReads;
    unsigned long totalEnergy;
    unsigned long totalTime;

    AddressTranslator translator;

    void Flush_trace_file(std::ostream &stream);
    void Flush_trace_file_csv(std::ostream &stream);
    void UpdateMap(TraceLine *line);

    std::map<std::string, std::tuple<unsigned int, unsigned int, enValue, tiValue>, cmpLocation> memoryMap; //reads, writes, energy, time
};

};

#endif
