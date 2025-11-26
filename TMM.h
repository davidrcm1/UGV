// TMM.h
#pragma once

#include "Controller.h" // always put controller and controllerinterface first

#include <UGVModule.h>


#include "Laser.h"
#include "GNSS.h"
#include "Display.h"
#include "VC.h"
#include "SMObjects.h"
#include "CrashAvoidance.h"

#using <System.dll>


ref struct ThreadProperties
{
    ThreadStart^ ThreadStart_;
    bool Critical;
    String^ ThreadName;
    uint8_t BitID;

    ThreadProperties(ThreadStart^ start, bool critical, uint8_t bit_ID, String^ threadName) {
        ThreadStart_ = start;
        Critical = critical;
        ThreadName = threadName;
        BitID = bit_ID;
    }
};

ref class ThreadManagement : public UGVModule {
public:
    // Create shared memory objects
    error_state setupSharedMemory();

    // Thread function for TMM
    void threadFunction() override;

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    // Shutdown all modules in the software (AKA SHUTDOWN THREAD from lecture)
    void shutdownModules();

    // Get Shutdown signal for module, from Thread Management SM
    bool getShutdownFlag() override;

    ///
    ///
    error_state processHeartbeats();

private:
    SM_ThreadManagement^ SM_TM_;
    SM_Laser^ SM_Laser_;
    SM_GPS^ SM_Gps_;
    SM_VehicleControl^ SM_Vc_;

    array<Stopwatch^>^ StopwatchList;
    array<Thread^>^ ThreadList;
    array<ThreadProperties^>^ ThreadPropertiesList;
};
