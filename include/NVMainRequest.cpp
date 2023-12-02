#include "include/NVMAddress.h"
#include "include/NVMDataBlock.h"
#include "include/NVMTypes.h"
#include <iostream>
#include <signal.h>
#include "include/NVMainRequest.h"

using namespace NVM;

NVMainRequest::NVMainRequest( ) { 
        type = NOP;
        bulkCmd = CMD_NOP; 
        threadId = 0; 
        tag = 0; 
        reqInfo = NULL; 
        flags = 0;
        arrivalCycle = 0; 
        issueCycle = 0; 
        queueCycle = 0;
        completionCycle = 0; 
        isPrefetch = false; 
        programCounter = 0; 
        burstCount = 1;
        writeProgress = 0;
        cancellations = 0;
        owner = NULL;
        energy = 0.0;
    };

NVMainRequest::~NVMainRequest( )
    { 
    };


void NVMainRequest::SetEnergy(enValue en){
    energy = en;
}

void NVMainRequest::SetTime(tiValue ti){
    time = ti;
}