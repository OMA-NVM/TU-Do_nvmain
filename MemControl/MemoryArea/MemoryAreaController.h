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

#ifndef __MEMCONTROL_MEMORYAREACONTROLLER_H__
#define __MEMCONTROL_MEMORYAREACONTROLLER_H__

#include "src/MemoryController.h"
#include "Areas/Energy/EnergyController.h"
#include "Areas/Timing/TimingController.h"

namespace NVM {

class MemoryAreaController : public MemoryController
{
  public:
    MemoryAreaController( );
    ~MemoryAreaController( ) { }

    void SetConfig( Config *conf, bool createChildren = true );

    bool IssueCommand( NVMainRequest *request );
    bool IsIssuable( NVMainRequest *request, FailReason *fail = NULL );
    bool RequestComplete( NVMainRequest * request );

    void UpdateAreas( NVMainRequest *request );
    std::string ConvertDataString(NVMDataBlock &data);

    void Cycle( ncycle_t );

    void RegisterStats( );
    void CalculateStats( );

  private:
    uint64_t queueSize;

    /* Stats */
    uint64_t measuredLatencies, measuredQueueLatencies, measuredTotalLatencies;
    double averageLatency, averageQueueLatency, averageTotalLatency;
    uint64_t mem_reads, mem_writes;
    uint64_t rb_hits;
    uint64_t rb_miss;

    EnergyController energyController;
    TimingController timingController;

    uint64_t interfaceAddress;
    bool addressSet;
    int areaNumbers;
    int channel;
    int rank;
    int bank;
    int subarray;
    int row;
    int column;
    int writeMode;
    
};

};

#endif
