/* Implementierung eines FTL's */
#include "ftl.h"

flash_t flashDevice; // Datenstruktur
int recentBlock = 0; // ToDo: Marker f�r die Blockauswahl (evtl in Datenstruktur schieben?)

// Funktionen Allocator
////////////////////////////////////////////////////////////////////

/* Getter Map 
 * �bersetzt die physikalische Position in die  logischen Nummer eines Datenblocks
 * Gibt die logische Nummer eines Datenblocks zur�ck
 */
uint32_t getMapT(flash_t *flashDevice, int block, int seg);

/* Setter Map
 * Verkn�pft die logischen Nummer eines Datenblocks mit der physikalische Position in der Tabelle
 * Gibt die logische Nummer eines Datenblocks zur�ck
 */
void setMapT(flash_t *flashDevice, int block, int seg, uint32_t v);

/* mapping
 * �bersetzt die logischen Nummer eines Datenblocks in die physikalische Position 
 * Gibt die physikalische Position eines Datenblocks zur�ck
 */
int mapping(flash_t *flashDevice, uint32_t index);

/* 
 * L�scht einen Eintrag in der Map und invalidiert das dazugeh�rige Segment
 */
void invalidationOfOldIndex(flash_t *flashDevice, uint32_t index);

// Funktionen Cleaner
////////////////////////////////////////////////////////////////////
/*
 * Zusammenfassung der geschriebenen Daten (Bereinigung von invalidierten Segmenten), L�schen kompletter Bl�cke. 
 */
void cleaner(flash_t *flashDevice);

/*
 * Setzt die Leseposition auf einen neuen freien Block und schlie�t den alten Block ab
 */
void setFreeBlock(flash_t *flashDevice, BlockStatus_t bs);

// Funktionsimplementation Wear-Leveler ([TC11]- Algorithmus)
////////////////////////////////////////////////////////////////////

// Funktionen FTL lokal
////////////////////////////////////////////////////////////////////
/*
 * Schreibt einen Datenblock an der angegebene Indexposition auf den Flashspeicher, der
 * mit der in flashDevice �bergebenen Datenstruktur verwaltet wird.
 * flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 * dieses Flash-Datentr�gers dient.
 * index ist die Nummer des zu schreibenden Blocks auf dem Flashdevice
 * data ist ein Pointer auf den Quelldatenblock.
 * Der zus�tzliche useCleaner Parameter gibt an ob die Funktion vom Cleaner aufgerufen wird, 
 * in diesem Fall ruft sie intern den Cleaner nicht auf und nutzt auch Spare Bl�cke
 * Der R�ckgabewert ist als Boolescher Wert zu interpretieren. */
uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data, int useCleaner);

/*
* Liest einen Datenblock an der angegebene physikalischen vom Flashspeicher, der mit der
* in flashDevice �bergebenen Datenstruktur verwaltet wird.
* flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
* dieses Flash-Datentr�gers dient.
* index ist die Nummer des zu lesenden Blocks.
* data ist ein Pointer auf einen ausreichend gro�en Datenbereich, in den die zu lesenden
* Daten kopiert werden.
* Der R�ckgabewert ist als Boolescher Wert zu interpretieren.
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

// Gibt 512 Speicherplatz representation zur�ck [Blocksegment]
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
	ToDo: Algorihtmus nicht so effizient wie HashMap, aber akutell keine Idee wie ich eine Hashmap f�r uint32_t in C implementiere
	*/
}

