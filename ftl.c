/* Implementierung eines FTL's */
#include "ftl.h"

flash_t flashDevice;
// Funktionen Allocator

// Funktionen Cleaner
// Nichts zu tun ? 
void cleaner(flash_t *flashDevice){
	int i = -1;
	int j;
	int loeschcount = 0;
	int schwellwert = flashDevice->invalidCounter / BLOCK_COUNT;
	uint8_t count;
	uint32_t adress;
	uint8_t temp[16];

	while (i < BLOCK_COUNT && loeschcount < CLEAN_BLOCK_COUNT){
		i++;
		if (flashDevice->blockArray[i].invalidCounter >= schwellwert && flashDevice->blockArray[i].status == used){ // Wenn Block über Schwellwert liegt und benutzt wird
			for (j = 0; j < BLOCKSEGMENTS; j++){

				if (flashDevice->blockArray[i].BlockStatus[j] != invalid){
					adress = flashDevice->mappingTable[(i * BLOCKSEGMENTS) + j ];
					count = FL_readData(i, j / PAGES_PER_BLOCK, j % PAGES_PER_BLOCK * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, &temp);
					count = writeBlock(flashDevice, adress, &temp);
					flashDevice->invalidCounter--;
					//flashDevice->blockArray[i].invalidCounter--;
				}
				flashDevice->blockArray[i].BlockStatus[j] = unused; // Segemente auf unused setzen
			}
			if (FL_deleteBlock(i) == TRUE){ // Wenn erfolgreich gelöscht
				flashDevice->blockArray[i].loeschzaehler++; // block löschzähler hochsetzten
				flashDevice->blockArray[i].invalidCounter = 0; // Counter zurück setzen
				flashDevice->blockArray[i].status = ready; // Status auf Ready setzen
				flashDevice->freeBlocks++;
				loeschcount++;
			}
			else { // wenn nicht erfolgreich gelöscht
				printf("Bad Block!\n"); // badBlock
				flashDevice->blockArray[i].status = badBlock;
			}
		}

	}
}

// Funktionen Wear-Leveler ([TC11]- Algorithmus)

// Hilfs Funktionen FLT

void printerr(flash_t *flashDevice){
	int i, j, invCo = 0, loesch = 0;
	int block = flashDevice->activeBlockPosition / BLOCK_COUNT;
	int segment = (flashDevice->activeBlockPosition % BLOCK_COUNT);
	char marker;
	char fehler;
	printf("Fehleranalyse!\n");
	printf("Freie Blocks: %i\nInvalide Segmente: %i\nAktuelle Schreibposition: %i\n\n"
		, flashDevice->freeBlocks, flashDevice->invalidCounter, flashDevice->activeBlockPosition);
	printf("Block | Status | Invalide Segmente | Loeschanzahl\n");
	for (i = 0; i < BLOCK_COUNT; i++)
	{
		printf("   %02i |   %i    |         %02i        |     %i\n", i
			, flashDevice->blockArray[i].status, flashDevice->blockArray[i].invalidCounter, flashDevice->blockArray[i].loeschzaehler);
		invCo = invCo + flashDevice->blockArray[i].invalidCounter;
		loesch = loesch + flashDevice->blockArray[i].loeschzaehler;
	}
	printf("      |Free:%02i |        %04i       |     %i\n", flashDevice->freeBlocks, invCo, loesch);
	printf("Block: ready (0) / used (1) / badBlock (2) \n");
	printf("Block fuer Block Detailanalyse!\n");
	getchar();
	for (i = 0; i < BLOCK_COUNT; i++)
	{
		printf("\nBlock %02i:\n", i);
		if (flashDevice->blockArray[i].status == ready){
			printf(" Status: Ready (0)\n");
		}
		else if (flashDevice->blockArray[i].status == used){
			printf(" Status. Used (1)\n");
		}
		else{
			printf(" Status: Invalid (2)\n");
		}
			printf(" Invalide Segmente: %i\n Loeschanzahl: %i\n\n", flashDevice->blockArray[i].invalidCounter, flashDevice->blockArray[i].loeschzaehler);
		for (j = 0; j < BLOCKSEGMENTS; j++){
			marker = ' ';
			fehler = ' ';
			if (block == i && segment == j){marker = 'S';}
			if (flashDevice->blockArray[i].BlockStatus[j] != assigned && flashDevice->mappingTable[(i *BLOCKSEGMENTS) + j + 1] != 0){ fehler = '!'; }
			if (flashDevice->blockArray[i].BlockStatus[j] == assigned && flashDevice->mappingTable[(i *BLOCKSEGMENTS) + j + 1] == 0){ fehler = '!'; }
			if (flashDevice->blockArray[i].BlockStatus[j] > 2 ){ fehler = '?'; }
			printf("Segment %02i: Table: %03i - %i %c %c\n", j, flashDevice->mappingTable[(i *BLOCKSEGMENTS) + j + 1], flashDevice->blockArray[i].BlockStatus[j],fehler, marker);
		}
		printf("Segment: unused (0) / assigned (1) / invalid (2)\n");
		getchar();

	}

}

int setFreeBlock(flash_t *flashDevice){
	int i = 0;
	while (flashDevice->blockArray[i].status != ready && i < BLOCK_COUNT)
	{
		i++;
	}
	flashDevice->activeBlockPosition = i * BLOCK_COUNT;
	flashDevice->freeBlocks--;
}

