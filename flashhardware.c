/* Implementierung der Simulation eines Flash-Speichers */
#include "flashhardware.h"

////////////////////////////////////////////////////////////////////
// Deklaration der Instanz eines Flash-Speichermoduls hier 
flashMem_t flashMem;		


// lokal funtions 
uint8_t FL_isInitialised(flashMem_t *flashHardware); 
// predicate, testing if the module is initialised 

uint8_t FL_initFlash(flashMem_t *flashHardware); 
// initialises the flash datastructure without deleting it 

uint8_t FL_deleteFlash(flashMem_t *flashHardware); 
// deletes the flash memory without resetting the deletecounts

uint8_t FL_resetDeleteCounts(flashMem_t *flashHardware); 
// resets the deleteCount values of all blocks to 0, resets dead-flag too

//////////////////////////////////////////////////////////////////////
// Implementation of global functions
uint8_t FL_resetFlash() {
	FL_initFlash(&flashMem);
	if (FL_isInitialised(&flashMem)) FL_deleteFlash(&flashMem);
	if (FL_isInitialised(&flashMem)) FL_resetDeleteCounts(&flashMem);
	return FL_isInitialised(&flashMem); 
}

uint8_t FL_deleteBlock (uint16_t nr) {
	uint32_t i,j,r; 
	if (!FL_isInitialised(&flashMem)) return FALSE;
	if (flashMem.block[nr].dead) return FALSE;	// tote Blöcke nicht löschen
	// Ende der Lebensdauer überprüfen
	if (flashMem.block[nr].deleteCount>flashMem.wearOutLimit) {
		// Zelle ggf. als defekt markieren, und löschen fehlschlagen lassen 
		if (FL_USE_RANDOM) { // Zufallsganerator für Ausfall bemühen
			r=(rand()%100); 
			if (r<flashMem.randomFailProbability) {
				flashMem.block[nr].dead = TRUE;	// Der Block ist tot
				return FALSE;					// Das Löschen schlägt fehl
			}
		}
		else {	// hartes Limit 
			flashMem.block[nr].dead = TRUE;		// Der Block ist tot
			return FALSE;						// Das Löschen schlägt fehl
		}
	}
	// falls bis hier nicht als defekt markiert: Block normal löschen

	for (i=0; i<FL_getPagesPerBlock(); i++) {
		// for each page in the block delete both data areas
		for (j=0; j<FL_getPageDataSize(); j++) 
			flashMem.block[nr].page[i].data[j]=0xFF; 
		for (j=0; j<FL_getPageSpareSize(); j++) 
			flashMem.block[nr].page[i].spare[j]=0xFF; 
	}
	flashMem.block[nr].deleteCount++; 
	return TRUE; 
}

uint16_t FL_writeData (uint16_t blockNr, uint16_t pageNr, uint16_t index, 
						uint16_t len, void *data) {
	uint8_t *bytePtr= (uint8_t*)data;
	uint32_t byteCount, i; 
	if (!FL_isInitialised(&flashMem)) return 0; // nix geschrieben
	// Parameter überprüfen und ggf. stutzen. 
	if (index>FL_getPageDataSize()) return 0;		// nix geschrieben
	if (len>FL_getPageDataSize()) byteCount=FL_getPageDataSize(); else byteCount=len; 
	if (index+byteCount > FL_getPageDataSize()) byteCount=FL_getPageDataSize()-index;
	// byteCount enthält nun die Anzahl der schreibbaren Bytes
	if (byteCount==0) return 0; // nix geschrieben
	// VerUNDen statt überschreiben!
	for (i=0; i<byteCount; i++) {
		flashMem.block[blockNr].page[pageNr].data[i+index] &= bytePtr[i]; 
	}
	return (uint16_t)byteCount; 
}

 
uint16_t FL_readData (uint16_t blockNr, uint16_t pageNr, uint16_t index, 
						uint16_t len, void *data) {
	uint8_t *bytePtr= (uint8_t*)data;
	uint32_t byteCount, i; 
	if (!FL_isInitialised(&flashMem)) return 0; // nix gelesen
	// Parameter überprüfen und ggf. stutzen. 
	if (index>FL_getPageDataSize()) return 0;		// nix gelesen
	if (len>FL_getPageDataSize()) byteCount=FL_getPageDataSize(); else byteCount=len; 
	if (index+byteCount > FL_getPageDataSize()) byteCount=FL_getPageDataSize()-index;
	// byteCount enthält nun die Anzahl der schreibbaren Bytes
	if (byteCount==0) return 0; // nix geschrieben
	// nun auslesen
	for (i=0; i<byteCount; i++) {
		bytePtr[i] = flashMem.block[blockNr].page[pageNr].data[i+index]; 
	}
	return (uint16_t)byteCount; 
	return 0; // nix gelesen
}