// Basis FTL Funktion oder Allocator Funktion, evtl aufsplitten?  
void invalidationOfOldIndex(flash_t *flashDevice, uint32_t index){ // ToDo: Pr�ft auf Null, erzeugt evtl fehler bei ID Null ?
	int old_block, i;
	int oldDataIndex = mapping(flashDevice, index); // Testen ob schon Eintrag mit diesem Index vorhanden ist
	if (oldDataIndex < MAPPING_TABLE_SIZE){
		i = BLOCKSEGMENTS;
		old_block = oldDataIndex / i;
		setMapT(flashDevice, oldDataIndex / BLOCKSEGMENTS, oldDataIndex % BLOCKSEGMENTS, 0);
		flashDevice->blockArray[old_block].segmentStatus[oldDataIndex - (old_block * i)] = invalid;
		flashDevice->invalidCounter++;
		flashDevice->blockArray[old_block].invalidCounter++;
	}
}

// Lokale Funktionsimplementation  Cleaner
////////////////////////////////////////////////////////////////////
void cleaner(flash_t *flashDevice){
	int i = recentBlock;
	int j = 0;
	int k = 0;
	uint16_t deleteCount = 0;
	uint16_t level = flashDevice->invalidCounter / (FL_getBlockCount() - flashDevice->freeBlocks); //Anzahl der zu bereinigen Blocks, dynamisch berechnet
	uint32_t adress;
	uint8_t temp[16];
	if (level < 1){
		level = 1;
	}
	while (k < FL_getBlockCount () && deleteCount < (BLOCKSEGMENTS / level) + 1){ // Solange noch nicht alle Bloecke durchlaufen wurden oder genung Bloecke gereinigt wurden
		if (flashDevice->blockArray[i].invalidCounter >= level && flashDevice->blockArray[i].status == used){ // Wenn Block �ber Schwellwert liegt und benutzt wird
			for (j = 0; j < BLOCKSEGMENTS; j++){ // F�r jedes Segment eines Blockes
				if (flashDevice->blockArray[i].segmentStatus[j] == assigned){ // Wenn das Element benutzt wird
					adress = getMapT(flashDevice, i, j);
					readBlockIntern(flashDevice,i, j / FL_getPagesPerBlock (), j % FL_getPagesPerBlock (),  temp);
					if (flashDevice->freeBlocks > 0){
						writeBlockIntern(flashDevice, adress, temp,TRUE);
					}
					else {
						printf("Cleaner Zugriffsfehler!\n");
						printerr(flashDevice);
					}
				}
				flashDevice->blockArray[i].segmentStatus[j] = empty; // Segemente auf unused setzen
			}
			if (FL_deleteBlock(i) == TRUE){ // Wenn erfolgreich gel�scht
				flashDevice->invalidCounter = flashDevice->invalidCounter - flashDevice->blockArray[i].invalidCounter;
				flashDevice->blockArray[i].deleteCounter++; // block l�schz�hler hochsetzten
				flashDevice->blockArray[i].invalidCounter = 0; // Counter zur�ck setzen
				flashDevice->blockArray[i].status = ready; // Status auf Ready setzen
				flashDevice->freeBlocks++;
				deleteCount++;
			}
			else { // wenn nicht erfolgreich gel�scht
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
	int i = 0;
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
	if (count != LOGICAL_BLOCK_DATASIZE){ // Pr�fen ob wirklich entsprechende Daten geschrieben wurden		}
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}
	return TRUE;
}

uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data, int useCleaner){

	int block = flashDevice->activeBlockPosition / FL_getBlockCount();
	int page = (flashDevice->activeBlockPosition % FL_getBlockCount()) / FL_getPagesPerBlock();
	int bp_index = (flashDevice->activeBlockPosition % FL_getBlockCount()) % FL_getPagesPerBlock();
	uint16_t count;
	if (index < 1){ // Fehlerhafter Index wurde �bergeben
		printf("Fehlerhafter Index wurde uebergeben. Index darf nicht < 1  sein!\n");
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
	if (count != LOGICAL_BLOCK_DATASIZE){ // Pr�fen ob wirklich entsprechende Daten geschrieben wurden		}
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}
	invalidationOfOldIndex(flashDevice, index); // Alten Eintrag in Mapping Table und Block invalidieren
	setMapT(flashDevice, block, (page * FL_getPagesPerBlock()) + bp_index, index); // Setze Mapeintrag
	flashDevice->blockArray[block].segmentStatus[(page * FL_getPagesPerBlock()) + bp_index] = assigned; // Beschriebenes Segement merken

	// Auswahl des n�chsten Schreibortes
	if ((page * FL_getPagesPerBlock()) + bp_index < BLOCKSEGMENTS - 1){ // position weiterz�hlen wenn innerhalb des selben blockes
		flashDevice->activeBlockPosition++;
	}
	else { // oder einen neuen Block ausw�hlen
		// alten Block abschlie�en
		setFreeBlock(flashDevice, used); // Freien Block finden und nutzen
		if (flashDevice->freeBlocks < START_CLEANING + SPARE_BLOCKS && useCleaner == 0){ // Cleaner
			cleaner(flashDevice); // Clean
		}
		if (flashDevice->freeBlocks <= SPARE_BLOCKS && useCleaner == 0) { // Fehler falls zu wenig freie Bl�cke da sind
			printf("Fehler! Festplatte zu voll! [ueber %i byte geschrieben]\n", (BLOCKSEGMENTS * (FL_getBlockCount() - SPARE_BLOCKS) * LOGICAL_BLOCK_DATASIZE));
			printerr(flashDevice);
			return FALSE;
		}
	}
	return TRUE;
}

// Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

flash_t *mount(flashMem_t *flashHardware){
	uint32_t i, j;

	// flashDevice.flashHardware = flashHardware;
	// Initialisieren
	flashDevice.isErr = 0;
	for (i = 0; i < FL_getBlockCount (); i++){
		for (j = 0; j < FL_getPagesPerBlock () * ( FL_getPageDataSize () / LOGICAL_BLOCK_DATASIZE); j++){
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
	flashDevice.freeBlocks = FL_getBlockCount ();
	//uint8_t *FL_restoreState(unit8_t *state)

	//ToDo: Datenstruktur laden
	return &flashDevice; // ToDo
}

flash_t *unmount(flash_t *flashDevice){
/*	uint8_t stateSize = FL_getStateSize();
	uint8_t* state = (uint8_t*)malloc(sizeof(uint8_t) * stateSize); // allocate 
	if (stateSize > 0){
		// FL_getStateSize(void)
		FL_saveState(FL_getBlockCount (), *state);
	}
	// ToDo Datenstruktur speichern
	*/
	// Fehlerfall Datenstruktur mit Fehlermeldung zur�ck geben
	return NULL; // ToDo
}

uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	int i = mapping(flashDevice, index); // Mapping
	return readBlockIntern(flashDevice, i / BLOCKSEGMENTS, (i % BLOCKSEGMENTS) / FL_getPagesPerBlock (), ((i % BLOCKSEGMENTS) % FL_getPagesPerBlock ()), data); // Blocksegment auslesen

}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	return writeBlockIntern(flashDevice, index, data, 0);
}

// DEBUG Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

void printerr(flash_t *flashDevice){
	int i, j, invCo = 0, del = 0;
	int block = flashDevice->activeBlockPosition / FL_getBlockCount();
	int segment = (flashDevice->activeBlockPosition % FL_getBlockCount());
	char marker;
	char error;
	char userInput;
	printf("\nFehleranalyse mit 'j'\n");
	scanf_s("%c", &userInput);
	getchar();
	if (userInput == 'j')
	{
		printf("Freie Blocks: %i\nInvalide Segmente: %i (Schwellwert %i)\nAktuelle Schreibposition: %i (B:%i / S:%i)\n\n"

			, flashDevice->freeBlocks, flashDevice->invalidCounter, flashDevice->invalidCounter / (FL_getBlockCount() - flashDevice->freeBlocks), flashDevice->activeBlockPosition, block, segment);
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
					printf(" Status: Invalid (2)\n");
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