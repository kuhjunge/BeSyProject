/* Implementierung eines FTL's */
#include "ftl.h"

flash_t flashDevice;
int recentBlock = 0;
// Funktionen Allocator

// Funktionen Cleaner
// Nichts zu tun ? 

uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data, int useCleaner);
uint8_t readBlockIntern(flash_t *flashDevice, uint16_t block, uint16_t page, uint16_t index, uint8_t *data);

uint32_t getMapT(flash_t *flashDevice, int block, int seg){
	return flashDevice->mappingTable[(block * BLOCKSEGMENTS) + seg];
}

void setMapT(flash_t *flashDevice, int block, int seg, uint32_t v){
	flashDevice->mappingTable[(block * BLOCKSEGMENTS) + seg] = v;
}

void cleaner(flash_t *flashDevice){
	int i = recentBlock;
	int j = 0;
	int k = 0;
	int deleteCount = 0;
	int level = flashDevice->invalidCounter / (BLOCK_COUNT - flashDevice->freeBlocks);
	uint32_t adress;
	uint8_t temp[16];
	if (level < 1){
		level = 1;
	}
	while (k < BLOCK_COUNT && deleteCount < (BLOCKSEGMENTS / level) + 1){ // Solange noch nicht alle Bloecke durchlaufen wurden oder genung Bloecke gereinigt wurden
		if (flashDevice->blockArray[i].invalidCounter >= level && flashDevice->blockArray[i].status == used){ // Wenn Block über Schwellwert liegt und benutzt wird
			for (j = 0; j < BLOCKSEGMENTS; j++){ // Für jedes Segment eines Blockes
				if (flashDevice->blockArray[i].BlockStatus[j] == assigned){ // Wenn das Element benutzt wird
					adress = getMapT(flashDevice, i, j);
					readBlockIntern(flashDevice,i, j / PAGES_PER_BLOCK, j % PAGES_PER_BLOCK,  &temp);
					//count = writeBlock(flashDevice, adress, &temp);
					if (flashDevice->freeBlocks > 0){
						writeBlockIntern(flashDevice, adress, &temp,TRUE);
					}
					else {
						printf("Cleaner Zugriffsfehler!\n");
						printerr(flashDevice);
					}
				}
				flashDevice->blockArray[i].BlockStatus[j] = empty; // Segemente auf unused setzen
			}
			if (FL_deleteBlock(i) == TRUE){ // Wenn erfolgreich gelöscht
				flashDevice->invalidCounter = flashDevice->invalidCounter - flashDevice->blockArray[i].invalidCounter;
				flashDevice->blockArray[i].deleteCounter++; // block löschzähler hochsetzten
				flashDevice->blockArray[i].invalidCounter = 0; // Counter zurück setzen
				flashDevice->blockArray[i].status = ready; // Status auf Ready setzen
				flashDevice->freeBlocks++;
				deleteCount++;
			}
			else { // wenn nicht erfolgreich gelöscht
				printf("Bad Block!\n"); // badBlock
				flashDevice->blockArray[i].status = badBlock;
			}
			j = 0;
		}
		k++;
		for (j = 0; j < BLOCKSEGMENTS; j++){
			if ((flashDevice->blockArray[i].BlockStatus[j] != assigned && getMapT(flashDevice, i, j) != 0) ||
				(flashDevice->blockArray[i].BlockStatus[j] == assigned && getMapT(flashDevice, i, j) == 0) ||
				(flashDevice->blockArray[i].BlockStatus[j] > 2)){
				printf("Cleaner Konsistenzfehler!\n");
				printerr(flashDevice);
			}
		}
		i++;
		if (i >= BLOCK_COUNT){
			i = 0;
		}
	}
}

// Funktionen Wear-Leveler ([TC11]- Algorithmus)

// Hilfs Funktionen FLT

