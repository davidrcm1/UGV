// Laser.cpp
#include "Laser.h"
#using <System.dll>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;


Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser) {
    SM_Laser_ = SM_Laser;
    SM_TM_ = SM_TM;

    Watch = gcnew Stopwatch();
    connect(WEEDER_ADDRESS, 23000);
}

// Set up shared memory SETUP BY TMM?
error_state Laser::setupSharedMemory() {
    return error_state::SUCCESS;
}


void Laser::threadFunction() {
    //Console::WriteLine("Laser threadFunction is starting");
    Watch = gcnew Stopwatch();
    SM_TM_->ThreadBarrier->SignalAndWait();
    Watch->Start();

    while (!getShutdownFlag())
    {
        //Console::WriteLine("Laser threadFuction is running.");
        processHeartbeats();
        if (communicate() == error_state::SUCCESS && checkData() == error_state::SUCCESS)
        {
            processSharedMemory();
        }
        Thread::Sleep(20);
    }

    shutdownModules();
    Console::WriteLine("Laser shutdown");
}


error_state Laser::processHeartbeats() {
    // Check if TMM has put the bit down 
    if ((SM_TM_->heartbeat & bit_LASER) == 0) {
        // Set Laser bit back up to show Laser active
        SM_TM_->heartbeat |= bit_LASER;
        Watch->Restart();
    }
    // TMM not responding, shut down.
    else if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
        Console::WriteLine("Laser Module timeout. Shutting down");
        shutdownModules();
        return error_state::ERR_TMM_FAILURE;
    }
    return error_state::SUCCESS;
}

void Laser::shutdownModules() {
    SM_TM_->shutdown |= bit_LASER;

    if (Client && Client->Connected) {
        Client->Close();
    }
    //Console::WriteLine("Laser shutdownModule");
}

bool Laser::getShutdownFlag() {
    return (SM_TM_->shutdown & bit_LASER) != 0;
}

error_state Laser::connect(String^ hostName, int portNumber) {
    Console::WriteLine("Connecting to Laser");
    // Lec 3.5
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
    Stream->Write(SendData, 0, SendData->Length); // write to laser

    System::Threading::Thread::Sleep(10);
    Stream->Read(ReadData, 0, ReadData->Length); // response from laser
    String^ Response = Encoding::ASCII->GetString(ReadData);

    Console::WriteLine(Response); // OK if verified
    if (Response->Contains("OK")) {
        Console::WriteLine("Laser Authenticated");
        return error_state::SUCCESS;
    }
    else {
        Console::WriteLine("Laser Authentication failed");
        // shutdown if fail (crit thread)
        shutdownModules();
        return error_state::ERR_CONNECTION;
    }

}

error_state Laser::communicate() {
    String^ Request = "\x02sRN LMDscandata\x03";
    SendData = Encoding::ASCII->GetBytes(Request);

    if (Stream->CanWrite) {
        Stream->Write(SendData, 0, SendData->Length);

        int byteIdx = Stream->Read(ReadData, 0, ReadData->Length);

        if (ReadData[0] == 0x02 && ReadData[byteIdx - 1] == 0x03) {
            /*
            for (int i = 1; i < byteIdx - 1; i++) {
                Console::Write("0x{0:X2} ", ReadData[i]);
            }
            Console::WriteLine();
            */
            return error_state::SUCCESS;
        }
        else {
            Console::WriteLine("Laser data ignored");
            return error_state::ERR_INVALID_DATA;
        }
    }
    else {
        Console::WriteLine("Laser communicate: Stream is not writable.");
        return error_state::ERR_CONNECTION;
    }
}

error_state Laser::checkData() {
    // Lec 7.1
    String^ Response = Encoding::ASCII->GetString(ReadData);
    array<String^>^ dataPoint = Response->Split(' ');

    int string = System::Convert::ToInt32(dataPoint[5], 16);
    int numDataPoints = Convert::ToInt32(dataPoint[25], 16);
    if (numDataPoints != 361) {
        Console::WriteLine("Invalid data pt");
        return error_state::ERR_INVALID_DATA;
    }
    // device status: 0 = OK
    if (string != 0) {
        Console::WriteLine("Invalid device status");

        return error_state::ERR_INVALID_DATA;
    }
    else {
        return error_state::SUCCESS;
    }

}

error_state Laser::processSharedMemory() {
    // Lec 7.1
    Monitor::Enter(SM_Laser_->lockObject); 
    String^ Response = Encoding::ASCII->GetString(ReadData);
    array<String^>^ dataPoint = Response->Split(' ');
    int dataIndex = Convert::ToInt32(dataPoint[25], 16);
    for (int i = 0; i < dataIndex; ++i) { 
        double range = Convert::ToInt32(dataPoint[26 + i], 16);

        SM_Laser_->x[i] = range * Math::Sin((90.0 + i * 0.5) * Math::PI / 180.0);
        SM_Laser_->y[i] = -range * Math::Cos((90.0 + i * 0.5) * Math::PI / 180.0);
    }
    Monitor::Exit(SM_Laser_->lockObject);

    return error_state::SUCCESS;
}