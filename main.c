// Test ausführung
#include "ftl.h"

flash_t* ssd;
flashMem_t flMe;


uint8_t myState[2 * STATEBLOCKSIZE], *statePtr;
uint8_t myData[16], myRetData[16];
uint16_t count;


int main(int argc, char *argv[]) {
	int i;

	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	printf("Mount");
	for (i = 0; i<16; i++)
		myData[i] = (uint8_t)(i % 10 + 65);

	writeBlock(ssd, 1, &myData);
	printf("Write");
	readBlock(ssd, 1, &myRetData);
	for (i = 0; i < 4; i++)
	{
		printf("%c", myData[i]);
	}
	//unmount(&myData);
	printf("Unmount");
	printf("Test Ende");
}