// Gibt 512 Speicherplatz representation zurück [Blocksegment]
int mapping(flash_t *flashDevice, uint32_t index){
	uint32_t target;
	int i = -1;
	int x = MAPPING_TABLE_SIZE;
	do {
		i++;
		target = flashDevice->mappingTable[i];
	} while (target != (index +1)&& i < x);
	if (i+1 >= x){
		i = -1;
	}
	/*
	Algorihtmus nicht so effizient wie HashMap, aber akutell keine Idee wie ich eine Hashmap für uint32_t in C implementiere
	*/
	return i;
}

void invalidationOfOldIndex(flash_t *flashDevice, uint32_t index){ // ToDo: Prüft auf Null, erzeugt evtl fehler bei ID Null ?
	int old_block;
	int oldDataIndex = mapping(flashDevice, index); // Testen ob schon Eintrag mit diesem Index vorhanden ist
	if (oldDataIndex > -1) { // Wenn ja, Eintrag invalidieren
		int i = BLOCKSEGMENTS;
		int old_block = oldDataIndex / i;
		
		flashDevice->mappingTable[oldDataIndex+1] = 0;
		flashDevice->blockArray[old_block].BlockStatus[oldDataIndex - (old_block * i)] = invalid;
		flashDevice->invalidCounter++;
		flashDevice->blockArray[old_block].invalidCounter++;
	}
}

// Funktionen FLT
flash_t *mount(flashMem_t *flashHardware){
	int i, j;

	flashDevice.flashHardware = flashHardware;
	// Initialisieren
	flashDevice.isFehler = 0;
	for (i = 0; i < BLOCK_COUNT; i++){
		for (j = 0; j < PAGES_PER_BLOCK * (PAGE_DATASIZE / LOGICAL_BLOCK_DATASIZE); j++){
			flashDevice.blockArray[i].BlockStatus[j] = unused;
		}
		flashDevice.blockArray[i].invalidCounter = 0;
		flashDevice.blockArray[i].loeschzaehler = 0;
		flashDevice.blockArray[i].status = ready;
	}
	for (i = 0; i < MAPPING_TABLE_SIZE; i++){
		flashDevice.mappingTable[i] = 0;
	}
	flashDevice.activeBlockPosition = 0;
	flashDevice.invalidCounter = 0;
	flashDevice.freeBlocks = BLOCK_COUNT;
	//uint8_t *FL_restoreState(unit8_t *state)

	//ToDo: Datenstruktur laden
	return &flashDevice; // ToDo
}

flash_t *unmount(flash_t *flashDevice){
	uint8_t stateSize = FL_getStateSize();
	uint8_t* state = (uint8_t*)malloc(sizeof(uint8_t) * stateSize); // allocate 
	if (stateSize > 0){
		// FL_getStateSize(void)
		FL_saveState(BLOCK_COUNT, *state);
	}
	// ToDo Datenstruktur speichern

	// Fehlerfall Datenstruktur mit Fehlermeldung zurück geben
	return NULL; // ToDo
}

uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	int block;
	int page;
	int bp_index;
	int i = mapping(flashDevice, index) -1; // Mapping
	block = i / BLOCKSEGMENTS;
	page = (i % BLOCKSEGMENTS) / PAGES_PER_BLOCK;
	bp_index = (i % BLOCKSEGMENTS) % PAGES_PER_BLOCK;
	return FL_readData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Blocksegment auslesen
}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	
	int block = flashDevice->activeBlockPosition / BLOCK_COUNT; 
	int page = (flashDevice->activeBlockPosition % BLOCK_COUNT) / PAGES_PER_BLOCK;
	int bp_index = (flashDevice->activeBlockPosition % BLOCK_COUNT) % PAGES_PER_BLOCK;
	int count;
	if (flashDevice->blockArray[block].status != ready){
		printf("Fehler beim Schreiben des Datensatzes! Fehlerhafter Block Zugriff!\n");
		printerr(flashDevice);
		return FALSE;
	}
	count = FL_writeData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Daten beschreiben
	if (count != LOGICAL_BLOCK_DATASIZE){ // Prüfen ob wirklich entsprechende Daten geschrieben wurden		}
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		return FALSE;
	}
	flashDevice->blockArray[block].BlockStatus[flashDevice->activeBlockPosition % BLOCK_COUNT] = assigned; // Beschriebenes Segement merken
	
	invalidationOfOldIndex(flashDevice, index); // Alten Eintrag in Mapping Table und Block invalidieren

	flashDevice->mappingTable[(block * BLOCKSEGMENTS) + (page * PAGES_PER_BLOCK) + bp_index+1] = index + 1; // Mapping erzeugen nachdem das alte Mapping invalidiert wurde

	// Auswahl des nächsten Schreibortes
	if (page  * (bp_index + 1) < ((PAGES_PER_BLOCK - 1)* (PAGE_DATASIZE / LOGICAL_BLOCK_DATASIZE) - 1)){ // position weiterzählen wenn innerhalb des selben blockes
		flashDevice->activeBlockPosition++;
	}
	else { // oder einen neuen Block auswählen
		flashDevice->blockArray[block].status = used;  // alten Block abschließen
		setFreeBlock(flashDevice); // Freien Block finden und nutzen
		if (flashDevice->freeBlocks < START_CLEANING + SPARE_BLOCKS){ // Cleaner
			// ToDo: Clean
			cleaner(flashDevice);
		}
		if (flashDevice->freeBlocks <= SPARE_BLOCKS) { // Fehler falls zu wenig freie Blöcke da sind
			printf("Fehler Festplatte zu voll!\n");
			return FALSE;
		}
	}
	return TRUE;

	

}