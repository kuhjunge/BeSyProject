// Test ausf�hrung
#include "ftl.h"
#include <time.h>

flash_t* ssd;
flashMem_t flMe;
#define TEST_COUNT 20000

uint8_t myData[16], myRetData[16];
uint16_t count;
int blocksegment = 16;

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
		printf(".");
		for (i = 0; i < 16; i++)
			myData[i] = (uint8_t)(i + 65);

		writeBlock(ssd, r, myData);

		//printf("*");
		readBlock(ssd, r, myRetData);
		if (myRetData[0] != 'A'){
			printf("Lesefehler\n\n");
			printerr(ssd);
		}
		/*for (i = 0; i < 16; i++)
		{
			printf("%c", myRetData[i]);
		}
		printf("\n");*/
	}
}

void test_write_random_n_locigalBlocks( uint16_t blocks, uint16_t range, uint16_t charNumber){
	uint16_t i, j, l;
	uint16_t k = 0;

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}

	for (i = 0; i < charNumber; i++)
		myData[i] = (uint8_t)(i + 65);

	for(i = 0; i < range; i++)		{
			//zuf�llige Zeichenfolge
			for (j = 0; j < charNumber; j++){
				myData[j] = (uint8_t)(65 + rand() % 20);
			}
			//Zufall logischer Block
			l = (rand() % blocks) ;
			printf("%i/%i\n",i+1,range);
			//schreibe
			writeBlock(ssd, l, myData);
			//lese
			readBlock(ssd, l, myRetData);
			//�berpr�fe
			for(k = 0; k < charNumber; k++){
				if(myData[k] != myRetData[k]){
					printf("Fehler beim Lesen nach %i Zugriffen\n",i);
					printerr(ssd);
					return;
				}
			}
		}

	printerr(ssd);
	printf("Unmount\n");
	ssd = unmount(ssd);	
	printf("test_write_n_logicalBlocks() beendet");
}

void test_write_n_locigalBlocks(uint16_t blocks, uint16_t range, uint16_t charNumber){
	uint16_t i, l, j;
	uint16_t k = 0;

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	
	for(i = 0; i < range; i++)
		for(l = 1; l <= blocks; l++){
			//zuf�llige Zeichenfolge
			for (j = 0; j < charNumber; j++){
				myData[j] = (uint8_t)(65 + rand() % 20);
			}
			printf("%i/%i\n",i+1,range);
			//schreibe
			writeBlock(ssd, l, myData);
			//lese
			readBlock(ssd, l, myRetData);
			//�berpr�fe
			for(k = 0; k < charNumber; k++){
				if(myData[k] != myRetData[k]){
					printf("Fehler beim Lesen nach %i Zugriffen\n",i);
					printerr(ssd);
					return;
				}
			}
		}

	printerr(ssd);
	printf("Unmount\n");
	ssd = unmount(ssd);	
	printf("test_write_n_logicalBlocks() beendet");
}


void test_write_one_logicalBlock(uint16_t range, uint16_t charNumber){
	uint16_t i,j;
	uint16_t k = 0;

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}

	
	for(i = 0; i < range; i++){
		//zuf�llige Zeichenfolge
		for (j = 0; j < charNumber; j++){
			myData[j] = (uint8_t)(65 + rand() % 20);
		}
		printf("%i/%i\n",i+1,range);
		//schreibe
		writeBlock(ssd, 1, myData);
		//lese
		readBlock(ssd, 1, myRetData);
		//�berpr�fe
		for(k = 0; k < charNumber; k++){
			if(myData[k] != myRetData[k]){
				printf("Fehler beim Lesen nach %i Zugriffen\n",i);
				printerr(ssd);
				return;
			}
		}
	}

	printerr(ssd);
	printf("Unmount\n");
	ssd = unmount(ssd);	
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

