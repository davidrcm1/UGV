#include "GNSS.h"
// 123
using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

GNSS::GNSS(SM_ThreadManagement^ SM_TM, SM_GPS^ SM_GPS) {
    SM_Gps_ = SM_GPS;
    SM_TM_ = SM_TM;

    connect(WEEDER_ADDRESS, 24000);
    Watch = gcnew Stopwatch();
}

error_state GNSS::setupSharedMemory() {
    return error_state::SUCCESS;
}

void GNSS::threadFunction() {
    //Console::WriteLine("GNSS threadFunction starting");
    SM_TM_->ThreadBarrier->SignalAndWait();
    Watch->Start();

    while (!getShutdownFlag()) {
        //Console::WriteLine("GNSS threadFunction is running.");
        processHeartbeats();
        if (communicate() == error_state::SUCCESS && checkData() == error_state::SUCCESS ) {
            processSharedMemory();
        }

        Thread::Sleep(20);
    }
    shutdownModules();
    Console::WriteLine("GNSS shutdown");
}

error_state GNSS::processSharedMemory() {
    //Console::WriteLine("Entered processSharedMemory");
    // Lec 7.4 allocate to struct
    GNSSData gpsPackage{};
    unsigned char* BytePtr = (unsigned char*)&gpsPackage;

    //Console::WriteLine("Copying data into GNSSData structure");
    for (int i = 0; i < sizeof(GNSSData); i++) {
        *(BytePtr + i) = ReadData[i];
    }

    // Update shared memory
    Monitor::Enter(SM_Gps_->lockObject);
    SM_Gps_->Northing = gpsPackage.Northing;
    SM_Gps_->Easting = gpsPackage.Easting;
    SM_Gps_->Height = gpsPackage.Height;
    receivedCRC = gpsPackage.CRC;
    Monitor::Exit(SM_Gps_->lockObject);

    Console::WriteLine("Northing: {0:F4}", SM_Gps_->Northing);
    Console::WriteLine("Easting: {0:F4}", SM_Gps_->Easting);
    Console::WriteLine("Height: {0:F4}", SM_Gps_->Height);
    Console::WriteLine("CRC: 0x{0:X}", receivedCRC);
    
    return error_state::SUCCESS;
}

bool GNSS::getShutdownFlag() {
    return (SM_TM_->shutdown & bit_GPS) != 0;
}

error_state GNSS::connect(String^ hostName, int portNumber) {
    // same as laser
    Console::WriteLine("Connecting to GNSS ");

    Client = gcnew TcpClient(hostName, portNumber);
    Stream = Client->GetStream();
    Client->NoDelay = true;
    Client->ReceiveTimeout = 500;
    Client->SendTimeout = 500;

    Client->ReceiveBufferSize = 1024;
    Client->SendBufferSize = 1024;

    ReadData = gcnew array<unsigned char>(2048);

    return error_state::SUCCESS;
}

error_state GNSS::communicate() {
    unsigned int header = 0;
    unsigned char byte = 0;

    if (Stream->DataAvailable) {
        // lec 7.5
        // exclude header
        do {
            byte = Stream->ReadByte();
            header = (header << 8) | byte;
        } while (header != 0xAA44121C);
        
        int byteIdx = Stream->Read(ReadData, 0, ReadData->Length);

        // check size of package
        if (byteIdx != 108 ) {
            return ERR_NO_DATA;
        }
        /*
        for (int i = 0; i < byteIdx; i++) {
            Console::Write("0x{0:X2} ", ReadData[i]);
        }
        Console::WriteLine();
        */
        
        return error_state::SUCCESS;
    }
    else {
        return error_state::ERR_NO_DATA;
    }
}

error_state GNSS::processHeartbeats() {
    if ((SM_TM_->heartbeat & bit_GPS) == 0) {
        SM_TM_->heartbeat |= bit_GPS;
        Watch->Restart();
    }

    else if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
        Console::WriteLine("GNSS Module timeout. Shutting down");
        shutdownModules();
        return error_state::ERR_TMM_FAILURE;
    }
    return error_state::SUCCESS;
}

error_state GNSS::checkData()
{   
    GNSSData gpsPackage{};
    unsigned char* BytePtr = (unsigned char*)&gpsPackage;
    unsigned char gnssData[108] = { 0xAA, 0x44, 0x12, 0x1C };

    //Console::WriteLine("Copying data into GNSSData structure");
    for (int i = 0; i < sizeof(GNSSData); i++) {
        *(BytePtr + i) = ReadData[i];
        if (i < 104) {
            gnssData[i + 4] = ReadData[i];
        }
    }
    unsigned long CRC = gpsPackage.CRC;
    
    /*
    for (int i = 0; i < 108; i++) {
        Console::Write("0x{0:X2} ", gnssData[i]);
    }
    Console::WriteLine();
    */
    
    unsigned long calculatedCRC = CalculateBlockCRC32(sizeof(gnssData), gnssData);
    if (calculatedCRC == CRC) {
        Console::WriteLine(" Calculated: 0x{0:X}, Received: 0x{1:X}", calculatedCRC, CRC);
        return error_state::SUCCESS;
    }
    else {
        Console::WriteLine("ERROR = Calculated: 0x{0:X}, Expected: 0x{1:X}", calculatedCRC, CRC);
        return error_state::ERR_INVALID_DATA;
    }
}

void GNSS::shutdownModules() {
    //Console::WriteLine("GNSS shutdownModule");
    SM_TM_->shutdown |= bit_GPS;

    if (Client && Client->Connected) {
        Stream->Close();
        Client->Close();
    }


}

unsigned long GNSS::CRC32Value(int i)
{
    int j;
    unsigned long ulCRC;
    ulCRC = i;
    for (j = 8; j > 0; j--)
    {
        if (ulCRC & 1)
            ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
        else
            ulCRC >>= 1;
    }
    return ulCRC;
}


unsigned long GNSS::CalculateBlockCRC32(
    unsigned long ulCount, /* Number of bytes in the data block */
    unsigned char* ucBuffer) /* Data block */
{
    unsigned long ulTemp1;
    unsigned long ulTemp2;
    unsigned long ulCRC = 0;
    while (ulCount-- != 0)
    {
        ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
        ulTemp2 = CRC32Value(((int)ulCRC ^ *ucBuffer++) & 0xff);
        ulCRC = ulTemp1 ^ ulTemp2;
    }
    return(ulCRC);
}