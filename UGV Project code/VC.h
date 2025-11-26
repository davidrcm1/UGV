#pragma once

#include "ControllerInterface.h"

#include "SMObjects.h"
#include "NetworkedModule.h"


// omni directional ethernet connection.
using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class VC : public NetworkedModule
{
public:
    VC(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC);

    error_state setupSharedMemory();
    error_state processSharedMemory() override;
    bool getShutdownFlag() override;
    void threadFunction() override;

    error_state connect(String^ hostName, int portNumber) override;
    error_state communicate() override;

    error_state processHeartbeats();
    void shutdownModules();

    ~VC() {}

private:
    SM_ThreadManagement^ SM_TM_;
    SM_VehicleControl^ SM_VC_;
    Stopwatch^ Watch;
    array<unsigned char>^ SendData;
    String^ vcRequest;
};