void printerr(flash_t *flashDevice){
	int i, j, invCo = 0, del = 0;
	int block = flashDevice->activeBlockPosition / BLOCK_COUNT;
	int segment = (flashDevice->activeBlockPosition % BLOCK_COUNT);
	char marker;
	char error;
	char userInput;
	printf("\nFehleranalyse mit 'j'\n");
	scanf_s("%c", &userInput);
	getchar();
	if (userInput == 'j')
	{
		printf("Freie Blocks: %i\nInvalide Segmente: %i (Schwellwert %i)\nAktuelle Schreibposition: %i (B:%i / S:%i)\n\n"
			, flashDevice->freeBlocks, flashDevice->invalidCounter, flashDevice->invalidCounter / (BLOCK_COUNT - flashDevice->freeBlocks), flashDevice->activeBlockPosition, block, segment);
		printf("Block | Status | Invalide Segmente | Loeschanzahl\n");
		for (i = 0; i < BLOCK_COUNT; i++)
		{
			printf("   %02i |   %i    |         %02i        |     %i\n", i
				, flashDevice->blockArray[i].status, flashDevice->blockArray[i].invalidCounter, flashDevice->blockArray[i].deleteCounter);
			invCo = invCo + flashDevice->blockArray[i].invalidCounter;
			del = del + flashDevice->blockArray[i].deleteCounter;
		}
		printf("      |Free:%02i |        %04i       |     %i\n", flashDevice->freeBlocks, invCo, del);
		printf("Block: ready (0) / used (1) / badBlock (2) \n");
		printf("Block fuer Block Detailanalyse? j / n \n");
		scanf_s("%c", &userInput);
		getchar();
		if (userInput == 'j')
		{
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
				printf(" Invalide Segmente: %i\n Loeschanzahl: %i\n\n", flashDevice->blockArray[i].invalidCounter, flashDevice->blockArray[i].deleteCounter);
				for (j = 0; j < BLOCKSEGMENTS; j++){
					marker = ' ';
					error = ' ';
					if (block == i && segment == j){ marker = 'S'; }
					if (flashDevice->blockArray[i].BlockStatus[j] != assigned && getMapT(flashDevice, i, j) != 0){ error = '!'; }
					if (flashDevice->blockArray[i].BlockStatus[j] == assigned && getMapT(flashDevice, i, j) == 0){ error = '!'; }
					if (flashDevice->blockArray[i].BlockStatus[j] > 2){ error = '?'; }
					printf("Segment %02i: Table: %03i - %i %c %c\n", j, getMapT(flashDevice, i, j), flashDevice->blockArray[i].BlockStatus[j], error, marker);
				}
				printf("Segment: empty (0) / assigned (1) / invalid (2)\n");
				getchar();
			}
		}
	}
}

void setFreeBlock(flash_t *flashDevice, BlockStatus_t bs){
	int i =0;
	flashDevice->blockArray[flashDevice->activeBlockPosition / BLOCK_COUNT].status = bs;
	flashDevice->freeBlocks--;
	do
	{
		recentBlock++;
		if (recentBlock == BLOCK_COUNT){
			recentBlock = 0;
		}
		i++;
	} while (flashDevice->blockArray[recentBlock].status != ready && i < BLOCK_COUNT);
	flashDevice->activeBlockPosition = recentBlock * BLOCK_COUNT;
}

// Gibt 512 Speicherplatz representation zurück [Blocksegment]
int mapping(flash_t *flashDevice, uint32_t index){
	uint32_t target;
	int i = 0;
	int x = MAPPING_TABLE_SIZE;
	for (i = 0; i < x; i++){
		target = getMapT(flashDevice, i / BLOCKSEGMENTS, i % BLOCKSEGMENTS);
		if (target == index){ return i; }
	}
	return x;
	/*
	Algorihtmus nicht so effizient wie HashMap, aber akutell keine Idee wie ich eine Hashmap für uint32_t in C implementiere
	*/
}

void invalidationOfOldIndex(flash_t *flashDevice, uint32_t index){ // ToDo: Prüft auf Null, erzeugt evtl fehler bei ID Null ?
	int old_block, i;
	int oldDataIndex = mapping(flashDevice, index); // Testen ob schon Eintrag mit diesem Index vorhanden ist
	if (oldDataIndex < MAPPING_TABLE_SIZE){
		i = BLOCKSEGMENTS;
		old_block = oldDataIndex / i;
		setMapT(flashDevice, oldDataIndex / BLOCKSEGMENTS, oldDataIndex % BLOCKSEGMENTS, 0);
		flashDevice->blockArray[old_block].BlockStatus[oldDataIndex - (old_block * i)] = invalid;
		flashDevice->invalidCounter++;
		flashDevice->blockArray[old_block].invalidCounter++;
	}
}

