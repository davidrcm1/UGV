// TMM.cpp
#include "TMM.h"

error_state ThreadManagement::setupSharedMemory() {
	SM_TM_ = gcnew SM_ThreadManagement();
	SM_Laser_ = gcnew SM_Laser();
	SM_Gps_ = gcnew SM_GPS();
    SM_Vc_ = gcnew SM_VehicleControl;

	return error_state::SUCCESS;
}

error_state ThreadManagement::processSharedMemory() {
	return error_state::SUCCESS;
}

void ThreadManagement::shutdownModules() {
    SM_TM_->shutdown = 0xff;
}

bool ThreadManagement::getShutdownFlag() {
	//return true;
    return (SM_TM_->shutdown & bit_TM) != 0;
}

void ThreadManagement::threadFunction() {

    // controller
    // IRL
    ControllerInterface controller = ControllerInterface(1, 0);
    // Sim
    //ControllerInterface controller =  ControllerInterface(0, 1);
    
    //Console::WriteLine("ControllerInterface IsConnected(): {0}", controller.IsConnected());
    
    ThreadPropertiesList = gcnew array<ThreadProperties^> {
        gcnew ThreadProperties{ gcnew ThreadStart(gcnew Laser(SM_TM_, SM_Laser_), &Laser::threadFunction), true, bit_LASER, "Laser thread" },
        gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(SM_TM_, SM_Gps_), &GNSS::threadFunction), false, bit_GPS, "GNSS thread"),
        gcnew ThreadProperties{ gcnew ThreadStart(gcnew Display(SM_TM_, SM_Laser_), &Display::threadFunction), true, bit_DISPLAY, "Display thread" },
        gcnew ThreadProperties(gcnew ThreadStart(gcnew VC(SM_TM_, SM_Vc_), &VC::threadFunction), true, bit_VC, "VC thread"),
        gcnew ThreadProperties(gcnew ThreadStart(gcnew Controller(SM_TM_, SM_Vc_, &controller), &Controller::threadFunction), true, bit_CONTROLLER, "Controller thread"),
        gcnew ThreadProperties(gcnew ThreadStart(gcnew CrashAvoidance(SM_TM_, SM_Laser_, SM_Vc_), &CrashAvoidance::threadFunction), true, bit_CRASHAVOIDANCE,"CrashAvoidance thread"),
    };

    ThreadList = gcnew array<Thread^>(ThreadPropertiesList->Length);
    StopwatchList = gcnew array<Stopwatch^>(ThreadPropertiesList->Length);
    SM_TM_->ThreadBarrier = gcnew Barrier(ThreadPropertiesList->Length + 1);

    // Start all the threads
    for (int i = 0; i < ThreadPropertiesList->Length; i++) {
        StopwatchList[i] = gcnew Stopwatch();
        ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
        ThreadList[i]->Start();
    }

    SM_TM_->ThreadBarrier->SignalAndWait();

    // Start all the stopwatches
    for (int i = 0; i < ThreadList->Length; i++) {
        StopwatchList[i]->Start();
    }

    // Start the thread loop
    while (!(Console::KeyAvailable && Console::ReadKey(true).Key == ConsoleKey::Q) && !getShutdownFlag()) {
        //Console::WriteLine("TMM Thread running");

        processHeartbeats();
        Thread::Sleep(100);
    }

    // End of thread loop
    // Shutdown all threads
    shutdownModules();

    for (int i = 0; i < ThreadPropertiesList->Length; i++) {
        ThreadList[i]->Join();
    }

    Console::WriteLine("TMM shutdown");
}

error_state ThreadManagement::processHeartbeats()
{
    for (int i = 0; i < ThreadList->Length; i++) {
        if (SM_TM_->heartbeat & ThreadPropertiesList[i]->BitID) {
            SM_TM_->heartbeat ^= ThreadPropertiesList[i]->BitID;
            StopwatchList[i]->Restart();
        }
        else {
            if (StopwatchList[i]->ElapsedMilliseconds > CRASH_LIMIT) {
                if (ThreadPropertiesList[i]->Critical) {
                    Console::WriteLine(ThreadList[i]->Name + " failure. Shutting down all threads.");
                    shutdownModules();
                    return ERR_CRITICAL_PROCESS_FAILURE;
                }
                else {
                    Console::WriteLine(ThreadList[i]->Name + " failed. Attempting to restart.");
                    ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
                    SM_TM_->ThreadBarrier = gcnew Barrier(1);
                    ThreadList[i]->Start();
                }
            }
        }
    }
	return error_state::SUCCESS;
}