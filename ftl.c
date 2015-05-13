/* Implementierung eines FTL's */
#include "ftl.h"

flash_t flashDevice;
// Funktionen Allocator



// Funktionen Cleaner
// Nichts zu tun ? 

// Funktionen Wear-Leveler ([TC11]- Algorithmus)

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
	for (i = 0; i < BLOCK_COUNT * PAGES_PER_BLOCK * (PAGE_DATASIZE / LOGICAL_BLOCK_DATASIZE); i++){
		flashDevice.mappingTable[i] = 0;
	}
	flashDevice.activeBlockPosition = 32;
	flashDevice.invalidCounter = 0;
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
	int i = 0;
	uint32_t target = 0;
	while (target != index){
		target = flashDevice->mappingTable[i];
		i++;
	}
	block = target / BLOCK_COUNT;
	page = target % BLOCK_COUNT;
	bp_index = (target % BLOCK_COUNT) % PAGES_PER_BLOCK;
	return FL_readData(block, page, bp_index, LOGICAL_BLOCK_DATASIZE, &flashDevice->flashHardware); // ToDo
}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	
	int block = flashDevice->activeBlockPosition / BLOCK_COUNT; 
	int page = flashDevice->activeBlockPosition % BLOCK_COUNT;
	int bp_index = (flashDevice->activeBlockPosition % BLOCK_COUNT) % PAGES_PER_BLOCK;
	int count;
	count = FL_writeData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, &flashDevice->flashHardware);
	if (count == LOGICAL_BLOCK_DATASIZE){
		flashDevice->mappingTable[index] = flashDevice->activeBlockPosition;
		if (index < LOGICAL_BLOCK_DATASIZE){
			flashDevice->activeBlockPosition++;
		}
		else {
			// ToDo: Wähle neuen Block aus der Blockliste und passe "activeBlockPosition" an
		}
		return TRUE;
	}
	else {
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben", count, LOGICAL_BLOCK_DATASIZE);
	}
}