// Funktionen FLT
flash_t *mount(flashMem_t *flashHardware){
	int i, j;

	// flashDevice.flashHardware = flashHardware;
	// Initialisieren
	flashDevice.isErr = 0;
	for (i = 0; i < BLOCK_COUNT; i++){
		for (j = 0; j < PAGES_PER_BLOCK * (PAGE_DATASIZE / LOGICAL_BLOCK_DATASIZE); j++){
			flashDevice.blockArray[i].BlockStatus[j] = empty;
		}
		flashDevice.blockArray[i].invalidCounter = 0;
		flashDevice.blockArray[i].deleteCounter = 0;
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
/*	uint8_t stateSize = FL_getStateSize();
	uint8_t* state = (uint8_t*)malloc(sizeof(uint8_t) * stateSize); // allocate 
	if (stateSize > 0){
		// FL_getStateSize(void)
		FL_saveState(BLOCK_COUNT, *state);
	}
	// ToDo Datenstruktur speichern
	*/
	// Fehlerfall Datenstruktur mit Fehlermeldung zurück geben
	return NULL; // ToDo
}

uint8_t readBlockIntern(flash_t *flashDevice, uint16_t block, uint16_t page, uint16_t index, uint8_t *data){
	return FL_readData(block, page, index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Blocksegment auslesen
	// Todo: rückgabe boolischer wert über Readdata ?
}

uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	int i = mapping(flashDevice, index); // Mapping
	return readBlockIntern(flashDevice, i / BLOCKSEGMENTS, (i % BLOCKSEGMENTS) / PAGES_PER_BLOCK, ((i % BLOCKSEGMENTS) % PAGES_PER_BLOCK), data); // Blocksegment auslesen
	// Todo: rückgabe boolischer wert über Readdata ?
}

uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data, int useCleaner){

	int block = flashDevice->activeBlockPosition / BLOCK_COUNT;
	int page = (flashDevice->activeBlockPosition % BLOCK_COUNT) / PAGES_PER_BLOCK;
	int bp_index = (flashDevice->activeBlockPosition % BLOCK_COUNT) % PAGES_PER_BLOCK;
	int count;
	int constcheck;

	if (flashDevice->blockArray[block].status != ready){
		setFreeBlock(flashDevice, used); // Versuche einen alternativen Block zu finden
		printf("Fehler beim Schreiben des Datensatzes! Versuche alternativen Block!\n");
		if (flashDevice->blockArray[block].status != ready){
			printf("Fataler Fehler! Fehlerhafter Block Zugriff!\n");
			printerr(flashDevice);
			return FALSE;
		}
	}
	count = FL_writeData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Daten beschreiben
	if (count != LOGICAL_BLOCK_DATASIZE){ // Prüfen ob wirklich entsprechende Daten geschrieben wurden		}
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}
	invalidationOfOldIndex(flashDevice, index); // Alten Eintrag in Mapping Table und Block invalidieren
	setMapT(flashDevice, block, (page * PAGES_PER_BLOCK) + bp_index, index); // Setze Mapeintrag
	flashDevice->blockArray[block].BlockStatus[(page * PAGES_PER_BLOCK) + bp_index] = assigned; // Beschriebenes Segement merken

	// Auswahl des nächsten Schreibortes
	if ((page * PAGES_PER_BLOCK) + bp_index < BLOCKSEGMENTS - 1){ // position weiterzählen wenn innerhalb des selben blockes
		flashDevice->activeBlockPosition++;
	}
	else { // oder einen neuen Block auswählen
		// alten Block abschließen
		setFreeBlock(flashDevice, used); // Freien Block finden und nutzen
		if (flashDevice->freeBlocks < START_CLEANING + SPARE_BLOCKS && useCleaner == 0){ // Cleaner
			cleaner(flashDevice); // Clean
		}
		if (flashDevice->freeBlocks <= SPARE_BLOCKS && useCleaner == 0) { // Fehler falls zu wenig freie Blöcke da sind
			printf("Fehler! Festplatte zu voll! [ueber %i byte geschrieben]\n", (BLOCKSEGMENTS * (BLOCK_COUNT - SPARE_BLOCKS) * LOGICAL_BLOCK_DATASIZE));
			printerr(flashDevice);
			return FALSE;
		}
	}
	return TRUE;
}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	return writeBlockIntern(flashDevice, index, data, 0);
}