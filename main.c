// Test ausführung
#include "ftl.h"
#include <time.h>

flash_t* ssd;
flashMem_t flMe;
#define TEST_COUNT 10000

uint8_t myData[16], myRetData[16];
uint16_t count;

void writeData(int start, int amount, int rnd, int tc){
	int i;
	int j;
	uint32_t r;
	int k = 0;

	for (j = 0; j < tc; j++){
		if (rnd > 0){
			r = 1 + start + rand() % amount;// 31 x 16 = 496; 1 Block spare
		}
		else {
			r = 1 + start + k;
			k++;
			if (k  >= amount) {
				k = 0;
			}
		}
		printf("Write\n");
		for (i = 0; i < 16; i++)
			myData[i] = (uint8_t)(i + 65);

		writeBlock(ssd, r, myData);

		printf("Read\n");
		readBlock(ssd, r, myRetData);
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
}

void load_test_Random_Full(){

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	writeData(0, 495, 1, TEST_COUNT);

	printf("Unmount\n");
	//unmount(&myData);
	printerr(ssd);
	printf("Test Ende");
}

void overload_test_Random(){

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	writeData(0, 496, 1, TEST_COUNT);

	printf("Unmount\n");
	//unmount(&myData);
	printerr(ssd);
	printf("Test Ende");
}


void load_test_Random_Light(){
	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);

	writeData(0, 120, 1, TEST_COUNT);

	printf("Unmount\n");
	//unmount(&myData);
	printerr(ssd);
	printf("Test Ende");
}

void load_test_OS(){
	int i;

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);

	writeData(0, 119, 0, 120); // Installation OS
	for (i = 0; i < 20; i++){
		writeData(119, 239, 1, 200); // File Usage
		writeData(358, 70, 1, 1000); // Temp Data Usage
		writeData(428, 30, 1, 10000); // Extrem Data Usage
	}

	printerr(ssd);
	printf("Test Ende");
}

// Test ergänzen der Zahlen 1 - n mit Index 1 - n in den Speicher schreibt und anschließend wieder ausliest und prüft
void mapping_test(){
	uint32_t i, j;
	uint8_t checkvalue;
	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	printf("Write Prev\n");
	writeData(0, 495, 1, BLOCK_COUNT * BLOCKSEGMENTS); // Speicher vorbeschreiben
	printf("Write\n"); // Sortiertes Schreiben
	for (i = 1; i <=( BLOCK_COUNT - SPARE_BLOCKS)* BLOCKSEGMENTS-1 ; i++){
		for (j = 0; j < LOGICAL_BLOCK_DATASIZE; j++){
			uint8_t checkvalue = (uint8_t)((i * j) % 255);
			myData[j] = checkvalue;
		}
		writeBlock(ssd, i, myData);
	}
	printf("Read\n"); // Sortiertes lesen
	for (i = 1; i <= (BLOCK_COUNT - SPARE_BLOCKS)* BLOCKSEGMENTS -1 ; i++){

		readBlock(ssd, i, myRetData);
		for (j = 0; j < LOGICAL_BLOCK_DATASIZE; j++){
			uint8_t checkvalue = (uint8_t)((i * j) % 255);
			if (myRetData[j] != checkvalue){
				printf("Mappingfehler\n");
				// printerr(ssd);
			}
		}
	}

	printf("Unmount\n");
	//unmount(&myData);
	printf("Mappingtest erfolgreich\n");
	printerr(ssd);
}

int main(int argc, char *argv[]) {
	srand((unsigned int)time(NULL));

	//load_test_Random_Light(); // Wenige Random Datensätze die kreuz und quer geschrieben werden (Testet Block Verteilung bei wenig geschriebenen Datensätzen)

	//load_test_Random_Full(); // Komplette Festplatte wird mit Random Datensätzen vollgeschrieben (Extremwerttest)

	//overload_test_Random(); // Was passiert, wenn die Festplatte zu voll geschrieben wird ?

	mapping_test(); // Prüft das Mapping auf Richtigkeit  (Testbeispiel für [TC11] Algorithmus)

	//load_test_OS(); // Sorgt für hohe schreibrate und lässt teilweise komplette Blöcke unberührt (Testbeispiel für [TC11] ), Läuft eine Weile
}