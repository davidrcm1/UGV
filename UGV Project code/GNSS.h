#pragma once

#include "NetworkedModule.h"  
#using <System.dll>
#include <cstring>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

#define CRC32_POLYNOMIAL 0xEDB88320L

// lec  7.5
#pragma pack(push, 4)
struct GNSSData {

    unsigned char Discards1[40];
    double Northing; // byteset 40
    double Easting; // byteset 48
    double Height; // byteset 56
    unsigned char Discards2[40];
    unsigned int CRC; // byteset 104
};
#pragma pack(pop, 4)

ref class GNSS : public NetworkedModule {
public:
    GNSS(SM_ThreadManagement^ SM_TM, SM_GPS^ SM_GPS);

    error_state setupSharedMemory();
    error_state processSharedMemory() override;
    bool getShutdownFlag() override;
    void threadFunction() override;

    error_state connect(String^ hostName, int portNumber) override;
    error_state communicate() override;

    error_state processHeartbeats();
    error_state checkData();
    void shutdownModules();

    unsigned long CRC32Value(int i);
    unsigned long CalculateBlockCRC32(unsigned long ulCount, unsigned char* ucBuffer);

    ~GNSS() {}

private:
    SM_GPS^ SM_Gps_;
    Stopwatch^ Watch;
    unsigned long receivedCRC;
};