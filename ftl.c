/* Implementierung eines FTL's */
#include "ftl.h"

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
uint16_t mapping(flash_t *flashDevice, uint32_t index);

/*
 * Gibt Auskunft �ber den Status eines Elements
 */
StatusPageElem_t segmentStatus(flash_t *flashDevice, uint16_t block, uint16_t segment);

/* 
 * L�scht einen Eintrag in der Map und invalidiert das dazugeh�rige Segment
 */
void invalidationOfOldIndex(flash_t *flashDevice, uint16_t block, uint16_t segment);

// Funktionen WearLeveling
////////////////////////////////////////////////////////////////////

/*
 *	Schreibt ein Datensegment in einen Block
 *	Der R�ckgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t writeSegmentToBlock(flash_t* flashDevice, uint8_t* segment, uint16_t block);

/*
 *	WearLeveling Algorithmus nach [TC11]
 *	�bergabeparameter ist eine Instanz von flash_t und der gerade gel�schte Block
 */
void wearLeveling(flash_t* flashDevice, uint16_t deletedBlock);

// Funktionen Garbage Collector
////////////////////////////////////////////////////////////////////
/*
 * Zusammenfassung der geschriebenen Daten (Bereinigung von invalidierten Segmenten), L�schen kompletter Bl�cke. 
 */
void garbageCollector(flash_t *flashDevice);

void cleanBlock(flash_t *flashDevice, uint16_t block);

// Funktionen FTL lokal
////////////////////////////////////////////////////////////////////
/*
 *	Schreibt Daten in aktBlock und sucht, falls der voll ist, einen neuen Block zum beschreiben
 */
void writeIntern(flash_t *flashDevice, uint8_t *data);

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


// Funktionsimplementation Wear-Leveler ([TC11]- Algorithmus)
////////////////////////////////////////////////////////////////////


uint8_t writeSegmentToBlock(flash_t* flashDevice, uint8_t* segment, uint16_t block){
	uint16_t count = 0;
	uint16_t page = flashDevice->blockArray[block].writePos  / FL_getPagesPerBlock();
	uint16_t bp_index = flashDevice->blockArray[block].writePos % FL_getPagesPerBlock();

	//Abfrage, ob in den zu schreibender Block beschreibbar ist
	if( flashDevice->blockArray[block].status != ready){
		return FALSE;
	}

	count = FL_writeData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, segment); 
	// Pr�fen ob wirklich entsprechende Daten geschrieben wurden	
	if (count != LOGICAL_BLOCK_DATASIZE){ 	
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		//TODO als BadBlock markieren, da Datensatz nicht richtig geschrieben wurde!
		return FALSE;
	}

	flashDevice->blockArray[block].writePos++;
	// Block ist komplett gef�llt	
	if( flashDevice->blockArray[block].writePos == BLOCKSEGMENTS -1 ){
		flashDevice->blockArray[block].status = used;		
	}
	return TRUE;
}

