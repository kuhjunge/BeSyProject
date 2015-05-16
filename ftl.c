/* Implementierung eines FTL's */
#include "ftl.h"

flash_t flashDevice; // Datenstruktur
uint16_t recentBlock = 0; // ToDo: Marker für die Blockauswahl (evtl in Datenstruktur schieben?)

// Funktionen Allocator
////////////////////////////////////////////////////////////////////

/* Getter Map 
 * Übersetzt die physikalische Position in die  logischen Nummer eines Datenblocks
 * Gibt die logische Nummer eines Datenblocks zurück
 */
uint32_t getMapT(flash_t *flashDevice, int block, int seg);

/* Setter Map
 * Verknüpft die logischen Nummer eines Datenblocks mit der physikalische Position in der Tabelle
 * Gibt die logische Nummer eines Datenblocks zurück
 */
void setMapT(flash_t *flashDevice, int block, int seg, uint32_t v);

/* mapping
 * Übersetzt die logischen Nummer eines Datenblocks in die physikalische Position 
 * Gibt die physikalische Position eines Datenblocks zurück
 */
uint16_t mapping(flash_t *flashDevice, uint32_t index);

/* 
 * Löscht einen Eintrag in der Map und invalidiert das dazugehörige Segment
 */
void invalidationOfOldIndex(flash_t *flashDevice, uint16_t block, uint16_t segment);

// Funktionen Cleaner
////////////////////////////////////////////////////////////////////
/*
 * Zusammenfassung der geschriebenen Daten (Bereinigung von invalidierten Segmenten), Löschen kompletter Blöcke. 
 */
void cleaner(flash_t *flashDevice);

/*
 * Setzt die Leseposition auf einen neuen freien Block und schließt den alten Block ab
 */
void setFreeBlock(flash_t *flashDevice, BlockStatus_t bs);

// Funktionsimplementation Wear-Leveler ([TC11]- Algorithmus)
////////////////////////////////////////////////////////////////////

// Funktionen FTL lokal
////////////////////////////////////////////////////////////////////
/*
 * Schreibt einen Datenblock an der angegebene Indexposition auf den Flashspeicher, der
 * mit der in flashDevice übergebenen Datenstruktur verwaltet wird.
 * flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 * dieses Flash-Datenträgers dient.
 * index ist die Nummer des zu schreibenden Blocks auf dem Flashdevice
 * data ist ein Pointer auf den Quelldatenblock.
 * Der zusätzliche useCleaner Parameter gibt an ob die Funktion vom Cleaner aufgerufen wird, 
 * in diesem Fall ruft sie intern den Cleaner nicht auf und nutzt auch Spare Blöcke
 * Der Rückgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data, int useCleaner, uint16_t block_index, uint16_t segment_index);

/*
* Liest einen Datenblock an der angegebene physikalischen vom Flashspeicher, der mit der
* in flashDevice übergebenen Datenstruktur verwaltet wird.
* flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
* dieses Flash-Datenträgers dient.
* index ist die Nummer des zu lesenden Blocks.
* data ist ein Pointer auf einen ausreichend großen Datenbereich, in den die zu lesenden
* Daten kopiert werden.
* Der Rückgabewert ist als Boolescher Wert zu interpretieren.
*/
uint8_t readBlockIntern(flash_t *flashDevice, uint16_t block, uint16_t page, uint16_t index, uint8_t *data);

// Lokale Funktionsimplementation Allocator
////////////////////////////////////////////////////////////////////
uint32_t getMapT(flash_t *flashDevice, int block, int seg){
	return flashDevice->mappingTable[(block * BLOCKSEGMENTS) + seg];
}

void setMapT(flash_t *flashDevice, int block, int seg, uint32_t v){
	flashDevice->mappingTable[(block * BLOCKSEGMENTS) + seg] = v;
}

// Gibt 512 Speicherplatz representation zurück [Blocksegment]
uint16_t mapping(flash_t *flashDevice, uint32_t index){
	uint32_t target;
	uint16_t i = 0;
	uint16_t x = MAPPING_TABLE_SIZE;
	for (i = 0; i < x; i++){
		target = getMapT(flashDevice, i / BLOCKSEGMENTS, i % BLOCKSEGMENTS);
		if (target == index){ return i; }
	}
	return x;
	/*
	ToDo: Algorihtmus nicht so effizient wie HashMap, aber akutell keine Idee wie ich eine Hashmap für uint32_t in C implementiere
	*/
}

