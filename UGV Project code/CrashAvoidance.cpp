#include "CrashAvoidance.h"


using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

CrashAvoidance::CrashAvoidance(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_VehicleControl^ SM_VC) {
    SM_Laser_ = SM_Laser;
    SM_VC_ = SM_VC;
    SM_TM_ = SM_TM;
    Watch = gcnew Stopwatch();
}

error_state CrashAvoidance::setupSharedMemory() {
    return SUCCESS;
}

void CrashAvoidance::threadFunction() {
    //Console::WriteLine("CrashAvoidance thread is starting.");
    SM_TM_->ThreadBarrier->SignalAndWait();
    Watch->Start();

    while (!getShutdownFlag()) {
        processHeartbeats();
        processSharedMemory();
        Thread::Sleep(20);
    }

    shutdownModules();
    Console::WriteLine("CrashAvoidance shutdown");
}

error_state CrashAvoidance::processHeartbeats() {
    if ((SM_TM_->heartbeat & bit_CRASHAVOIDANCE) == 0) {
        SM_TM_->heartbeat |= bit_CRASHAVOIDANCE;
        Watch->Restart();
    }
    else if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
        //Console::WriteLine("CrashAvoidance timeout. Shutting down.");
        shutdownModules();
        return ERR_TMM_FAILURE;
    }
    return SUCCESS;
}

void CrashAvoidance::shutdownModules() {
    //Console::WriteLine("CrashAvoidance shutdownmodules.");
    SM_TM_->shutdown |= bit_CRASHAVOIDANCE;
}

bool CrashAvoidance::getShutdownFlag() {
    return (SM_TM_->shutdown & bit_CRASHAVOIDANCE) != 0;
}

error_state CrashAvoidance::processSharedMemory() {
    Monitor::Enter(SM_Laser_->lockObject);
    array<double>^ laserX = SM_Laser_->x;
    array<double>^ laserY = SM_Laser_->y;
    Monitor::Exit(SM_Laser_->lockObject);

    bool obstacleDetected = false;
    for (int i = 0; i < laserX->Length; ++i) {
        double distance = Math::Sqrt(laserX[i] * laserX[i] + laserY[i] * laserY[i]);
        if (distance < 1.0) { 
            obstacleDetected = true;
            break;
        }
    }

    if (obstacleDetected) {
        //Console::WriteLine("CrashAvoidance: Obstacle detected.");
        // CHANGE***************
        SM_VC_->emergencyStop = true;
    }
    else {
        SM_VC_->emergencyStop = false; 
    }
    

    return SUCCESS;
}
