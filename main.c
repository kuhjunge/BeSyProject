// Test ausführung
#include "ftl.h"
#include <time.h>

void writeData(flash_t* ssd, int start, int amount, int rnd, int tc){
	uint8_t myData[16], myRetData[16];
	int i;
	int j;
	uint32_t r;
	int k = 0;

	for (j = 0; j < tc; j++){
		if (rnd > 0){
			r = 1 + start + rand() % amount;
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

		readBlock(ssd, r, myRetData);
		if (myRetData[0] != 'A'){
			printf("Lesefehler\n\n");
			printerr(ssd);
		}
	}
}

void test_write_random_n_locigalBlocks(flash_t* ssd, flashMem_t flMe, uint16_t blocks, uint16_t range, uint16_t charNumber){
	uint16_t i, j, l;
	uint16_t k = 0;
	uint8_t myData[16], myRetData[16];

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
			//zufällige Zeichenfolge
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
			//überprüfe
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

void test_write_n_locigalBlocks(flash_t* ssd, flashMem_t flMe, uint16_t blocks, uint16_t range, uint16_t charNumber){
	uint16_t i, l, j;
	uint16_t k = 0;
	uint8_t myData[16], myRetData[16];

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	
	for(i = 0; i < range; i++)
		for(l = 1; l <= blocks; l++){
			//zufällige Zeichenfolge
			for (j = 0; j < charNumber; j++){
				myData[j] = (uint8_t)(65 + rand() % 20);
			}
			printf("%i/%i\n",i+1,range);
			//schreibe
			writeBlock(ssd, l, myData);
			//lese
			readBlock(ssd, l, myRetData);
			//überprüfe
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


void test_write_one_logicalBlock(flash_t* ssd, flashMem_t flMe, uint16_t range, uint16_t charNumber){
	uint16_t i,j;
	uint16_t k = 0;
	uint8_t myData[16], myRetData[16];

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}

	
	for(i = 0; i < range; i++){
		//zufällige Zeichenfolge
		for (j = 0; j < charNumber; j++){
			myData[j] = (uint8_t)(65 + rand() % 20);
		}
		printf("%i/%i\n",i+1,range);
		//schreibe
		writeBlock(ssd, 1, myData);
		//lese
		readBlock(ssd, 1, myRetData);
		//überprüfe
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
void load_test_Random_Full(flash_t* ssd, flashMem_t flMe, int tc, int segmax){
	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(ssd, 0, segmax, 1, tc);

	printf("Unmount\n");
	ssd = unmount(ssd);
	printerr(ssd);
	printf("load_test_Random_Full Ende");
}

void overload_test_Random(flash_t* ssd, flashMem_t flMe){

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(ssd, 0, 480, 0, 480);

	printf("Unmount\n");
	ssd = unmount(ssd);
	printerr(ssd);
	printf("overload_test_Random Ende");
}

void mount_test_Light(flash_t* ssd, flashMem_t flMe, uint16_t range, uint8_t charNumber){
	uint32_t i,k,j;
	uint8_t myData[16], myRetData[16];

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	
	for(i = 0; i < range; i++){
		//zufällige Zeichenfolge
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
	//überprüfe
	for(k = 0; k < charNumber; k++){
		if(myData[k] != myRetData[k]){
			printf("Fehler beim Lesen\n");
			printerr(ssd);
			return;
		}
	}
	for(i = 0; i < range; i++){
		//zufällige Zeichenfolge
		for (j = 0; j < charNumber; j++){
			myData[j] = (uint8_t)(65 + rand() % 20);
		}
	
		printf("write data %i/%i\n",i+1,range);

		//schreibe
		writeBlock(ssd, 1, myData);
	}
	//lese
	readBlock(ssd, 1, myRetData);
	//überprüfe
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

void load_test_Random_Light(flash_t* ssd, flashMem_t flMe, int tc){
	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe); 
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(ssd, 0, 120, 1, tc);

	printf("Unmount\n");
	ssd = unmount(ssd);
	printf("load_test_Random_Light Ende");
	printerr(ssd);
}

void load_test_OS(flash_t* ssd, flashMem_t flMe){
	int i;

	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(ssd, 0, 119, 0, 120); // Installation OS
	for (i = 0; i < 20; i++){
		writeData(ssd, 119, 239, 1, 200); // File Usage
		writeData(ssd, 358, 70, 1, 1000); // Temp Data Usage
		writeData(ssd, 428, 30, 1, 10000); // Extrem Data Usage
	}
	printf("Unmount\n");
	ssd = unmount(ssd);
	printerr(ssd);
	printf("load_test_OS Ende");
}


/*
	Der Mapping Test schreibt erst den Flashspeicher voll mit Daten und prüft anschließend ob alle Daten erreichbar sind

	ssd : Datenstruktur
	flMe: Übergabeparameter für Datenstruktur
	multiplikator : Wie oft der Schreibzugang durchgeführt wird (1 - 750+)
	logicalsize : Wie groß die logische Größe ist (16)
	spare: Angabe der Spareblockanzahl (~2)
	blockcount: Wie viele Blöcke es insgesammt gibt (32)
	blocksegment : Anzahl der beschreibbaren Einheiten eines Blockes (16)
*/
void mapping_test(flash_t* ssd, flashMem_t flMe, int multiplikator, int logicalsize, int spare, int blockcount, int blocksegment){
	uint32_t i, j, k;
	uint8_t checkvalue;
	uint8_t myData[16], myRetData[16];
	printf("Mount \n");
	FL_resetFlash(); // Start der Simulation
	ssd = mount(&flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(ssd, 0, blocksegment, 1, blockcount * blocksegment); // Speicher vorbeschreiben	

	for (k = 0; k < multiplikator; k++){
		printf("\nZyklus: %i\n", k);
		for (i = blocksegment; i <= (blockcount - spare)* blocksegment; i++){
			for (j = 0; j < logicalsize; j++){
				checkvalue = (uint8_t)(((i * j)  +k) % 255);
				myData[j] = checkvalue;
				//printf("%c", checkvalue);
			}
			writeBlock(ssd, i, myData);
			//printf(".");
		}
		for (i = blocksegment; i <= (blockcount - spare)* blocksegment; i++){
			readBlock(ssd, i, myRetData);
			for (j = 0; j < logicalsize; j++){
				checkvalue = (uint8_t)(((i * j) + k) % 255);
				if (myRetData[j] != checkvalue){
					printf("Mappingfehler\n");
					printerr(ssd);
				}
			}
		}
	}
	printf("Unmount\n");
	printerr(ssd); // Debug Ausgabe der Datenstruktur!
	ssd = unmount(ssd);
	printf("Mappingtest erfolgreich\n");
	
}

int main(int argc, char *argv[]) {
	int blocksegment = 16;
	flash_t* ssd = NULL;
	flashMem_t flMe;

	srand((unsigned int)time(NULL));

	//schreibe wiederholt einen logischen Block
	//test_write_one_logicalBlock(20000, LOGICAL_BLOCK_DATASIZE); 

	//schreibe wiederholt verschiedene logische Blöcke	
	// 1 bis maximal 480 => 2 Blöcke Spare; d.h. 512(32*16) - 32 
	//test_write_n_locigalBlocks( 480, 100,LOGICAL_BLOCK_DATASIZE );		

	//schreibe wiederholt zufällige logische Blöcke
	//test_write_random_n_locigalBlocks(480, 65533, LOGICAL_BLOCK_DATASIZE);

	//schreibe erst einen Block wiederholt; unmount, mount und überprüfe den Inhalt dieses Blocks, danach wieder schreiben und überprüfen
	//mount_test_Light(1000, LOGICAL_BLOCK_DATASIZE);

	// Wenige Random Datensätze die kreuz und quer geschrieben werden (Testet Block Verteilung bei wenig geschriebenen Datensätzen)
	//load_test_Random_Light(); 

	//load_test_Random_Full(); // Komplette Festplatte wird mit Random Datensätzen vollgeschrieben (Extremwerttest)

	//mapping_test(750, 16, 2, 32);
	//void mapping_test(flash_t* ssd, flashMem_t flMe, int multiplFactor, int blocksegement, int spare, int blockcount){
	mapping_test(ssd, flMe, 3, LOGICAL_BLOCK_DATASIZE, SPARE_BLOCKS, BLOCK_COUNT, blocksegment); // Prüft das Mapping auf Richtigkeit  (Testbeispiel für [TC11] Algorithmus)

	//load_test_OS(); // Sorgt für hohe schreibrate und lässt teilweise komplette Blöcke unberührt (Testbeispiel für [TC11] ), Läuft eine Weile

	//overload_test_Random(); // Was passiert, wenn die Festplatte zu voll geschrieben wird ?


}