// Basis FTL Funktion oder Allocator Funktion, evtl aufsplitten?  
void invalidationOfOldIndex(flash_t *flashDevice, uint16_t block, uint16_t segment){ // ToDo: Prüft auf Null, erzeugt evtl fehler bei ID Null ?
	setMapT(flashDevice, block , segment , 0);
	flashDevice->blockArray[block].segmentStatus[segment] = invalid;
	flashDevice->invalidCounter++;
	flashDevice->blockArray[block].invalidCounter++;
	//}
}

// Lokale Funktionsimplementation  Cleaner
////////////////////////////////////////////////////////////////////
void cleaner(flash_t *flashDevice){
	uint16_t i = recentBlock;
	uint16_t j = 0;
	uint16_t k = 0;
	uint16_t deleteCount = 0;
	uint16_t level = flashDevice->invalidCounter / (FL_getBlockCount() - flashDevice->freeBlocks); //Anzahl der zu bereinigen Blocks, dynamisch berechnet
	uint32_t adress;
	uint8_t temp[16];
	if (level < 1){
		level = 1;
	}
	while (k < FL_getBlockCount () && deleteCount < (BLOCKSEGMENTS / level) + 1){ // Solange noch nicht alle Bloecke durchlaufen wurden oder genung Bloecke gereinigt wurden
		if (flashDevice->blockArray[i].invalidCounter >= level && flashDevice->blockArray[i].status == used){ // Wenn Block über Schwellwert liegt und benutzt wird
			for (j = 0; j < BLOCKSEGMENTS; j++){ // Für jedes Segment eines Blockes
				if (flashDevice->blockArray[i].segmentStatus[j] == assigned){ // Wenn das Element benutzt wird
					adress = getMapT(flashDevice, i, j);
					readBlockIntern(flashDevice,i, j / FL_getPagesPerBlock (), j % FL_getPagesPerBlock (),  temp);
					if (flashDevice->freeBlocks > 0){
						writeBlockIntern(flashDevice, adress, temp,TRUE,i,j);
					}
					else {
						printf("Cleaner Zugriffsfehler!\n");
						printerr(flashDevice);
					}
				}
				flashDevice->blockArray[i].segmentStatus[j] = empty; // Segemente auf unused setzen
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
			if ((flashDevice->blockArray[i].segmentStatus[j] != assigned && getMapT(flashDevice, i, j) != 0) ||
				(flashDevice->blockArray[i].segmentStatus[j] == assigned && getMapT(flashDevice, i, j) == 0) ||
				(flashDevice->blockArray[i].segmentStatus[j] > 2)){
				printf("Cleaner Konsistenzfehler!\n");
				printerr(flashDevice);
			}
		}
		i++;
		if (i >= FL_getBlockCount ()){
			i = 0;
		}
	}
}

void setFreeBlock(flash_t *flashDevice, BlockStatus_t bs){
	uint16_t i = 0;
	flashDevice->blockArray[flashDevice->activeBlockPosition / FL_getBlockCount()].status = bs;
	flashDevice->freeBlocks--;
	do
	{
		recentBlock++;
		if (recentBlock == FL_getBlockCount()){
			recentBlock = 0;
		}
		i++;
	} while (flashDevice->blockArray[recentBlock].status != ready && i < FL_getBlockCount());
	flashDevice->activeBlockPosition = recentBlock * FL_getBlockCount();
}

// Funktionsimplementation Wear-Leveler ([TC11]- Algorithmus)
////////////////////////////////////////////////////////////////////

// Lokale Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

uint8_t readBlockIntern(flash_t *flashDevice, uint16_t block, uint16_t page, uint16_t index, uint8_t *data){
	uint16_t count = FL_readData(block, page, index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Blocksegment auslesen
	if (count != LOGICAL_BLOCK_DATASIZE){ // Prüfen ob wirklich entsprechende Daten geschrieben wurden		}
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}
	return TRUE;
}

uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data, int useCleaner, uint16_t block_index, uint16_t segment_index){

	uint16_t block = flashDevice->activeBlockPosition / FL_getBlockCount();
	uint16_t page = (flashDevice->activeBlockPosition % FL_getBlockCount()) / FL_getPagesPerBlock();
	uint16_t bp_index = (flashDevice->activeBlockPosition % FL_getBlockCount()) % FL_getPagesPerBlock();
	uint16_t count;
	uint32_t index_old;
	if (index < 1 && block_index == FL_getBlockCount() +1 ){ // Fehlerhafter Index wurde übergeben
		printf("Fehlerhafter Index wurde uebergeben. Index darf nicht < 1  sein!\n");
		printerr(flashDevice);
		return FALSE;
	}

	if (flashDevice->blockArray[block].status != ready){ // Der Block auf dem geschrieben wird, ist nicht beschreibbar
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
	if (index > 0){
		index_old = mapping(flashDevice, index);
		if (index_old < MAPPING_TABLE_SIZE){
			invalidationOfOldIndex(flashDevice, index_old / BLOCKSEGMENTS, index_old % BLOCKSEGMENTS); // Alten Eintrag in Mapping Table und Block invalidieren
		} 
	}
	else {
		index = getMapT(flashDevice, block_index, segment_index); // alten Eintrag zwischenspeichern
		invalidationOfOldIndex(flashDevice, block_index, segment_index); // Alten Eintrag in Mapping Table und Block invalidieren
	}
	setMapT(flashDevice, block, (page * FL_getPagesPerBlock()) + bp_index, index); // Setze Mapeintrag
	flashDevice->blockArray[block].segmentStatus[(page * FL_getPagesPerBlock()) + bp_index] = assigned; // Beschriebenes Segement merken

	// Auswahl des nächsten Schreibortes
	if ((page * FL_getPagesPerBlock()) + bp_index < BLOCKSEGMENTS - 1){ // position weiterzählen wenn innerhalb des selben blockes
		flashDevice->activeBlockPosition++;
	}
	else { // oder einen neuen Block auswählen
		// alten Block abschließen
		setFreeBlock(flashDevice, used); // Freien Block finden und nutzen
		if (flashDevice->freeBlocks < START_CLEANING + SPARE_BLOCKS && useCleaner == FALSE){ // Cleaner
			cleaner(flashDevice); // Clean
		}
		if (flashDevice->freeBlocks <= SPARE_BLOCKS && useCleaner == FALSE) { // Fehler falls zu wenig freie Blöcke da sind
			printf("Fehler! Festplatte zu voll! [ueber %i byte geschrieben]\n", (BLOCKSEGMENTS * (FL_getBlockCount() - SPARE_BLOCKS) * LOGICAL_BLOCK_DATASIZE) -1);
			printerr(flashDevice);
			return FALSE;
		}
	}
	return TRUE;
}

// Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

flash_t * mount(flashMem_t *flashHardware){
	uint32_t i, j;
	uint8_t* state; // pointer auf dem der Flash speicher konserviert werden soll
	flash_t * flde;

	uint8_t  myReadData;
	uint16_t count;
	// Laden der Datenstruktur
	count = FL_readSpare(0, 0, 0, 1, &myReadData);
	if (count < 1){
		printf("Kein initialisierter Flash Speicher gefunden!\n");
		return NULL;
	}
	if (FL_getStateSize() > 0){
		state = (uint8_t*)malloc(sizeof(uint8_t)* FL_getStateSize()); // allocate 
		FL_restoreState(state);
		flde = state; // ToDo: BUG!!!! Funktioniert noch nicht	
		flashDevice = *flde;
		flashDevice.isNoErr = TRUE;
		printf("Mounting Punkt wiederhergestellt!\n"); 
	}
	else {
		// Initialisieren
		flashDevice.isNoErr = TRUE;
		for (i = 0; i < FL_getBlockCount(); i++){
			for (j = 0; j < FL_getPagesPerBlock() * (FL_getPageDataSize() / LOGICAL_BLOCK_DATASIZE); j++){
				flashDevice.blockArray[i].segmentStatus[j] = empty;
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
		flashDevice.freeBlocks = FL_getBlockCount();
		flde = &flashDevice;
		printf("SSD initialisiert!\n");
	}
	return flde;
}

flash_t *unmount(flash_t *flashDevice){
	//uint8_t* state; // pointer auf dem der Flash speicher konserviert werden soll
	uint16_t sizeArray, sizeBlock;
	if (flashDevice == NULL) return FALSE;
	sizeArray = sizeof(*flashDevice) / 8;
	sizeBlock = ((sizeof(*flashDevice) / 512) +1) ;
	printf("Groesse der geunmounteten Datenstruktur: %i (%i)\n", sizeBlock, sizeArray);
	flashDevice->isNoErr = FL_saveState(sizeBlock, flashDevice);
	return flashDevice; 
}

uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	uint16_t i;
	if (flashDevice == NULL) return FALSE;
	i = mapping(flashDevice, index); // Mapping
	return readBlockIntern(flashDevice, i / BLOCKSEGMENTS, (i % BLOCKSEGMENTS) / FL_getPagesPerBlock (), ((i % BLOCKSEGMENTS) % FL_getPagesPerBlock ()), data); // Blocksegment auslesen

}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	if (flashDevice == NULL) return FALSE;
	return writeBlockIntern(flashDevice, index, data, FALSE, FL_getBlockCount() + 1, 0);
}

// DEBUG Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

void printerr(flash_t *flashDevice){
	uint16_t i, j, invCo = 0, del = 0, block, segment, calcLevel = 0;
	char marker, error,	userInput, unMountErr = 'n';
	if (flashDevice == NULL) return;
	block = flashDevice->activeBlockPosition / FL_getBlockCount();
	segment = (flashDevice->activeBlockPosition % FL_getBlockCount());
	printf("\nFehleranalyse mit 'j'\n");
	scanf_s("%c", &userInput);
	getchar();
	if (userInput == 'j')
	{
		if ((FL_getBlockCount() - flashDevice->freeBlocks) > 0){
			calcLevel = flashDevice->invalidCounter / (FL_getBlockCount() - flashDevice->freeBlocks);
		}
		if (flashDevice->isNoErr == TRUE){
			unMountErr = 'j';
		}
		printf("Freie Blocks: %i\nInvalide Segmente: %i (Schwellwert %i)\nAktuelle Schreibposition: %i (B:%i / S:%i)\nUnmount Fehler: %c \n\n"

			, flashDevice->freeBlocks, flashDevice->invalidCounter, calcLevel, flashDevice->activeBlockPosition, block, segment, unMountErr);
		printf("Block | Status | Invalide Segmente | Loeschanzahl\n");
		for (i = 0; i < FL_getBlockCount(); i++)
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
			for (i = 0; i < FL_getBlockCount(); i++)
			{
				printf("\nBlock %02i:\n", i);
				if (flashDevice->blockArray[i].status == ready){
					printf(" Status: Ready (0)\n");
				}
				else if (flashDevice->blockArray[i].status == used){
					printf(" Status. Used (1)\n");
				}
				else{
					printf(" Status: Bad Block (2)\n");
				}
				printf(" Invalide Segmente: %i\n Loeschanzahl: %i\n\n", flashDevice->blockArray[i].invalidCounter, flashDevice->blockArray[i].deleteCounter);
				for (j = 0; j < BLOCKSEGMENTS; j++){
					marker = ' ';
					error = ' ';
					if (block == i && segment == j){ marker = 'S'; }
					if (flashDevice->blockArray[i].segmentStatus[j] != assigned && getMapT(flashDevice, i, j) != 0){ error = '!'; }
					if (flashDevice->blockArray[i].segmentStatus[j] == assigned && getMapT(flashDevice, i, j) == 0){ error = '!'; }
					if (flashDevice->blockArray[i].segmentStatus[j] > 2){ error = '?'; }
					printf("Segment %02i: Table: %03i - %i %c %c\n", j, getMapT(flashDevice, i, j), flashDevice->blockArray[i].segmentStatus[j], error, marker);
				}
				printf("Segment: empty (0) / assigned (1) / invalid (2)\n");
				getchar();
			}
		}
	}
}