uint16_t FL_writeSpare (uint16_t blockNr, uint16_t pageNr, uint16_t index, 
						uint16_t len, void *data) {
	uint8_t *bytePtr= (uint8_t*)data;
	uint32_t byteCount, i; 
	if (!FL_isInitialised(&flashMem)) return 0; // nix geschrieben
	// Parameter überprüfen und ggf. stutzen. 
	if (index>FL_getPageSpareSize()) return 0;		// nix geschrieben
	if (len>FL_getPageSpareSize()) byteCount=FL_getPageSpareSize(); else byteCount=len; 
	if (index+byteCount > FL_getPageSpareSize()) byteCount=FL_getPageSpareSize()-index;
	// byteCount enthält nun die Anzahl der schreibbaren Bytes
	if (byteCount==0) return 0; // nix geschrieben
	// VerUNDen statt überschreiben!
	for (i=0; i<byteCount; i++) {
		flashMem.block[blockNr].page[pageNr].spare[i+index] &= bytePtr[i]; 
	}
	return (uint16_t)byteCount; 
}


uint16_t FL_readSpare (uint16_t blockNr, uint16_t pageNr, uint16_t index, 
						uint16_t len, void *data) {
	uint8_t *bytePtr= (uint8_t*)data;
	uint32_t byteCount, i; 
	if (!FL_isInitialised(&flashMem)) return 0; // nix gelesen
	// Parameter überprüfen und ggf. stutzen. 
	if (index>FL_getPageDataSize()) return 0;		// nix gelesen
	if (len>FL_getPageSpareSize()) byteCount=FL_getPageSpareSize(); else byteCount=len; 
	if (index+byteCount > FL_getPageSpareSize()) byteCount=FL_getPageSpareSize()-index;
	// byteCount enthält nun die Anzahl der schreibbaren Bytes
	if (byteCount==0) return 0; // nix geschrieben
	// nun auslesen
	for (i=0; i<byteCount; i++) {
		bytePtr[i] = flashMem.block[blockNr].page[pageNr].spare[i+index]; 
	}
	return (uint16_t)byteCount; 
	return 0; // nix gelesen
}


uint8_t FL_saveState(uint8_t blockCount, uint8_t *state) {
	uint16_t i; 
	if (!FL_isInitialised(&flashMem)) return FALSE;
	if (flashMem.stateStorage!=NULL) free (flashMem.stateStorage); 
	flashMem.stateStorageSize=blockCount;
	flashMem.stateStorage=(uint8_t*) malloc(blockCount*sizeof(uint8_t)*STATEBLOCKSIZE);
	for (i=0; i<blockCount*STATEBLOCKSIZE; i++)
		flashMem.stateStorage[i]=state[i];
	return TRUE; 
}


uint8_t FL_getStateSize(void) {
	return flashMem.stateStorageSize;
}


uint8_t *FL_restoreState(uint8_t *state) {
	uint16_t i; 
	if (!FL_isInitialised(&flashMem)) return NULL;
	if (flashMem.stateStorage==NULL) return NULL;
	for (i=0; i<FL_getStateSize()*STATEBLOCKSIZE; i++)
		state[i] = flashMem.stateStorage[i];
	return state; 
}

uint16_t FL_getPartitionCount () {
	return PARTITION_COUNT; 
}

uint16_t FL_getBlockCount () {
	return BLOCK_COUNT; 
}

uint16_t FL_getPagesPerBlock () {
	return PAGES_PER_BLOCK; 
}

uint32_t FL_getPageDataSize () {
	return PAGE_DATASIZE;
}

uint16_t FL_getPageSpareSize () {
	return PAGE_SPARESIZE; 
}



//////////////////////////////////////////////////////////////////////
// local function implementations


uint8_t FL_isInitialised(flashMem_t *flashHardware) {
// predicate, testing if the module is initialised 
	return (flashHardware->initialised == FLASH_INITIALISED); 
}

uint8_t FL_initFlash(flashMem_t *flashHardware) {
// initialises the flash datastructure without deleting it 
	time_t t;
	flashHardware->partitionCount = PARTITION_COUNT;
	flashHardware->blockCount = BLOCK_COUNT;
	flashHardware->pagesPerBlock = PAGES_PER_BLOCK;
	flashHardware->pageDataSize = PAGE_DATASIZE;
	flashHardware->pageSpareSize = PAGE_SPARESIZE; 
	if ((flashHardware->stateStorage!=NULL) && FL_isInitialised(flashHardware))
		free (flashHardware->stateStorage); 
	flashHardware->initialised=FLASH_INITIALISED;
	flashHardware->stateStorage=NULL; 
	flashHardware->stateStorageSize=0; 
	// initialisation of the simulation
	flashHardware->wearOutLimit=FL_WEAR_OUT_LIMIT; 
	flashHardware->randomFailProbability=FL_RANDOM_FAIL_PROBABILITY; 
	if (FL_USE_RANDOM) {
	    time(&t);
	    srand((unsigned int)t);   
	}

	return TRUE; 
}

uint8_t FL_deleteFlash(flashMem_t *flashHardware) {
// deletes the flash memory without resetting the deleteCounts
	uint16_t i;
	if (!FL_isInitialised(flashHardware)) return FALSE; 
	for (i=0; i<FL_getBlockCount(); i++) {
		FL_deleteBlock(i);
	}
	return TRUE; 
}

uint8_t FL_resetDeleteCounts(flashMem_t *flashHardware) {
// resets the deleteCount values of all blocks to 0, resets dead-flag too
	uint16_t i;
	if (!FL_isInitialised(flashHardware)) return FALSE; 
	for (i=0; i<FL_getBlockCount(); i++) {
		flashHardware->block[i].deleteCount=0; 
		flashHardware->block[i].dead=FALSE; 
	}
	return TRUE; 
}

