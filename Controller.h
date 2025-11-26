#pragma once

#include "ControllerInterface.h"
#include "SMObjects.h"
#include "UGVModule.h"


using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class Controller : public UGVModule
{
public:
    Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC, ControllerInterface* controllerPtr);

    error_state setupSharedMemory();
    void threadFunction() override;
    error_state processHeartbeats();
    void shutdownModules();
    bool getShutdownFlag() override;
    error_state processSharedMemory() override;
    ~Controller() {}

private:
    // SM_ThreadManagement^ SM_TM_; from smobject
    SM_VehicleControl^ SM_VC_;
    Stopwatch^ Watch;

    ControllerInterface* controllerPtr;

};

