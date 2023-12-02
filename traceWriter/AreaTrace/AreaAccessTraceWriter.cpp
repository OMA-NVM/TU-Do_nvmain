#include "traceWriter/AreaTrace/AreaAccessTraceWriter.h"
#include <iomanip>
#include <tuple>
#include "Areas/Areas.h"

using namespace NVM;

AreaAccessTraceWriter::AreaAccessTraceWriter() {}

AreaAccessTraceWriter::~AreaAccessTraceWriter() {}


void AreaAccessTraceWriter::SetTraceFile(std::string file)
{
    // Note: This function assumes an absolute path is given, otherwise
    // the current directory is used.

    traceFile = file;

    trace.open(traceFile.c_str());

    if (!trace.is_open())
    {
        std::cout << "Warning: Could not open trace file " << file
                  << ". Output will be suppressed." << std::endl;
    }

    /* Write version number of this writer. */
    trace << "NVMV1-AreaAccess" << std::endl;
}

std::string AreaAccessTraceWriter::GetTraceFile() { return traceFile; }

bool AreaAccessTraceWriter::SetNextAccess(TraceLine *nextAccess)
{
    bool rv = false;

    if (trace.is_open())
    {
        WriteTraceLine(trace, nextAccess);
        rv = trace.good();
    }

    if (this->GetEcho())
    {
        WriteTraceLine(std::cout, nextAccess);
        rv = true;
    }

    return rv;
}


void AreaAccessTraceWriter::WriteTraceLine(std::ostream &stream, TraceLine *line)
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