void mount_test_Light(uint16_t range, uint8_t charNumber){
	uint32_t i,k,j;
	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	
	for(i = 0; i < range; i++){
		//zuf�llige Zeichenfolge
		for (j = 0; j < charNumber; j++){
			myData[j] = (uint8_t)(65 + rand() % 20);
		}
	
		printf("write data %i/%i\n",i+1,range);

		//schreibe
		writeBlock(ssd, 1, myData);
	}

	printf("Unmount\n");
	ssd = unmount(ssd);
	ssd = NULL;
	ssd = mount(&flMe);
	printf("Mount \n");

	//lese
	readBlock(ssd, 1, myRetData);
	//�berpr�fe
	for(k = 0; k < charNumber; k++){
		if(myData[k] != myRetData[k]){
			printf("Fehler beim Lesen\n");
			printerr(ssd);
			return;
		}
	}
	for(i = 0; i < range; i++){
		//zuf�llige Zeichenfolge
		for (j = 0; j < charNumber; j++){
			myData[j] = (uint8_t)(65 + rand() % 20);
		}
	
		printf("write data %i/%i\n",i+1,range);

		//schreibe
		writeBlock(ssd, 1, myData);
	}
	//lese
	readBlock(ssd, 1, myRetData);
	//�berpr�fe
	for(k = 0; k < charNumber; k++){
		if(myData[k] != myRetData[k]){
			printf("Fehler beim Lesen\n");
			printerr(ssd);
			return;
		}
	}

	
	printerr(ssd);
	printf("Unmount\n");
	ssd = unmount(ssd);
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


// Test erg�nzen der Zahlen 1 - n mit Index 1 - n in den Speicher schreibt und anschlie�end wieder ausliest und pr�ft
void mapping_test(int multiplFactor){
	uint32_t i, j, k;
	uint8_t checkvalue;
	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	for (k = 0; k < multiplFactor; k++){
		printf("\nZyklus: %i\n", k);
		writeData(0, 480, 1, BLOCK_COUNT * blocksegment); // Speicher vorbeschreiben			
		printf("."); // Sortiertes Schreiben	
		for (i = 0; i <= (BLOCK_COUNT - SPARE_BLOCKS)* blocksegment; i++){
			for (j = 0; j < LOGICAL_BLOCK_DATASIZE; j++){
				checkvalue = (uint8_t)((i * j) % 255);
				myData[j] = checkvalue;
			}
			writeBlock(ssd, i, myData);
		}
		printf("*"); // Sortiertes lesen
		for (i = 0; i <= (BLOCK_COUNT - SPARE_BLOCKS)* blocksegment; i++){
			readBlock(ssd, i, myRetData);
			for (j = 0; j < LOGICAL_BLOCK_DATASIZE; j++){
				checkvalue = (uint8_t)((i * j) % 255);
				if (myRetData[j] != checkvalue){
					printf("Mappingfehler\n");
					printerr(ssd);
				}
			}
		}
	}
	printf("Unmount\n");
	printerr(ssd);
	ssd = unmount(ssd);
	printf("Mappingtest erfolgreich\n");
	
}

int main(int argc, char *argv[]) {
	srand((unsigned int)time(NULL));

	//schreibe wiederholt einen logischen Block
	//test_write_one_logicalBlock(20000, LOGICAL_BLOCK_DATASIZE); 

	//schreibe wiederholt verschiedene logische Bl�cke	
	// 1 bis maximal 480 => 2 Bl�cke Spare; d.h. 512(32*16) - 32 
	//test_write_n_locigalBlocks( 480, 100,LOGICAL_BLOCK_DATASIZE );		

	//schreibe wiederholt zuf�llige logische Bl�cke
	//test_write_random_n_locigalBlocks(480, 65533, LOGICAL_BLOCK_DATASIZE);

	//schreibe erst einen Block wiederholt; unmount, mount und �berpr�fe den Inhalt dieses Blocks, danach wieder schreiben und �berpr�fen
	//mount_test_Light(1000, LOGICAL_BLOCK_DATASIZE);

	// Wenige Random Datens�tze die kreuz und quer geschrieben werden (Testet Block Verteilung bei wenig geschriebenen Datens�tzen)
	//load_test_Random_Light(); 

	//load_test_Random_Full(); // Komplette Festplatte wird mit Random Datens�tzen vollgeschrieben (Extremwerttest)

	mapping_test(250); // Pr�ft das Mapping auf Richtigkeit  (Testbeispiel f�r [TC11] Algorithmus)

	//load_test_OS(); // Sorgt f�r hohe schreibrate und l�sst teilweise komplette Bl�cke unber�hrt (Testbeispiel f�r [TC11] ), L�uft eine Weile

	//overload_test_Random(); // Was passiert, wenn die Festplatte zu voll geschrieben wird ?


}