#pragma once


#include "SMObjects.h"
#include <UGVModule.h>

#include <cmath>
using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;


ref class CrashAvoidance : public UGVModule {
public:
    CrashAvoidance(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_VehicleControl^ SM_VC);

    error_state setupSharedMemory();

    void threadFunction() override;
    error_state processHeartbeats();
    error_state processSharedMemory() override;
    void shutdownModules();
    bool getShutdownFlag() override;

private:
    SM_Laser^ SM_Laser_;
    SM_VehicleControl^ SM_VC_;
    Stopwatch^ Watch;
};
