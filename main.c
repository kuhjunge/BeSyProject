// Test ausführung
#include "ftl.h"
#include <time.h>

flash_t* ssd;
flashMem_t flMe;
#define TEST_COUNT 20000

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

void test_write_n_locigalBlocks(int value){
	int i;
	int k = 0;

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}

	for (i = 0; i < 16; i++)
		myData[i] = (uint8_t)(i + 65);

	for(i = 0; i < TEST_COUNT/value; i++){
		for(k = 1; k <= value; k++){
			writeBlock(ssd, k, myData);
			readBlock(ssd, k,myRetData);
			if(myData[0] != myRetData[0]){
				printf("Fehler beim Lesen\n");
			}
		}
	}

	printf("Unmount\n");
	ssd = unmount(ssd);
	printerr(ssd);
	printf("test_write_n_logicalBlocks() beendet");
}


void test_write_one_logicalBlock(){
	int i;
	int k = 0;

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}

	for (i = 0; i < 16; i++)
		myData[i] = (uint8_t)(i + 65);

	for(i = 0; i < TEST_COUNT; i++){
		writeBlock(ssd, 1, myData);
		readBlock(ssd, 1, myRetData);
		if(myData[0] != myRetData[0]){
			printf("Fehler beim Lesen\n");
		}
	}

	printf("Unmount\n");
	ssd = unmount(ssd);
	printerr(ssd);
	printf("test_write_one_logicalBlock() beendet");
}
void load_test_Random_Full(){

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(0, 495, 1, TEST_COUNT);

	printf("Unmount\n");
	ssd = unmount(ssd);
	printerr(ssd);
	printf("load_test_Random_Full Ende");
}

void overload_test_Random(){

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(0, 480, 0, 480);

	printf("Unmount\n");
	ssd = unmount(ssd);
	printerr(ssd);
	printf("overload_test_Random Ende");
}

void mount_test_Light(){
	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(0, 120, 1, ((BLOCK_COUNT - SPARE_BLOCKS)* BLOCKSEGMENTS - 1) * 2);

	printf("Unmount\n");
	ssd = unmount(ssd);
	ssd = NULL;
	ssd = mount(&flMe);
	printerr(ssd);
	printf("mount_test_Light Ende");
}

void load_test_Random_Light(){
	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe); 
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(0, 120, 1, TEST_COUNT);

	printf("Unmount\n");
	ssd = unmount(ssd);
	printf("load_test_Random_Light Ende");
	printerr(ssd);
}

void load_test_OS(){
	int i;

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(0, 119, 0, 120); // Installation OS
	for (i = 0; i < 20; i++){
		writeData(119, 239, 1, 200); // File Usage
		writeData(358, 70, 1, 1000); // Temp Data Usage
		writeData(428, 30, 1, 10000); // Extrem Data Usage
	}
	printf("Unmount\n");
	ssd = unmount(ssd);
	printerr(ssd);
	printf("load_test_OS Ende");
}

// Test ergänzen der Zahlen 1 - n mit Index 1 - n in den Speicher schreibt und anschließend wieder ausliest und prüft
void mapping_test(){
	uint32_t i, j;
	uint8_t checkvalue;
	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	printf("Write Prev\n");
	writeData(0, 495, 1, BLOCK_COUNT * BLOCKSEGMENTS); // Speicher vorbeschreiben		
	//writeData(0, 432, 1, TEST_COUNT); // Speicher vorbeschreiben		
	printf("Write\n"); // Sortiertes Schreiben	
	for (i = 1; i <=( BLOCK_COUNT - SPARE_BLOCKS)* BLOCKSEGMENTS-1 ; i++){
	//for (i = 1; i < 432 ; i++){
		for (j = 0; j < LOGICAL_BLOCK_DATASIZE; j++){
			checkvalue = (uint8_t)((i * j) % 255);
			myData[j] = checkvalue;
		}
		writeBlock(ssd, i, myData);
	}
	printf("Read\n"); // Sortiertes lesen
	for (i = 1; i <= (BLOCK_COUNT - SPARE_BLOCKS)* BLOCKSEGMENTS -1 ; i++){
	//for (i = 1; i < 432 ; i++){
		readBlock(ssd, i, myRetData);
		for (j = 0; j < LOGICAL_BLOCK_DATASIZE; j++){
			checkvalue = (uint8_t)((i * j) % 255);
			if (myRetData[j] != checkvalue){
				printf("Mappingfehler\n");
				// printerr(ssd);
			}
		}
	}
	printf("Unmount\n");
	ssd = unmount(ssd);
	printf("Mappingtest erfolgreich\n");
	printerr(ssd);
}

int main(int argc, char *argv[]) {
	srand((unsigned int)time(NULL));

	//schreibe wiederholt einen Block
	//test_write_one_logicalBlock(); 

	//schreibe wiederholt verschiedene Datensätze	
	// 1 bis maximal 479 => 2 Blöcke Spare; d.h. 512(32*16) - 32 - 1(TODO warum?)
	test_write_n_locigalBlocks( 479 );	
	//test_write_n_locigalBlocks((FL_getBlockCount() - SPARE_BLOCKS )* BLOCKSEGMENTS);

	//mount_test_Light();

	// Wenige Random Datensätze die kreuz und quer geschrieben werden (Testet Block Verteilung bei wenig geschriebenen Datensätzen)
	//load_test_Random_Light(); 

	//load_test_Random_Full(); // Komplette Festplatte wird mit Random Datensätzen vollgeschrieben (Extremwerttest)

	//mapping_test(); // Prüft das Mapping auf Richtigkeit  (Testbeispiel für [TC11] Algorithmus)

	//load_test_OS(); // Sorgt für hohe schreibrate und lässt teilweise komplette Blöcke unberührt (Testbeispiel für [TC11] ), Läuft eine Weile

	//overload_test_Random(); // Was passiert, wenn die Festplatte zu voll geschrieben wird ?


}