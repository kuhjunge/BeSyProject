// Test ausf�hrung
#include "ftl.h"
#include <time.h>

/*
Diese Helferfunktion erm�glicht es die SSD mit Testdaten vollzuschreiben
Es wird nach dem Schreibvorgang direkt einmal gepr�ft ob das erste Zeichen richtig geschrieben wurde

ssd : Datenstruktur
start: Anfang des Schreibbereiches (Anfang schreibbereich)
amount : wie viele Datensegmente geschrieben werden sollen (Ende Schreibbereich)
rnd : nacheinander schreiben [0] oder Zuf�llig [1]
tc : Wie viele Datens�tze sollen geschrieben werden
debug : Punkt Ausgabe nach jedem beschriebenen Block (Optische Verdeutlichung der Funktionsweise)
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
	Diese Funktion testet die SSD mit einer bestimmten Anzahl von Random Beschreibungen.
	diese Funktion nutzt die Funktion writeData

	ssd : Datenstruktur
	flMe: �bergabeparameter f�r Datenstruktur
	tc : Anzahl der Geschriebenen Testdaten
	segmax : Wie viele Segmente sollen genutzt werden?
	rnd : [0] kein Zuf�lliges beschreiben , [1] Zuf�lliges Schreiben
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
Der mountmapping_test schreibt erst den Flashspeicher voll mit Daten und pr�ft anschlie�end ob alle Daten erreichbar sind.

ssd : Datenstruktur
flMe: �bergabeparameter f�r Datenstruktur
multiplikator : Wie oft der Schreibzugang durchgef�hrt wird (1 - 750+)
logicalsize : Wie gro� die logische Gr��e ist (16)
spare: Angabe der Spareblockanzahl (~5)
blockcount: Wie viele Bl�cke es insgesammt gibt (32)
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
				printf("Fataler Lesefehler");
				printerr(ssd);
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

/*
	Der Mapping Test schreibt erst den Flashspeicher voll mit Daten und pr�ft anschlie�end ob alle Daten erreichbar sind.

	ssd : Datenstruktur
	flMe: �bergabeparameter f�r Datenstruktur
	multiplikator : Wie oft der Schreibzugang durchgef�hrt wird (1 - 750+)
	logicalsize : Wie gro� die logische Gr��e ist (16)
	spare: Angabe der Spareblockanzahl (~2)
	blockcount: Wie viele Bl�cke es insgesammt gibt (32)
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
				checkvalue = (uint8_t)((i + j  + k) % 254);
				myData[j] = checkvalue;
				//printf("%c", checkvalue); 
			}
			if (writeBlock(ssd, i, myData) == FALSE){
				if (i > 0){
					// Schreibs und Leseschleife unterbrechen
					checki = i -1; 
					printerr(ssd);
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
				checkvalue = (uint8_t)((i + j + k) % 254);
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
	Der Mount Test schreibt erst ein Segment. Unmount, mountet und �berpr�ft, ob das geschrieben Segment richtig gelesen wurde.

	ssd : Datenstruktur
	flMe: �bergabeparameter f�r Datenstruktur	
	logicalsize : Wie gro� die logische Gr��e ist (16)	
*/
void mount_test(flash_t* ssd, flashMem_t* flMe, uint8_t logicalsize){
	uint32_t i;
	uint8_t myData[16], myRetData[16], j;
	
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

/*
	Hier werden die Tests gestartet
*/
int main(int argc, char *argv[]) {
	int blocksegment = 16;
	flash_t* ssd = NULL;
	flashMem_t *flMe = NULL;

	srand((unsigned int)time(NULL));

	FL_resetFlash();
	// Wenige Random Datens�tze die kreuz und quer geschrieben werden (Testet Block Verteilung bei wenig geschriebenen Datens�tzen)
	load_test(ssd, flMe, 2000, 200, 1);
	// Komplette Festplatte wird mit Random Datens�tzen vollgeschrieben (Grenzwerttest)
	load_test(ssd, flMe, 5000, 422, 1);

	FL_resetFlash();	
	//schreibe erst einen Block wiederholt; unmount, mount und �berpr�fe den Inhalt dieses Blocks, danach wieder schreiben und �berpr�fen
	mountmapping_test(ssd, flMe, 2, LOGICAL_BLOCK_DATASIZE, SPARE_BLOCKS, BLOCK_COUNT, blocksegment,0); // Pr�ft das Mapping auf Richtigkeit  (Testbeispiel f�r [TC11] Algorithmus)
	
	FL_resetFlash();
	//schreibe ein Segment, unmount, mounte und �berpr�fe, ob Segment richtig gelesen wurde
	mount_test(ssd, flMe, LOGICAL_BLOCK_DATASIZE);

	FL_resetFlash();
	//Test f�r BadBlock-Verhalten, Festplatte nicht ganz voll
	mapping_test(ssd, flMe, 2500, LOGICAL_BLOCK_DATASIZE, SPARE_BLOCKS, BLOCK_COUNT, blocksegment, 0); // Pr�ft das Mapping auf Richtigkeit  (Testbeispiel f�r [TC11] Algorithmus)

}