void wearLeveling(flash_t* flashDevice, uint16_t deletedBlock){
	uint16_t p, i;	
	uint8_t* data[BLOCKSEGMENTS];
	uint8_t* tempSegment;
	ListElem_t* position;
	uint32_t tempAllocData;
	uint16_t tempBlock;

	//Average Recalculation gesamt
	flashDevice->AVG += (1 / FL_getBlockCount());

	//erase operation in neutral pool?	
	if( isElementOfList(flashDevice->neutralPool, deletedBlock) == TRUE){
		//Average Recalculation Neutral
		recalculationAVG( flashDevice->neutralPool );

		//check condition 1 und 2
		while(flashDevice->blockArray[showLastBlock(flashDevice->neutralPool)].deleteCounter > flashDevice->AVG + THETA){
			addBlock(flashDevice->hotPool, getLastBlock(flashDevice->neutralPool));
		}
		while(flashDevice->blockArray[showFirstBlock(flashDevice->neutralPool)].deleteCounter < flashDevice->AVG - THETA){
			addBlock(flashDevice->coldPool, getFirstBlock(flashDevice->neutralPool));
		}

		// kopiere Inhalt zwischen, l�sche deletedBlock und schreibe Inhalt neu
		for(p = 0; p < FL_getPagesPerBlock(); p++){
				for(i = 0; i < FL_getPageDataSize()/LOGICAL_BLOCK_DATASIZE; i++){
					if( segmentStatus(flashDevice, deletedBlock, (FL_getPagesPerBlock()*p)+i ) == assigned){
						readBlockIntern( flashDevice, deletedBlock, p, i, data[(FL_getPagesPerBlock()*p)+i ]);
					}
				}
			}
		cleanBlock(flashDevice, deletedBlock);
		for(i = 0; i < BLOCKSEGMENTS; i++){
			writeIntern(flashDevice, data[i]);
		}

		return;
	}
	//erase operation in hot pool
	if( isElementOfList(flashDevice->hotPool, deletedBlock) == TRUE){
		//Average Recalculation Hot
		recalculationAVG( flashDevice->hotPool );

		//check condition 3
		if(flashDevice->blockArray[deletedBlock].deleteCounter > flashDevice->hotPool->AVG + DELTA ){
			//step 1 kopiere valide Segmente in tempBlock			
			for(p = 0; p < FL_getPagesPerBlock(); p++){
				for(i = 0; i < FL_getPageDataSize()/LOGICAL_BLOCK_DATASIZE; i++){
					if( segmentStatus(flashDevice, deletedBlock, (FL_getPagesPerBlock()*p)+i ) == assigned){
						readBlockIntern( flashDevice, deletedBlock, p, i, data[(FL_getPagesPerBlock()*p)+i ]);
					}
				}
			}
			//step 2 l�sche Block und setze Z�hler hoch
			cleanBlock(flashDevice, deletedBlock);
			//step 3 kopiere alle validen Segmente aus Min(neutralPool) in deletedBlock
			for(p = 0; p < FL_getPagesPerBlock(); p++){
				for(i = 0; i < FL_getPageDataSize()/LOGICAL_BLOCK_DATASIZE; i++){
					if( segmentStatus(flashDevice, deletedBlock, (FL_getPagesPerBlock()*p)+i ) == assigned){
						readBlockIntern( flashDevice, showFirstBlock(flashDevice->neutralPool),p, i, tempSegment);
						writeSegmentToBlock(flashDevice, tempSegment, deletedBlock);
					}
				}
			}
			//step 4 kopiere aus tempBlock(data) in MAX(neutralPool)
			position = flashDevice->neutralPool->last;
			for(i = 0; i < BLOCKSEGMENTS; i++){
				p = writeSegmentToBlock(flashDevice, data[i], position->blockNr);
				// falls das Segment nicht geschrieben wurde
				if(p == FALSE){
					position = getPrevElement(position);
					if(position == NULL){
						printf("Fehler im WearLevelingAlgorithmus in step 4. Kein beschreibbarer Block mehr im neutralPool vorhanden\n");
					}
				}
			}
			//step 5 update der Adressen; deletedBlock <-> showFirstBlock(flashDevice->neutralPool)
			tempBlock = showFirstBlock(flashDevice->neutralPool);
			for(i = 0; i < BLOCKSEGMENTS; i++){
				tempAllocData = getMapT(flashDevice, deletedBlock, i);
				setMapT(flashDevice, deletedBlock, i, getMapT(flashDevice, tempBlock, i));
				setMapT(flashDevice, tempBlock, i, tempAllocData);
			}						
		}
		else{
			// kopiere Inhalt zwischen, l�sche deletedBlock und schreibe Inhalt neu
			for(p = 0; p < FL_getPagesPerBlock(); p++){
				for(i = 0; i < FL_getPageDataSize()/LOGICAL_BLOCK_DATASIZE; i++){
					if( segmentStatus(flashDevice, deletedBlock, (FL_getPagesPerBlock()*p)+i ) == assigned){
						readBlockIntern( flashDevice, deletedBlock, p, i, data[(FL_getPagesPerBlock()*p)+i ]);
					}
				}
			}
			cleanBlock(flashDevice, deletedBlock);
			for(i = 0; i < BLOCKSEGMENTS; i++){
				writeIntern(flashDevice, data[i]);
			}
		}
		return;
	}
	//erase operation in cold pool
	if( isElementOfList(flashDevice->coldPool, deletedBlock) == TRUE){
		//Average Recalculation Cold
		recalculationAVG( flashDevice->coldPool );

		//check condition 3
		if(flashDevice->blockArray[deletedBlock].deleteCounter < flashDevice->coldPool->AVG - DELTA ){
			//step 1 kopiere valide Segmente in tempBlock			
			for(p = 0; p < FL_getPagesPerBlock(); p++){
				for(i = 0; i < FL_getPageDataSize()/LOGICAL_BLOCK_DATASIZE; i++){
					if( segmentStatus(flashDevice, deletedBlock, (FL_getPagesPerBlock()*p)+i ) == assigned){
						readBlockIntern( flashDevice, deletedBlock, p, i, data[(FL_getPagesPerBlock()*p)+i ]);
					}
				}
			}
			//step 2 l�sche Block und setze Z�hler hoch
			cleanBlock(flashDevice, deletedBlock);
			//step 3 kopiere alle validen Segmente aus Max(neutralPool) in deletedBlock
			for(p = 0; p < FL_getPagesPerBlock(); p++){
				for(i = 0; i < FL_getPageDataSize()/LOGICAL_BLOCK_DATASIZE; i++){
					if( segmentStatus(flashDevice, deletedBlock, (FL_getPagesPerBlock()*p)+i ) == assigned){
						readBlockIntern( flashDevice, showLastBlock(flashDevice->neutralPool),p, i, tempSegment);
						writeSegmentToBlock(flashDevice, tempSegment, deletedBlock);
					}
				}
			}
			//step 4 kopiere aus tempBlock(data) in Min(neutralPool)
			position = flashDevice->neutralPool->first;
			for(i = 0; i < BLOCKSEGMENTS; i++){
				p = writeSegmentToBlock(flashDevice, data[i], position->blockNr);
				// falls das Segment nicht geschrieben wurde
				if(p == FALSE){
					position = getNextElement(position);
					if(position == NULL){
						printf("Fehler im WearLevelingAlgorithmus in step 4. Kein beschreibbarer Block mehr im neutralPool vorhanden\n");
					}
				}
			}
			//step 5 update der Adressen; deletedBlock <-> showLastBlock(flashDevice->neutralPool)
			tempBlock = showLastBlock(flashDevice->neutralPool);
			for(i = 0; i < BLOCKSEGMENTS; i++){
				tempAllocData = getMapT(flashDevice, deletedBlock, i);
				setMapT(flashDevice, deletedBlock, i, getMapT(flashDevice, tempBlock, i));
				setMapT(flashDevice, tempBlock, i, tempAllocData);
			}						
		}
		else{
			// kopiere Inhalt zwischen, l�sche deletedBlock und schreibe Inhalt neu
			for(p = 0; p < FL_getPagesPerBlock(); p++){
				for(i = 0; i < FL_getPageDataSize()/LOGICAL_BLOCK_DATASIZE; i++){
					if( segmentStatus(flashDevice, deletedBlock, (FL_getPagesPerBlock()*p)+i ) == assigned){
						readBlockIntern( flashDevice, deletedBlock, p, i, data[(FL_getPagesPerBlock()*p)+i ]);
					}
				}
			}
			cleanBlock(flashDevice, deletedBlock);
			for(i = 0; i < BLOCKSEGMENTS; i++){
				writeIntern(flashDevice, data[i]);
			}
		}
		return;
	}
	
}


