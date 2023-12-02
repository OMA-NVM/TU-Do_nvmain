#ifndef __TIMINGCONTROLLER_H__
#define __TIMINGCONTROLLER_H__

#include "include/NVMainRequest.h"
#include <map>
#include <functional>
#include <string>
#include <tuple>
#include "include/NVMAddress.h"
#include "Areas/Areas.h"

namespace NVM {


class TimingController
{
  public:
    TimingController( );
    virtual ~TimingController( );

    tiValue GetTimingValue(NVMainRequest *request);
    tiValue DetermineWritingTime(tiValue setTime, tiValue resetTime, NVMainRequest *request);
    void ReadConfig( std::string filename );
    void ReadJoinedConfig( std::string filename );
    void SetExampleValues();
    void UpdateTimingMap(std::string loc, int mode);
    void SetWriteModi();

    void SetDefaultValues();
    void SetInvalidValues();

  private:
    std::map<std::string, std::tuple<tiValue,tiValue,tiValue>, cmpLocation> timingMap;

    std::map<int, std::tuple<enValue,enValue,enValue>> writeModi;


  protected:
    
};


};


#endif









