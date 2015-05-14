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

	for (j = 0; j < 10000; j++){ // Bug: bei 2048 kommt es zu Zugriffs und Schreibfehlern
		r = 1 + rand() % 495;// 31 x 16 = 496; 1 Block spare
		printf("Write\n");
		for (i = 0; i < 16; i++)
			myData[i] = (uint8_t)(i + 65);

		writeBlock(ssd, r, &myData);

		printf("Read\n");
		readBlock(ssd, r, &myRetData);
		if (myRetData[0] != 'A'){
			printf("Lesefehler\n\n");
			printerr(ssd);
		}
		for (i = 0; i < 16; i++)
		{
			printf("%c", myRetData[i]);
		}
		printf("\n");
	}
	printf("Unmount\n");
	//unmount(&myData);
	printerr(ssd);
	printf("Test Ende");
}