// Lokale Funktionsimplementation Allocator
////////////////////////////////////////////////////////////////////
uint32_t getMapT(flash_t *flashDevice, int block, int seg){
	return flashDevice->mappingTable[(block * BLOCKSEGMENTS) + seg];
}

void setMapT(flash_t *flashDevice, int block, int seg, uint32_t v){
	flashDevice->mappingTable[(block * BLOCKSEGMENTS) + seg] = v;
}

// Gibt 512 Speicherplatz representation zur�ck [Blocksegment]
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
	ToDo: Algorihtmus nicht so effizient wie HashMap, aber akutell keine Idee wie ich eine Hashmap f�r uint32_t in C implementiere
	*/
}

StatusPageElem_t segmentStatus(flash_t *flashDevice, uint16_t block, uint16_t segment){	
	if( getMapT(flashDevice, block, segment) != -1){
		return assigned;
	}
	if( flashDevice->blockArray[block].status = badBlock){
		return invalid;
	}
	if( getMapT(flashDevice, block, segment) == -1 && flashDevice->actWriteBlock == block && flashDevice->blockArray[flashDevice->actWriteBlock].writePos < segment){
		return invalid;
	}
	if( flashDevice->blockArray[block].status == ready && flashDevice->blockArray[flashDevice->actWriteBlock].writePos >= segment){
		return empty;
	}
}

