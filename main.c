// Test ausführung
#include "ftl.h"
#include <time.h>

flash_t* ssd;
flashMem_t flMe;


uint8_t myState[2 * STATEBLOCKSIZE], *statePtr;
uint8_t myData[16], myRetData[16];
uint16_t count;


int main(int argc, char *argv[]) {
	int i;
	int j;
	int r;

	srand(time(NULL));

	printf("Mount %i \n", MAPPING_TABLE_SIZE);
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);

	for (j = 0; j < 512; j++){
		r = rand() % 512;
		printf("Write\n");
		for (i = 0; i < 16; i++)
			myData[i] = (uint8_t)(i + 65);

		writeBlock(ssd, r, &myData);

		printf("Read\n");
		readBlock(ssd, r, &myRetData);
		for (i = 0; i < 16; i++)
		{
			printf("%c", myRetData[i]);
		}
		printf("\n");
	}
	printf("Unmount\n");
	//unmount(&myData);

	printf("Test Ende");
}