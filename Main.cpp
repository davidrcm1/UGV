#include "TMM.h"

int main(void) {
	ThreadManagement^ myTMM = gcnew ThreadManagement();
	myTMM->setupSharedMemory();
	myTMM->threadFunction();

	Console::ReadKey();
	Console::ReadKey();

	
	
	return 0;
}