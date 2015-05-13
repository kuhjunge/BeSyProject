// Test ausführung
#include "ftl.h"

flash_t* ssd;
flashMem_t flMe;


uint8_t myState[2 * STATEBLOCKSIZE], *statePtr;
uint8_t myData[16], myRetData[16];
uint16_t count;


int main(int argc, char *argv[]) {
	int i;
	int j;
	printf("Mount\n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);

	for (j = 0; j < 512; j++){
		printf("Write\n");
		for (i = 0; i < 16; i++)
			myData[i] = (uint8_t)(i + 65);

		writeBlock(ssd, j, &myData);

		printf("Read\n");
		readBlock(ssd, j, &myRetData);
		for (i = 0; i < 4; i++)
		{
			printf("%c", myRetData[i]);
		}
		printf("\n");
	}
	printf("Unmount\n");
	//unmount(&myData);


	printf("Test Ende");
}