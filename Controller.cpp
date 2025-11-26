#include "Controller.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

Controller::Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC, ControllerInterface* controller) {
    SM_TM_ = SM_TM;
    SM_VC_ = SM_VC;
    controllerPtr = controller;

    Watch = gcnew Stopwatch();
}

error_state Controller::setupSharedMemory() {
    return error_state::SUCCESS;
}

void Controller::threadFunction() {
    //Console::WriteLine("Controller thread is starting");
    SM_TM_->ThreadBarrier->SignalAndWait();
    Watch->Start();

    while (!getShutdownFlag()) {
        processHeartbeats();
        processSharedMemory();
        Thread::Sleep(20);
    }
    shutdownModules();
    Console::WriteLine("Controller shutdown");
}

error_state Controller::processSharedMemory() {
    if (controllerPtr->IsConnected()) {
        //Console::WriteLine("ControllerInterface IsConnected");
        _controllerState controllerInput = controllerPtr->GetState();

        //Console::WriteLine("Left Thumb (X, Y): ({0:F2}, {1:F2})", controllerInput.leftThumbX, controllerInput.leftThumbY);
        //Console::WriteLine("Right Thumb (X, Y): ({0:F2}, {1:F2})", controllerInput.rightThumbX, controllerInput.rightThumbY);
        //Console::WriteLine("Buttons (A, B, X, Y): ({0}, {1}, {2}, {3})", controllerInput.buttonA, controllerInput.buttonB, controllerInput.buttonX, controllerInput.buttonY);
        //Console::WriteLine("DPad (Up, Down, Left, Right): ({0}, {1}, {2}, {3})", controllerInput.DpadUp, controllerInput.DpadDown, controllerInput.DpadLeft, controllerInput.DpadRight);
        //Console::WriteLine("Triggers (Left, Right): ({0:F2}, {1:F2})", controllerInput.leftTrigger, controllerInput.rightTrigger);
        Console::WriteLine("", controllerInput.buttonA, controllerInput.buttonB, controllerInput.buttonX, controllerInput.buttonY);

        
        const bool shutdownFlag = controllerInput.buttonB;
        Thread::Sleep(20);
        if (shutdownFlag) {
            Console::WriteLine("SHUTTING DOWN");
            shutdownModules();  
            return error_state::ERR_CONNECTION;
        }
        else {
            // CONTROLLER CHANGE
            const double speed = controllerInput.rightTrigger - controllerInput.leftTrigger;
            //const double speed = controllerInput.rightThumbY;
            const double steer = -1 * controllerInput.rightThumbX * 40;
            //Console::WriteLine(" speed: {0:F0} steer: {1:F1}", speed, steer);

            Monitor::Enter(SM_VC_->lockObject);
            SM_VC_->Speed = speed;
            SM_VC_->Steering = steer;
            Monitor::Exit(SM_VC_->lockObject);

            return error_state::SUCCESS;
        }
    }
    else {
        Console::WriteLine("Controller not connected");
        shutdownModules();
        return error_state::ERR_CONNECTION;
    }
}

bool Controller::getShutdownFlag() {
    return (SM_TM_->shutdown & bit_CONTROLLER);
}


error_state Controller::processHeartbeats() {
    if ((SM_TM_->heartbeat & bit_CONTROLLER) == 0) {
        SM_TM_->heartbeat |= bit_CONTROLLER;
        Watch->Restart();
    }
    else if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
        Console::WriteLine("Controller module timed out. Shutting down");
        shutdownModules();
        return error_state::ERR_TMM_FAILURE;
    }
    return error_state::SUCCESS;
}

void Controller::shutdownModules() {

    Monitor::Enter(SM_VC_->lockObject);
    SM_VC_->Speed = 0;
    SM_VC_->Steering = 0;
    Monitor::Exit(SM_VC_->lockObject);

    SM_TM_->shutdown |= bit_CONTROLLER;

    //Console::WriteLine("Controller shutdownModule");
}