void invalidationOfOldIndex(flash_t *flashDevice, uint16_t block, uint16_t segment){ 
	setMapT(flashDevice, block , segment , -1);	
	flashDevice->invalidCounter++;
	flashDevice->blockArray[block].invalidCounter++;	
}

// Lokale Funktionsimplementation  Cleaner
////////////////////////////////////////////////////////////////////
void garbageCollector(flash_t *flashDevice){	
	uint16_t i = flashDevice->actWriteBlock;	
	uint16_t k = 0;
	uint16_t deleteCount = 0;
	uint16_t level = flashDevice->invalidCounter / (FL_getBlockCount() - flashDevice->freeBlocks); //Anzahl der zu bereinigen Blocks, dynamisch berechnet
	

	if (level < 1){
		level = 1;
	}
	while (k < FL_getBlockCount () && deleteCount < (BLOCKSEGMENTS / level) + 1){ // Solange noch nicht alle Bloecke durchlaufen wurden oder genung Bloecke gereinigt wurden
		if (flashDevice->blockArray[i].invalidCounter >= level && flashDevice->blockArray[i].status == used){ // Wenn Block �ber Schwellwert liegt und benutzt wird
			deleteCount++;
			wearLeveling(flashDevice, i);
		}
		k++;
		i++;
	}

}

void cleanBlock(flash_t *flashDevice, uint16_t block){
	int i;
		
	flashDevice->invalidCounter = flashDevice->invalidCounter - flashDevice->blockArray[block].invalidCounter;
	flashDevice->blockArray[block].deleteCounter++; // block l�schz�hler hochsetzten
	flashDevice->blockArray[block].invalidCounter = 0; // Counter zur�ck setzen
	flashDevice->blockArray[block].writePos = 0;
	flashDevice->blockArray[block].status = ready; // Status auf Ready setzen
	flashDevice->freeBlocks++;
	
	//Hardware Block l�schen
	i = FL_deleteBlock(block);
	//TODO BadBlock-Behandlung einbauen
	//Adressen l�schen
	for(i = 0; i < BLOCKSEGMENTS; i++){
		setMapT(flashDevice, block, i, -1);
	}

}

// Lokale Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

