#pragma once


#include "NetworkedModule.h"
#using <System.dll>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class Display : public NetworkedModule
{
public:
    Display() {};
    Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser);

    error_state setupSharedMemory();
    void threadFunction() override;
    error_state processHeartbeats();
    void shutdownModules();
    bool getShutdownFlag() override;
    error_state processSharedMemory() override;

    virtual error_state connect(String^ hostName, int portNumber) override;
    virtual error_state communicate() override;

    void sendDisplayData();

    ~Display() {}

private:
    // inhereted functions
    // ThreadManagement already in UGV
    // TcpClient^ Client;
    // NetworkStream^ Stream;
    // array<unasigned chat>^ ReadData;
    array<unsigned char>^ SendData;
    SM_Laser^ SM_Laser_;
    Stopwatch^ Watch;
};
