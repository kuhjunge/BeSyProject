/* Implementierung eines FTL's */
#include "ftl.h"

flash_t flashDevice;
// Funktionen Allocator

// Funktionen Cleaner
// Nichts zu tun ? 

// Funktionen Wear-Leveler ([TC11]- Algorithmus)

// Hilfs Funktionen FLT

int setFreeBlock(flash_t *flashDevice){
	int i = 0;
	while (flashDevice->blockArray[i].status != ready)
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
	if (i >= MAPPING_TABLE_SIZE){
		i = 0;
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
		
		flashDevice->mappingTable[oldDataIndex] = 0;
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
	int i = mapping(flashDevice, index); // Mapping
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
	if (flashDevice->blockArray[block].status == ready){
		count = FL_writeData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Daten beschreiben
		flashDevice->blockArray[block].BlockStatus[flashDevice->activeBlockPosition % BLOCK_COUNT] = assigned; // Beschriebenes Segement merken
		if (count == LOGICAL_BLOCK_DATASIZE){ // Prüfen ob wirklich entsprechende Daten geschrieben wurden
			invalidationOfOldIndex(flashDevice, index); // Alten Eintrag in Mapping Table und Block invalidieren
			flashDevice->mappingTable[(block * BLOCKSEGMENTS) + (page * PAGES_PER_BLOCK) + bp_index] = index + 1; // Mapping erzeugen
			// 
			if (page  * (bp_index + 1) < ((PAGES_PER_BLOCK - 1)* (PAGE_DATASIZE / LOGICAL_BLOCK_DATASIZE) - 1)){ // position weiterzählen wenn innerhalb des selben blockes
				flashDevice->activeBlockPosition++;
			}
			else { // oder einen neuen Block auswählen
				flashDevice->blockArray[block].status = used;  // alten Block abschließen
				if (flashDevice->freeBlocks < 2){ // Cleaner
					// ToDo: Clean

				}
				setFreeBlock(flashDevice); // Freien Block finden und nutzen
				if (flashDevice->freeBlocks < 1) { // Fehler falls zu wenig freie Blöcke da sind
					printf("Fehler Festplatte zu voll!\n");
					return FALSE;
				}
			}
			return TRUE;
		}
		else {
			printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
			return FALSE;
		}
	}
	else {
		printf("Fehler beim Schreiben des Datensatzes! Fehlerhafter Block Zugriff!\n", count, LOGICAL_BLOCK_DATASIZE);
		return FALSE;
	}
}