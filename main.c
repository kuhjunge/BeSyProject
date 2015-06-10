// Test ausf�hrung
#include "ftl.h"
#include <time.h>

/*
Diese Funktion erm�glicht es die SSD zu testen

ssd : Datenstruktur
start: Anfang des Schreibbereiches (Anfang schreibbereich)
amount : wie viele Datensegmente geschrieben werden sollen (Ende Schreibbereich)
rnd : nacheinander schreiben [0] oder Zuf�llig [1]
tc : Wie viele Datens�tze sollen geschrieben werden
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
		if (myRetData[0] != 'A'){
			printf("Lesefehler\n\n");
			printerr(ssd);
		}
	}
}

/*
	Diese Funktion erm�glicht es die SSD zu testen 

	ssd : Datenstruktur
	flMe: �bergabeparameter f�r Datenstruktur
	tc : Anzahl der Geschriebenen Testdaten
	segmax : Wie viele Segmente sollen genutzt werden? [480]
	rnd : 0 = kein Zuf�lliges beschreiben , sonst alle werte Zufall
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
Von Chris Deter

ssd : Datenstruktur
flMe: �bergabeparameter f�r Datenstruktur
multiplikator : Wie oft der Schreibzugang durchgef�hrt wird (1 - 750+)
logicalsize : Wie gro� die logische Gr��e ist (16)
spare: Angabe der Spareblockanzahl (~2)
blockcount: Wie viele Bl�cke es insgesammt gibt (32)
blocksegment : Anzahl der beschreibbaren Einheiten eines Blockes (16)
*/
void mountmapping_test(flash_t* ssd, flashMem_t* flMe, uint32_t multiplikator, uint32_t logicalsize, int spare, int blockcount, uint32_t blocksegment){
	uint32_t i, j, k;
	uint8_t checkvalue;
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
		printf("\nZyklus: %i\n", k);
		writeData(ssd, blocksegment, ((blockcount - spare) * blocksegment) - blocksegment, 1, blockcount * blocksegment, 0); // Speicher vorbeschreiben	
		for (i = blocksegment; i <= (blockcount - spare)* blocksegment; i++){
			for (j = 0; j < logicalsize; j++){
				checkvalue = (uint8_t)(((i * j) + k) % 255);
				myData[j] = checkvalue;
				//printf("%c", checkvalue);
			}
			writeBlock(ssd, i, myData);
			//printf(".");
		}
		ssd = unmount(ssd); // Zwischen lese und schreibe zugriffen einmal unmounten und neu mounten
		ssd = mount(flMe);
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
	printf("Mount Mappingtest erfolgreich\n");
}

/*
	Der Mapping Test schreibt erst den Flashspeicher voll mit Daten und pr�ft anschlie�end ob alle Daten erreichbar sind.
	Von Chris Deter

	ssd : Datenstruktur
	flMe: �bergabeparameter f�r Datenstruktur
	multiplikator : Wie oft der Schreibzugang durchgef�hrt wird (1 - 750+)
	logicalsize : Wie gro� die logische Gr��e ist (16)
	spare: Angabe der Spareblockanzahl (~2)
	blockcount: Wie viele Bl�cke es insgesammt gibt (32)
	blocksegment : Anzahl der beschreibbaren Einheiten eines Blockes (16)
*/
void mapping_test(flash_t* ssd, flashMem_t* flMe, uint32_t multiplikator, uint32_t logicalsize, int spare, int blockcount, uint32_t blocksegment){
	uint32_t i, j, k;
	uint8_t checkvalue;
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
		printf("\nZyklus: %i\n", k);
		writeData(ssd, blocksegment, ((blockcount - spare) * blocksegment) - blocksegment, 1, blockcount * blocksegment,0); // Speicher vorbeschreiben	
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
					//printerr(ssd);
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
	flashMem_t *flMe = NULL;

	srand((unsigned int)time(NULL));
	FL_resetFlash();
	//schreibe erst einen Block wiederholt; unmount, mount und �berpr�fe den Inhalt dieses Blocks, danach wieder schreiben und �berpr�fen
	//mountmapping_test(ssd, flMe, 4, LOGICAL_BLOCK_DATASIZE, SPARE_BLOCKS, BLOCK_COUNT, blocksegment); // Pr�ft das Mapping auf Richtigkeit  (Testbeispiel f�r [TC11] Algorithmus)

	FL_resetFlash();
	// Wenige Random Datens�tze die kreuz und quer geschrieben werden (Testet Block Verteilung bei wenig geschriebenen Datens�tzen)
//	 load_test(ssd, flMe, 2000, 200, 1);
	// Komplette Festplatte wird mit Random Datens�tzen vollgeschrieben (Grenzwerttest)
//	load_test(ssd, flMe, 5000, 480, 1);
	
	FL_resetFlash();
	//mapping_test(ssd, flMe, 25, 16, 2, 32, 16);
	mapping_test(ssd, flMe, 200, LOGICAL_BLOCK_DATASIZE, SPARE_BLOCKS, BLOCK_COUNT, blocksegment); // Pr�ft das Mapping auf Richtigkeit  (Testbeispiel f�r [TC11] Algorithmus)
	
	// Overload Test
	load_test(ssd, flMe,481,481,0 ); // Was passiert, wenn die Festplatte zu voll geschrieben wird ?
}