uint8_t readBlockIntern(flash_t *flashDevice, uint16_t block, uint16_t page, uint16_t index, uint8_t *data){
	uint16_t count = FL_readData(block, page, index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Blocksegment auslesen
	if (count != LOGICAL_BLOCK_DATASIZE){ // Pr�fen ob wirklich entsprechende Daten gelesen wurden		}
		printf("Fehler beim Lesen des Datensatzes! %d von %d byte gelesen\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}
	return TRUE;
}


void writeIntern(flash_t *flashDevice, uint8_t *data){
	ListElem_t* element;

	while( writeSegmentToBlock(flashDevice, data, flashDevice->actWriteBlock) == false){
		// Start des GarbageCollectors
		if( flashDevice->freeBlocks <= START_CLEANING){
			garbageCollector(flashDevice);
		}
		// flashDevice->aktWriteBlock ist voll
		// nehme evtl. vorhandene Bl�cke aus writePool
		if( listLength(flashDevice->writePool) > 0){
			flashDevice->actWriteBlock = getFirstBlock(flashDevice->writePool);
		}
		else{
			flashDevice->actWriteBlock = -1;
			//nehme k�ltesten, beschreibbaren Block aus coldPool
			element = flashDevice->coldPool->first;
			while( element != NULL ) {
				if( flashDevice->blockArray[element->blockNr].status == ready){
					flashDevice->actWriteBlock = element->blockNr;
					break;
				}
				getNextElement(element);
			}
			//nehme k�ltesten beschreibbaren Block aus neutralPool
			if( flashDevice->actWriteBlock == -1 ){
				element = flashDevice->neutralPool->first;
				while( element != NULL ) {
					if( flashDevice->blockArray[element->blockNr].status == ready){
						flashDevice->actWriteBlock = element->blockNr;
						break;
					}
					getNextElement(element);
				}
			}			
			//nehme k�ltesten, beschreibbaren Block aus hotPool
			if( flashDevice->actWriteBlock == -1 ){
				element = flashDevice->hotPool->first;
				while( element != NULL ) {
					if( flashDevice->blockArray[element->blockNr].status == ready){
						flashDevice->actWriteBlock = element->blockNr;
						break;
					}
					getNextElement(element);
				}
				// Fehlerfall, kein beschreibbarer Block gefunden
				if(flashDevice->actWriteBlock == -1){
					printf("Fehler, es wurde kein beschreibbarer Block gefunden!\n");
				}
			}			
		}
		//Z�hle freie Bl�cke runter
		flashDevice->freeBlocks--;
	}	

	// Aktualisiere die Adresse 
	 setMapT(flashDevice, flashDevice->actWriteBlock, flashDevice->blockArray[flashDevice->actWriteBlock].writePos -1, getMapT(flashDevice, flashDevice->actWriteBlock, flashDevice->blockArray[flashDevice->actWriteBlock].writePos -1));
}

// Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

flash_t * mount(flashMem_t *flashHardware){
	flash_t* flashDevice;
	uint16_t i;

		// Initialisieren		
	flashDevice = (flash_t*)malloc(sizeof(flash_t));
	for (i = 0; i < FL_getBlockCount(); i++){			
		flashDevice->blockArray[i].invalidCounter = 0;
		flashDevice->blockArray[i].deleteCounter = 0;
		flashDevice->blockArray[i].writePos = 0;
		flashDevice->blockArray[i].status = ready;
	}
	// setze Adressen auf -1
	for (i = 0; i < MAPPING_TABLE_SIZE; i++){
		flashDevice->mappingTable[i] = -1;
	}
	// setze als ersten zu beschreibenden Block den Block 0, da alle noch gleich sind
	flashDevice->actWriteBlock = 0;
	flashDevice->invalidCounter = 0;
	flashDevice->freeBlocks = FL_getBlockCount();
	
	// Initialisiere Pools
	flashDevice->hotPool = initList(flashDevice->blockArray);
	flashDevice->neutralPool = initList(flashDevice->blockArray);
	flashDevice->coldPool = initList(flashDevice->blockArray);
	// Initialisiere AVG
	flashDevice->AVG = 0;
	// bef�lle neutralPool
	for(i = 0; i < FL_getBlockCount(); i++){
		addBlock(flashDevice->neutralPool, i);
	}

	printf("SSD initialisiert!\n");

	
	return flashDevice;
}

flash_t *unmount(flash_t *flashDevice){
	/*uint16_t sizeArray, sizeBlock;
	if (flashDevice == NULL) return FALSE;
	sizeArray = sizeof(*flashDevice) / 8;
	sizeBlock = ((sizeof(*flashDevice) / 512) +1) ;
	printf("Groesse der geunmounteten Datenstruktur: %i (%i)\n", sizeBlock, sizeArray);
	flashDevice->isNoErr = FL_saveState((uint8_t)sizeBlock, flashDevice);*/
	return flashDevice; 
}

uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	uint16_t i;
	if (flashDevice == NULL) return FALSE;
	i = mapping(flashDevice, index); // Mapping
	 // Blocksegment auslesen
	return readBlockIntern(flashDevice, i / BLOCKSEGMENTS, (i % BLOCKSEGMENTS) / FL_getPagesPerBlock (), ((i % BLOCKSEGMENTS) % FL_getPagesPerBlock ()), data);

}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	int i;	

	if (flashDevice == NULL) return FALSE;
	
	// Abfrage, ob index zu hoch ist, d.h. nichts mehr auf Platte pa�t
	if(index >= (FL_getBlockCount() - SPARE_BLOCKS) * BLOCKSEGMENTS){
		return FALSE;
	}

	// Abfrage, ob Daten �berschrieben werden sollen
	i = mapping(flashDevice, index);
	if(i != -1){		
		//Invalidiere
		invalidationOfOldIndex(flashDevice, (uint16_t)i / BLOCKSEGMENTS, (uint16_t)i % BLOCKSEGMENTS);
	}

	// Schreiben des Segments und aktualisiere Adresse
	writeIntern(flashDevice, data);
	
	return TRUE;
}

// DEBUG Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

void printerr(flash_t *flashDevice){
	uint16_t i, j, invCo = 0, del = 0, block, segment, calcLevel = 0;
	char marker, error,	userInput, unMountErr = 'n';
	if (flashDevice == NULL) return;
	block = flashDevice->actWriteBlock;
	segment = flashDevice->blockArray[flashDevice->actWriteBlock].writePos;
	printf("\nFehleranalyse mit 'j'\n");
	scanf_s("%c", &userInput);
	getchar();
	if (userInput == 'j')
	{
		if ((FL_getBlockCount() - flashDevice->freeBlocks) > 0){
			calcLevel = flashDevice->invalidCounter / (FL_getBlockCount() - flashDevice->freeBlocks);
		}
		printf("Freie Blocks: %i\nInvalide Segmente: %i (Schwellwert %i)\nAktuelle Schreibposition B:%i / S:%i\nUnmount Fehler: %c \n\n"

			, flashDevice->freeBlocks, flashDevice->invalidCounter, calcLevel, block, segment, unMountErr);
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
					if (flashDevice->blockArray[flashDevice->actWriteBlock].writePos == j){ marker = '+'; }
					if (block == i && segment == j){ marker = 'S'; }
					if (segmentStatus(flashDevice, i, j) != assigned && getMapT(flashDevice, i, j) != 0){ error = '!'; }
					if (segmentStatus(flashDevice, i, j) == assigned && getMapT(flashDevice, i, j) == 0){ error = '!'; }
					if (segmentStatus(flashDevice, i, j) > 2){ error = '?'; }
					printf("Segment %02i: Table: %03i - %i %c %c\n", j, getMapT(flashDevice, i, j), segmentStatus(flashDevice, i, j), error, marker);
				}
				printf("Segment: empty (0) / assigned (1) / invalid (2)\n");
				getchar();
			}
		}
	}
}
