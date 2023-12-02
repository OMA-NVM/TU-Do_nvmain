#ifndef __ENERGYCONTROLLER_H__
#define __ENERGYCONTROLLER_H__

#include "include/NVMainRequest.h"
#include <map>
#include <functional>
#include <string>
#include <tuple>
#include "include/NVMAddress.h"
#include "Areas/Areas.h"

namespace NVM {


class EnergyController
{
  public:
    EnergyController( );
    virtual ~EnergyController( );

    enValue GetEnergyValue(NVMainRequest *request);
    enValue DetermineWriteEnergy(enValue setEnergy, enValue resetEnergy, NVMainRequest *request);
    void ReadConfig( std::string filename );
    void ReadJoinedConfig( std::string filename);
    void SetExampleValues();

    void SetDefaultValues();
    void SetInvalidValues();
    void UpdateEnergyMap(std::string loc, int mode);
    void SetWriteModi();


  private:
    std::map<std::string, std::tuple<enValue,enValue,enValue>, cmpLocation> energyMap;

    std::map<int, std::tuple<enValue,enValue,enValue>> writeModi;
    
  protected:
    
};


};


#endif









