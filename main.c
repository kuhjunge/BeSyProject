// Test ausführung
#include "ftl.h"

uint8_t myState[2 * STATEBLOCKSIZE], *statePtr;
uint8_t myData[16], *dataPtr;
uint16_t count;
flash_t* ssd;

int main(int argc, char *argv[]) {
	FL_resetFlash(); // Start der Simulation

	ssd = mount(&myData);
	printf("Mount");
	writeBlock(ssd, 1, 'c');
	printf("Write");
	readBlock(ssd, 1, 'c');
	printf("Read %c", myData[0]);
	//unmount(&myData);
	printf("Unmount");
	printf("Test Ende");
}