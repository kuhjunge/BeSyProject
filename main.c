// Test ausführung
#include "ftl.h"
#include <time.h>

/*
Diese Funktion ermöglicht es die SSD zu testen

ssd : Datenstruktur
start: Anfang des Schreibbereiches (Anfang schreibbereich)
amount : wie viele Datensegmente geschrieben werden sollen (Ende Schreibbereich)
rnd : nacheinander schreiben [0] oder Zufällig [1]
tc : Wie viele Datensätze sollen geschrieben werden
debug : Punkt ausgabe nach jedem beschriebenen Block
*/
void writeData(flash_t* ssd, int start, int amount, int rnd, int tc, int debug){
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
		if (debug != 0) printf(".");
		for (i = 0; i < 16; i++)
			myData[i] = (uint8_t)(i + 65);

		writeBlock(ssd, r, myData);

		readBlock(ssd, r, myRetData);
		if (myRetData[0] != myData[0]){
			printf("Lesefehler\n\n");
			printerr(ssd);
			return;
		}
	}
}

/*
	Diese Funktion ermöglicht es die SSD zu testen 

	ssd : Datenstruktur
	flMe: Übergabeparameter für Datenstruktur
	tc : Anzahl der Geschriebenen Testdaten
	segmax : Wie viele Segmente sollen genutzt werden? [480]
	rnd : 0 = kein Zufälliges beschreiben , sonst alle werte Zufall
*/
void load_test(flash_t* ssd, flashMem_t *flMe, int tc, int segmax, int rnd){
	printf("Mount \n");
	//FL_resetFlash(); // Start der Simulation
	ssd = mount(flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(ssd, 0, segmax, rnd, tc,1);
	printf("Daten erfolgreich geschrieben.");
	printerr(ssd);
	printf("Unmount\n");
	unmount(ssd);
	printf("load_test Ende");
}

/*
Der mountmapping_test schreibt erst den Flashspeicher voll mit Daten und prüft anschließend ob alle Daten erreichbar sind.

ssd : Datenstruktur
flMe: Übergabeparameter für Datenstruktur
multiplikator : Wie oft der Schreibzugang durchgeführt wird (1 - 750+)
logicalsize : Wie groß die logische Größe ist (16)
spare: Angabe der Spareblockanzahl (~2)
blockcount: Wie viele Blöcke es insgesammt gibt (32)
blocksegment : Anzahl der beschreibbaren Einheiten eines Blockes (16)
*/
void mountmapping_test(flash_t* ssd, flashMem_t* flMe, uint32_t multiplikator, uint32_t logicalsize, int spare, int blockcount, uint32_t blocksegment, uint8_t showPoints){
	uint32_t i, j, k, checki;
	uint8_t checkvalue, fail = FALSE;
	uint8_t myData[16], myRetData[16];
	printf("Mount \n");
	//FL_resetFlash(); // Start der Simulation
	ssd = mount(flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(ssd, 0, blocksegment, 0, blocksegment, 0); // Speicher vorbeschreiben
	//printerr(ssd);
	for (k = 0; k < multiplikator; k++){
		checki = (blockcount - spare)* blocksegment;
		printf("\nZyklus: ------------------------------------------ %i\n", k);
		writeData(ssd, blocksegment, ((blockcount - spare) * blocksegment) - blocksegment, 1, blockcount * blocksegment, showPoints); // Speicher vorbeschreiben	
		for (i = blocksegment; i <= checki; i++){
			for (j = 0; j < logicalsize; j++){
				checkvalue = (uint8_t)('A' + j);
				myData[j] = checkvalue;
				//printf("%c", checkvalue); 
			}
			if (writeBlock(ssd, i, myData) == FALSE){
				if (i > 0){
					// Schreibs und Leseschleife unterbrechen
					checki = i - 1;
				}
				else{
					i = 0;
				}
				// Zyklus Schleife unterbrechen
				multiplikator = 0;
			}
			if (showPoints != 0) {
				printf(".");
			}

		}
		//Mount Test
		unmount(ssd);
		mount(flMe);
		for (i = blocksegment; i <= checki; i++){
			readBlock(ssd, i, myRetData);
			for (j = 0; j < logicalsize; j++){
				checkvalue = (uint8_t)('A' + j);
				if (myRetData[j] != checkvalue){
					printf("Mappingfehler an Adresse %i -> %c (%i) != %c (%i)\n", i, myRetData[j], myRetData[j], checkvalue, checkvalue);
					printLogicalToHW(ssd, i);
					printerr(ssd);
					break;
				}
			}
		}
	}
	printf("Unmount\n");
	printerr(ssd); // Debug Ausgabe der Datenstruktur!
	ssd = unmount(ssd);
	printf("Mappingtest erfolgreich\n");
}

void simple_mapping_test(flash_t* ssd, flashMem_t* flMe, uint32_t multiplikator, uint32_t logicalsize, int spare, int blockcount, uint32_t blocksegment, uint8_t showPoints){
	uint32_t i, j, k, checki;
	uint8_t checkvalue, ret;
	uint8_t myData[16], myRetData[16];
	printf("Mount \n");
	//FL_resetFlash(); // Start der Simulation
	ssd = mount(flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(ssd, 0, blocksegment, 0, blocksegment, 0); // Speicher vorbeschreiben
	//printerr(ssd);
	for (k = 0; k < multiplikator; k++){
		checki = (blockcount - spare)* blocksegment;
		printf("\nZyklus: ------------------------------------------ %i\n", k);
		//writeData(ssd, blocksegment, ((blockcount - spare) * blocksegment) - blocksegment, 1, blockcount * blocksegment,showPoints); // Speicher vorbeschreiben	
		for (i = blocksegment; i <= checki; i++){
			for (j = 0; j < logicalsize; j++){
				checkvalue = (uint8_t)('A' + j);
				myData[j] = checkvalue;
				//printf("%c", checkvalue); 
			}
			if (writeBlock(ssd, i, myData) == FALSE){
				if (i > 0){
					// Schreibs und Leseschleife unterbrechen
					checki = i - 1;
				}
				else{
					i = 0;
				}
				// Zyklus Schleife unterbrechen
				multiplikator = 0;
			}
			if (showPoints != 0) {
				printf(".");
			}

		}
		for (i = blocksegment; i <= checki; i++){
			if (FALSE == readBlock(ssd, i, myRetData))
			{ 
				printf("Lesefehler bei Pruefung\n");
			}
			for (j = 0; j < logicalsize; j++){
				checkvalue = (uint8_t)('A' + j);
				if (myRetData[j] != checkvalue){
					printf("(%i) Mappingfehler an Adresse %i -> %c (%i) != %c (%i)\n",k, i + 1, myRetData[j], myRetData[j], checkvalue, checkvalue);
					printLogicalToHW(ssd, i + 1);
					printerr(ssd);
					break;
				}
			}
		}
	}
	printf("Unmount\n");
	printerr(ssd); // Debug Ausgabe der Datenstruktur!
	ssd = unmount(ssd);
	printf("Mappingtest erfolgreich\n");
}

/*
	Der Mapping Test schreibt erst den Flashspeicher voll mit Daten und prüft anschließend ob alle Daten erreichbar sind.

	ssd : Datenstruktur
	flMe: Übergabeparameter für Datenstruktur
	multiplikator : Wie oft der Schreibzugang durchgeführt wird (1 - 750+)
	logicalsize : Wie groß die logische Größe ist (16)
	spare: Angabe der Spareblockanzahl (~2)
	blockcount: Wie viele Blöcke es insgesammt gibt (32)
	blocksegment : Anzahl der beschreibbaren Einheiten eines Blockes (16)
*/
void mapping_test(flash_t* ssd, flashMem_t* flMe, uint32_t multiplikator, uint32_t logicalsize, int spare, int blockcount, uint32_t blocksegment, uint8_t showPoints){
	uint32_t i, j, k, checki;
	uint8_t checkvalue, fail = FALSE;
	uint8_t myData[16], myRetData[16];
	printf("Mount \n");
	//FL_resetFlash(); // Start der Simulation
	ssd = mount(flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}
	writeData(ssd, 0, blocksegment, 0,  blocksegment,0); // Speicher vorbeschreiben
	//printerr(ssd);
	for (k = 0; k < multiplikator; k++){
		checki = (blockcount - spare)* blocksegment;
		printf("\nZyklus: ------------------------------------------ %i\n", k);
		writeData(ssd, blocksegment, ((blockcount - spare) * blocksegment) - blocksegment, 1, blockcount * blocksegment,showPoints); // Speicher vorbeschreiben	
		for (i = blocksegment; i <= checki; i++){
			for (j = 0; j < logicalsize; j++){
				checkvalue = (uint8_t)(((i * j)  +k) % 255);
				myData[j] = checkvalue;
				//printf("%c", checkvalue); 
			}
			if (writeBlock(ssd, i, myData) == FALSE){
				if (i > 0){
					// Schreibs und Leseschleife unterbrechen
					checki = i -1; 
				}
				else{
					i = 0;
				}
				// Zyklus Schleife unterbrechen
				multiplikator = 0;
			}
			if (showPoints != 0) {
				printf(".");
			}

		}
		for (i = blocksegment; i <= checki; i++){
			readBlock(ssd, i, myRetData);
			for (j = 0; j < logicalsize; j++){
				checkvalue = (uint8_t)(((i * j) + k) % 255);
				if (myRetData[j] != checkvalue){
					printf("Mappingfehler an Adresse %i -> %c (%i) != %c (%i)\n", i, myRetData[j], myRetData[j], checkvalue, checkvalue);
					printLogicalToHW(ssd, i);
					printerr(ssd);
					break;
				}
			}
		}
	}
	printf("Unmount\n");
	printerr(ssd); // Debug Ausgabe der Datenstruktur!
	ssd = unmount(ssd);
	printf("Mappingtest erfolgreich\n");
}

/*
	Der Mount Test schreibt erst ein Segment. Unmount, mountet und überprüft, ob das geschrieben Segment richtig gelesen wurde.

	ssd : Datenstruktur
	flMe: Übergabeparameter für Datenstruktur	
	logicalsize : Wie groß die logische Größe ist (16)	
*/
void mount_test(flash_t* ssd, flashMem_t* flMe, uint8_t logicalsize){
	uint32_t i, j, k;
	uint8_t myData[16], myRetData[16];
	
	printf("Mount \n");	
	ssd = mount(flMe);
	if (ssd == NULL){
		printf("FEHLER (ist Flashspeicher initialisiert?) \n");
		return;
	}

	for (j = 0; j < logicalsize; j++){		
		myData[j] = j;
		//printf("%i", j);
	}

	writeBlock(ssd, 0, myData);
	unmount(ssd);
	mount(flMe);
	readBlock(ssd, 0, myRetData);
	for(i = 0; i < logicalsize; i++){
		if( myData[i] != myRetData[i] ){
			printf("Lesefehler %i == %i \n", myData[i], myRetData[i]);
		}	
	}
	printf("Unmount\n");
	printerr(ssd); // Debug Ausgabe der Datenstruktur!
	ssd = unmount(ssd);
	printf("Mounttest erfolgreich\n");
}

int main(int argc, char *argv[]) {
	int blocksegment = 16;
	flash_t* ssd = NULL;
	flashMem_t *flMe = NULL;

	srand((unsigned int)time(NULL));

	FL_resetFlash();	
	//schreibe erst einen Block wiederholt; unmount, mount und überprüfe den Inhalt dieses Blocks, danach wieder schreiben und überprüfen
	//mountmapping_test(ssd, flMe, 3, LOGICAL_BLOCK_DATASIZE, SPARE_BLOCKS, BLOCK_COUNT, blocksegment,0); // Prüft das Mapping auf Richtigkeit  (Testbeispiel für [TC11] Algorithmus)
	
	//schreibe ein Segment, unmount, mounte und überprüfe, ob Segment richtig gelesen wurde
	mount_test(ssd, flMe, LOGICAL_BLOCK_DATASIZE);

	//FL_resetFlash();
	// Wenige Random Datensätze die kreuz und quer geschrieben werden (Testet Block Verteilung bei wenig geschriebenen Datensätzen)
	 //load_test(ssd, flMe, 2000, 200, 1);
	// Komplette Festplatte wird mit Random Datensätzen vollgeschrieben (Grenzwerttest)
	//load_test(ssd, flMe, 5000, 480, 1);
	
	FL_resetFlash();
	//mapping_test(ssd, flMe, 25, 16, 2, 32, 16);
	//Test am Limit
	simple_mapping_test(ssd, flMe, 4000, LOGICAL_BLOCK_DATASIZE, SPARE_BLOCKS, BLOCK_COUNT, blocksegment,0); // Prüft das Mapping auf Richtigkeit  (Testbeispiel für [TC11] Algorithmus)
	FL_resetFlash();
	//Test für BadBlock-Verhalten, Festplatte nicht ganz voll
	mapping_test(ssd, flMe, 500, LOGICAL_BLOCK_DATASIZE, SPARE_BLOCKS, BLOCK_COUNT, blocksegment, 0); // Prüft das Mapping auf Richtigkeit  (Testbeispiel für [TC11] Algorithmus)
	FL_resetFlash();
	// Overload Test
	load_test(ssd, flMe,481,481,0 ); // Was passiert, wenn die Festplatte zu voll geschrieben wird ?
}