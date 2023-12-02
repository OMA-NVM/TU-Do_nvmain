#ifndef __AREAACCESSTRACEWRITER_H__
#define __AREAACCESSTRACEWRITER_H__

#include "traceWriter/GenericTraceWriter.h"
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include "src/AddressTranslator.h"
#include <tuple>
#include "Areas/Areas.h"

namespace NVM {

class AreaAccessTraceWriter : public GenericTraceWriter
{
  public:
    AreaAccessTraceWriter( );
    ~AreaAccessTraceWriter( );
    
    void SetTraceFile( std::string file );
    std::string GetTraceFile( );
    
    bool SetNextAccess( TraceLine *nextAccess );
    bool SetNextAccess( TraceLine *nextAccess, AddressTranslator transl );
  
  private:
    std::string traceFile;
    std::ofstream trace;

    AddressTranslator translator;

    void WriteTraceLine( std::ostream& , TraceLine *line );

};

};

#endif
