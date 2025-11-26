// Display.cpp
#include "Display.h"


using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;


Display::Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser) {
    SM_TM_ = SM_TM;
    SM_Laser_ = SM_Laser;
    
    connect(DISPLAY_ADDRESS, 28000);
    Watch = gcnew Stopwatch();
    
}


error_state Display::setupSharedMemory() {
    return error_state::SUCCESS;
}


void Display::threadFunction() {
    //Console::WriteLine("Display threadFunction starting");
    SM_TM_->ThreadBarrier->SignalAndWait();

    // Start the stopwatch
    Watch->Start();

    // Start thread loop
    while (!getShutdownFlag()) {
        //Console::WriteLine("Display threadFunction is running.");
        processHeartbeats();
        communicate();
        Thread::Sleep(20);
    }
    shutdownModules();
    Console::WriteLine("Display shutdown");
}

error_state Display::processHeartbeats() {
    if ((SM_TM_->heartbeat & bit_DISPLAY) == 0) {
        SM_TM_->heartbeat |= bit_DISPLAY;
        Watch->Restart(); 
    }
    else if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
        //Console::WriteLine("Display module timed out. Shutting down.");
        shutdownModules();  // Shut down heartbeat fails
        return error_state::ERR_TMM_FAILURE;
    }
    return error_state::SUCCESS;
}

void Display::shutdownModules() {
    SM_TM_->shutdown |= bit_DISPLAY;

    if (Client && Client->Connected) {
        Stream->Close();
        Client->Close();
    }
    if (!Client->Connected) {
        Console::WriteLine("Display Module timeout. Shutting down");
        SM_TM_->shutdown |= bit_DISPLAY;
    }

    //Console::WriteLine("Display shutDownModule");
}

bool Display::getShutdownFlag() {
    return SM_TM_->shutdown & bit_DISPLAY;
}

error_state Display::connect(String^ hostName, int portNumber) {
    //Console::WriteLine("Connecting to Display server");
    // Lec W8.2
    Client = gcnew TcpClient(hostName, portNumber);
    Stream = Client->GetStream();
    Client->NoDelay = true;
    Client->ReceiveTimeout = 500;
    Client->SendTimeout = 500;
    Client->ReceiveBufferSize = 1024;
    Client->SendBufferSize = 1024;

    SendData = gcnew array<unsigned char>(64);
    ReadData = gcnew array<unsigned char>(2024);

    return error_state::SUCCESS;
}


error_state Display::communicate() {
    //Console::WriteLine("Communicating with Display server");
    sendDisplayData();
    return error_state::SUCCESS;
}


error_state Display::processSharedMemory() {
    return error_state::SUCCESS;
}

void Display::sendDisplayData() {
    // Serialize the data arrays to a byte array
        //(format required for sending)
    Monitor::Enter(SM_Laser_->lockObject);
    array<Byte>^ dataX = gcnew array<Byte>(SM_Laser_->x->Length * sizeof(double));
    Buffer::BlockCopy(SM_Laser_->x, 0, dataX, 0, dataX->Length);
    array<Byte>^ dataY = gcnew array<Byte>(SM_Laser_->y->Length * sizeof(double));
    Buffer::BlockCopy(SM_Laser_->y, 0, dataY, 0, dataY->Length);
    Monitor::Exit(SM_Laser_->lockObject);
    // Send byte data over connection
    Stream->Write(dataX, 0, dataX->Length);
    Thread::Sleep(10);
    Stream->Write(dataY, 0, dataY->Length);
}
