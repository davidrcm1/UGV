// VC.cpp
#include "VC.h"
#include <iostream>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

VC::VC(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC) {
    SM_VC_ = SM_VC;
    SM_TM_ = SM_TM;

    Watch = gcnew Stopwatch();
    connect(WEEDER_ADDRESS, 25000);
}

error_state VC::setupSharedMemory() {
    return error_state::SUCCESS;
}

void VC::threadFunction() {
    //Console::WriteLine("VC threadFunction is starting");
    SM_TM_->ThreadBarrier->SignalAndWait();
    Watch->Start();

    while (!getShutdownFlag()) {
        //Console::WriteLine("VC thread is running");
        processHeartbeats();
        processSharedMemory();
        communicate();

        Thread::Sleep(20);
    }
    shutdownModules();
    Console::WriteLine("VC shutdown");
}

error_state VC::processSharedMemory() {
    //Console::WriteLine("VC processSharedMem is starting");
 
    if (SM_VC_->emergencyStop) {
        SM_VC_->Speed = -1; // set to reverse
        SM_VC_->Steering = 0;
        Console::WriteLine("VC: Emergency stop, REVERSING.");
    }
    if (SM_VC_->watchDogFlag) {
        vcRequest = String::Format("# {0:F0} {1:F1} 1 #", SM_VC_->Steering, SM_VC_->Speed);

    }
    else {
        vcRequest = String::Format("# {0:F0} {1:F1} 0 #", SM_VC_->Steering, SM_VC_->Speed);

    }

    //Console::WriteLine("VC vcRequest Generated: {0}", vcRequest);
    SM_VC_->watchDogFlag = !SM_VC_->watchDogFlag;

    return error_state::SUCCESS;
}

bool VC::getShutdownFlag() {
    return (SM_TM_->shutdown & bit_VC);
}

error_state VC::connect(String^ hostName, int portNumber) {
    //Console::WriteLine("Connecting to Vehicle Control Module");

    Client = gcnew TcpClient(hostName, portNumber);
    Stream = Client->GetStream();
    Client->NoDelay = true;
    Client->ReceiveTimeout = 500;
    Client->SendTimeout = 500;
    Client->ReceiveBufferSize = 1024;
    Client->SendBufferSize = 1024;

    SendData = gcnew array<unsigned char>(64);
    ReadData = gcnew array<unsigned char>(2048);

    String^ Request = "5267332\n";
    SendData = Encoding::ASCII->GetBytes(Request);
    Stream->Write(SendData, 0, SendData->Length);
    System::Threading::Thread::Sleep(10);
    String^ Response = Encoding::ASCII->GetString(ReadData); 
    //Console::WriteLine(Response); 
    //No response expected?
    Console::WriteLine("VC Authenticated");
    return error_state::SUCCESS;
}

error_state VC::communicate() {
    SendData = Text::Encoding::ASCII->GetBytes(vcRequest);

    if (Stream->CanWrite) {
        Stream->Write(SendData, 0, SendData->Length);
        //Console::WriteLine("VC  Sent vcRequest: {0}", vcRequest);
        return error_state::SUCCESS;
    }
    else {
        Console::WriteLine("VC stream failed to send Request.");
        return error_state::ERR_CONNECTION;
    }
}

error_state VC::processHeartbeats() {
    if ((SM_TM_->heartbeat & bit_VC) == 0) {
        SM_TM_->heartbeat |= bit_VC;
        Watch->Restart();
    }
    else if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
        Console::WriteLine("VC Module timeout. Shutting down");
        shutdownModules();
        return error_state::ERR_TMM_FAILURE;
    }
    return error_state::SUCCESS;
}



void VC::shutdownModules() {
    // Stop vehicle
    Monitor::Enter(SM_VC_->lockObject);
    SM_VC_->Speed = 0;
    SM_VC_->Steering = 0;
    Monitor::Exit(SM_VC_->lockObject);

    SM_TM_->shutdown |= bit_VC;
    // Close cons
    if (Client && Client->Connected) {
        Stream->Close();
        Client->Close();
    }

    //Console::WriteLine("VC shutdownModules called");
}
