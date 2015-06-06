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
uint32_t mapping(flash_t *flashDevice, uint32_t index);

/*
 * Gibt Auskunft �ber den Status eines Elements
 */
StatusPageElem_t segmentStatus(flash_t *flashDevice, uint16_t block, uint16_t segment);

/* 
 * L�scht einen Eintrag in der Map und invalidiert das dazugeh�rige Segment
 */
void invalidationOfOldIndex(flash_t *flashDevice, uint32_t block, uint32_t segment);

// Funktionen WearLeveling
////////////////////////////////////////////////////////////////////

/*
 *	Schreibt ein Datensegment in einen Block
 *	Der R�ckgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t writeSegmentToBlock(flash_t* flashDevice, uint8_t* segment, uint32_t block);

void grouping(flash_t* flashDevice);

void neutralisation(flash_t* flashDevice, List_t* pool, uint32_t deletedBlock, uint8_t hotNeutralisation);

void deleteBlock(flash_t* flashDevice, uint32_t deletedBlock, uint16_t inPool);

uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data);

/*
 *	WearLeveling Algorithmus nach [TC11]
 *	�bergabeparameter ist eine Instanz von flash_t und der gerade gel�schte Block
 */
void wearLeveling(flash_t* flashDevice, uint32_t deletedBlock);

// Funktionen Garbage Collector
////////////////////////////////////////////////////////////////////
/*
 * Zusammenfassung der geschriebenen Daten (Bereinigung von invalidierten Segmenten), L�schen kompletter Bl�cke. 
 */
void garbageCollector(flash_t *flashDevice);

void cleanBlock(flash_t *flashDevice, uint32_t block);

// Funktionen FTL lokal
////////////////////////////////////////////////////////////////////
/*
 *	Gibt einen neuen beschreibbaren Block zur�ck(mit status == ready)
 */
uint32_t nextBlock(flash_t *flashDevice);

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
uint8_t readBlockIntern(flash_t *flashDevice, uint32_t block, uint32_t page, uint32_t index, uint8_t *data);

//uint8_t moveBlockIntern(flash_t *flashDevice, uint32_t block, uint16_t mappingBlockSource, uint16_t mappingSegmentSource);
// Funktionsimplementation Wear-Leveler ([TC11]- Algorithmus)
////////////////////////////////////////////////////////////////////

uint8_t writeSegmentToBlock(flash_t* flashDevice, uint8_t* segment, uint32_t block){
	uint16_t count = 0;
	uint32_t page;
	uint32_t bp_index;

	if(block == -1){
		return FALSE;
	}

	//Abfrage, ob in den zu schreibender Block beschreibbar ist
	if (flashDevice->blockArray[block].status != ready){
		return FALSE;
	}
	
	page = flashDevice->blockArray[block].writePos / FL_getPagesPerBlock();
	bp_index = flashDevice->blockArray[block].writePos % FL_getPagesPerBlock();

	count = FL_writeData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, segment);
	// Pr�fen ob wirklich entsprechende Daten geschrieben wurden	
	if (count != LOGICAL_BLOCK_DATASIZE){
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}

	flashDevice->blockArray[block].writePos++;		
	// Block ist komplett gef�llt	
	if (flashDevice->blockArray[block].writePos == BLOCKSEGMENTS ){
		flashDevice->blockArray[block].writePos = BLOCKSEGMENTS -1;
		flashDevice->blockArray[block].status = used;
		flashDevice->freeBlocks--;
	}
	return TRUE;
}

void neutralisation(flash_t* flashDevice, List_t* pool, uint32_t deletedBlock, uint8_t hotNeutralisation){
	uint16_t p, i, k;
	uint8_t data[BLOCKSEGMENTS][LOGICAL_BLOCK_DATASIZE];
	uint8_t data2[LOGICAL_BLOCK_DATASIZE];
	uint32_t mappingData[BLOCKSEGMENTS];
	uint16_t data_position = 0;
	uint8_t tempSegment[LOGICAL_BLOCK_DATASIZE];	
	uint32_t tempAddress = 0;
	uint32_t tempBlock = 0;
	uint32_t blockNr = MAPPING_TABLE_SIZE;

		//step 1 kopiere valide Segmente in tempBlock			
			for (p = 0; p < BLOCKSEGMENTS; p++){				
					if (segmentStatus(flashDevice, deletedBlock, p) == assigned){
						mappingData[data_position] = getMapT(flashDevice, deletedBlock, p);
						readBlockIntern(flashDevice, deletedBlock, p / FL_getPagesPerBlock(), (p % FL_getPagesPerBlock()), data2);
						invalidationOfOldIndex(flashDevice, deletedBlock, p);
						for(i = 0; i < LOGICAL_BLOCK_DATASIZE; i++){
							data[data_position][i] = data2[i];
						}
						data_position++;
					}
			}
			//step 2 l�sche Block und setze Z�hler hoch
			cleanBlock(flashDevice, deletedBlock);			
			//step 3 kopiere alle validen Segmente aus Min(neutralPool)/Max(neutralPool) in deletedBlock			
			// und step 5 update der Adressen
			//TODO Fehlerfall, es sind keine Elemente mehr im neutralPool
			if(hotNeutralisation == TRUE){							
				blockNr = showFirstElement(flashDevice->neutralPool)->blockNr;
			}
			else{
				blockNr = showLastElement(flashDevice->neutralPool)->blockNr;
			}
			for (p = 0; p < BLOCKSEGMENTS; p++){				
					if (segmentStatus(flashDevice, deletedBlock, p) == assigned){						
						tempAddress = getMapT(flashDevice, blockNr, p);
						readBlockIntern(flashDevice, blockNr, p / FL_getPagesPerBlock(), (p % FL_getPagesPerBlock()), data2);
						invalidationOfOldIndex(flashDevice, blockNr, p);
						writeSegmentToBlock(flashDevice, data2, deletedBlock);						
						//Adressupdate																				
						setMapT(flashDevice, deletedBlock, p, tempAddress);
					}
			}		

			//step 4 kopiere aus tempBlock(data) in MAX(neutralPool)/Min(neutralPool)
			//falls dieser nicht gen�gend Kapazit�t hat, l�sche den anderen und kopiere da den Rest rein
			// und step 5 update der Adressen
			if(hotNeutralisation == TRUE){
				blockNr = showLastElement(flashDevice->neutralPool)->blockNr;
			}
			else{
				blockNr = showFirstElement(flashDevice->neutralPool)->blockNr;
			}			
			for (i = 0; i < data_position; i++){		
									
					k = flashDevice->blockArray[blockNr].writePos;
					p = writeSegmentToBlock(flashDevice, data[i], blockNr);
					// falls das Segment nicht geschrieben wurde
					if (p == FALSE){												
						if(hotNeutralisation == TRUE){							
							blockNr = showFirstElement(flashDevice->neutralPool)->blockNr;
						}
						else{							
							blockNr = showLastElement(flashDevice->neutralPool)->blockNr;
						}						
						cleanBlock(flashDevice, blockNr);
						delBlock(flashDevice->neutralPool, blockNr);
						addBlock(flashDevice->neutralPool, blockNr);
						k = flashDevice->blockArray[blockNr].writePos;
						p = writeSegmentToBlock(flashDevice, data[i], blockNr);						
					}
					//Adressupdate								
					setMapT(flashDevice, blockNr, k, mappingData[i]);					
			}			
			// Berechen AVGs neu nach Neutralisation
			//TODO nachfragen
			//pool->AVG += (double) (EC(flashDevice->neutralPool, tempBlock) - EC(pool, deletedBlock)) / pool->blockCounter;
			//flashDevice->AVG += (double) (EC(pool, deletedBlock) - EC(flashDevice->neutralPool, tempBlock)) / FL_getBlockCount();
			
			//Update des Blocks in Pool			
			delBlock(pool, deletedBlock);						
			addBlock(pool, deletedBlock);				
}

void checkFreeBlocks(flash_t* flashDevice){
	uint32_t i, counter = 0;

	for(i = 0; i < FL_getBlockCount(); i++){
		if(flashDevice->blockArray[i].status == ready){
			counter++;
		}
	}
	flashDevice->freeBlocks = counter;
}

void deleteBlock(flash_t* flashDevice, uint32_t deletedBlock, uint16_t inPool){
	uint16_t p, i;
	uint8_t data[BLOCKSEGMENTS][LOGICAL_BLOCK_DATASIZE];
	uint8_t data2[LOGICAL_BLOCK_DATASIZE];
	uint32_t mappingData[BLOCKSEGMENTS];
	uint16_t data_position = 0;

	//L�sche Block
	// kopiere Inhalt zwischen, l�sche deletedBlock und schreibe Inhalt neu
			for (p = 0; p < BLOCKSEGMENTS; p++){								
				if(segmentStatus(flashDevice, deletedBlock, p) == assigned){
					mappingData[data_position] = getMapT(flashDevice, deletedBlock, p);									
					readBlockIntern(flashDevice, deletedBlock, p / FL_getPagesPerBlock(), (p % FL_getPagesPerBlock()), data2);										
					invalidationOfOldIndex(flashDevice, deletedBlock, p);
					for(i = 0; i < LOGICAL_BLOCK_DATASIZE; i++){
						data[data_position][i] = data2[i];
					}					
					data_position++;
				}				
			}
			cleanBlock(flashDevice, deletedBlock);
			//Update des Blocks in Pool
			if(inPool == 1){									
				delBlock(flashDevice->neutralPool, deletedBlock);				
				addBlock(flashDevice->neutralPool, deletedBlock);
			}
			if(inPool == 2){
				delBlock(flashDevice->hotPool, deletedBlock);				
				addBlock(flashDevice->hotPool, deletedBlock);
			}
			if(inPool == 3){
				delBlock(flashDevice->coldPool, deletedBlock);				
				addBlock(flashDevice->coldPool, deletedBlock);				
			}
		
			//schreibe Daten zur�ck 
			for (i = 0; i < data_position; i++){
				setMapT(flashDevice, deletedBlock, flashDevice->blockArray[deletedBlock].writePos, mappingData[i]);
				writeSegmentToBlock(flashDevice, data[i], deletedBlock);
			}				
}

void grouping(flash_t* flashDevice){
	ListElem_t* element = NULL;
	uint32_t blockNr;

	//�berpr�fung der Condition 1 und 2	
	//beide Seiten im neutralPool
	element = showLastElement(flashDevice->neutralPool);
	while(element != NULL && EC(flashDevice->neutralPool, element->blockNr) > flashDevice->AVG + THETA) {
		blockNr = element->blockNr;
		delBlock(flashDevice->neutralPool, blockNr);
		addBlock(flashDevice->hotPool, blockNr);
		element = showLastElement(flashDevice->neutralPool);
	}	
	element = showFirstElement(flashDevice->neutralPool);
	while(element != NULL && EC(flashDevice->neutralPool, element->blockNr) < flashDevice->AVG - THETA) {
		blockNr = element->blockNr;
		delBlock(flashDevice->neutralPool, blockNr);
		addBlock(flashDevice->coldPool, blockNr);
		element = showFirstElement(flashDevice->neutralPool);
	}
}

void wearLeveling(flash_t* flashDevice, uint32_t deletedBlock){		
	
	//Average Recalculation gesamt
	flashDevice->AVG += (double)1 / FL_getBlockCount();

	//erase operation in neutral pool?	
	if (isElementOfList(flashDevice->neutralPool, deletedBlock) == TRUE){
		
		//Average Recalculation Neutral
		recalculationAVG(flashDevice->neutralPool);		
	
		//l�sche deletedBlock		
		deleteBlock(flashDevice, deletedBlock, 1);
				
		//Grouping f�r alle Bl�cke in neutralPool
		grouping(flashDevice);
		
	}
	//erase operation in hot pool
	if (isElementOfList(flashDevice->hotPool, deletedBlock) == TRUE){
						
		//Average Recalculation Hot
		recalculationAVG(flashDevice->hotPool);
		
		//check condition 3
		if (flashDevice->blockArray[deletedBlock].deleteCounter > flashDevice->hotPool->AVG + DELTA){
					
			//Neutralisation
			neutralisation(flashDevice, flashDevice->hotPool, deletedBlock, TRUE);		
			return;
		}
		
		//l�sche deletedBlock		
		deleteBlock(flashDevice, deletedBlock, 2);
	}
	//erase operation in cold pool
	if (isElementOfList(flashDevice->coldPool, deletedBlock) == TRUE){
				
		//Average Recalculation Cold
		recalculationAVG(flashDevice->coldPool);
		
		//check condition 3
		if (flashDevice->blockArray[deletedBlock].deleteCounter < flashDevice->coldPool->AVG - DELTA){
			//Neutralisation
			neutralisation(flashDevice, flashDevice->coldPool, deletedBlock, FALSE);		
			return;			
		}
		
		//l�sche deletedBlock		
		deleteBlock(flashDevice, deletedBlock, 3);
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
uint32_t mapping(flash_t *flashDevice, uint32_t index){
	uint32_t target;
	uint32_t i = 0;
	uint32_t x = MAPPING_TABLE_SIZE;
	for (i = 0; i < x; i++){
		target = getMapT(flashDevice, i / BLOCKSEGMENTS, i % BLOCKSEGMENTS);
		if (target == index){ return i; }
	}
	return x;
	/*
	ToDo: Algorihtmus nicht so effizient wie HashMap, �ndern!
	*/
}

StatusPageElem_t segmentStatus(flash_t *flashDevice, uint16_t block, uint16_t segment){
	if ((flashDevice->blockArray[block].status == ready  && getMapT(flashDevice, block, segment) == 0)){ // Kompletter Block muss leer sein
		if (flashDevice->actWriteBlock == block && flashDevice->blockArray[flashDevice->actWriteBlock].writePos > segment){
			return invalid;
		}
		return empty;
		// Kleine Grauzone, Direkt als ung�ltig markierte Datens�tze werden auch als empty angezeigt, es darf aber sowieso nicht zur�ck gesprungen werden ???
	}
	else if ((flashDevice->blockArray[block].status == used || flashDevice->blockArray[block].status == ready) && getMapT(flashDevice, block, segment) > 0){ // Wenn Map Verweis, dann belegt
		return assigned;
	}
	else if (flashDevice->blockArray[block].status == used  && getMapT(flashDevice, block, segment) == 0){ // Wenn kein Map Verweis
		return invalid;
	}
	else { // Bad Block 
		return invalid;
	}
	//empty, assigned, invalid
}

void invalidationOfOldIndex(flash_t *flashDevice, uint32_t block, uint32_t segment){
	setMapT(flashDevice, block , segment , 0);	
	flashDevice->invalidCounter++;
	flashDevice->blockArray[block].invalidCounter++;	
}

// Lokale Funktionsimplementation  Cleaner
////////////////////////////////////////////////////////////////////
void garbageCollector(flash_t *flashDevice){	
	uint16_t i = flashDevice->actWriteBlock;
	uint32_t k = 0;		
	uint32_t level = flashDevice->invalidCounter / (FL_getBlockCount()); //Anzahl der zu bereinigen Blocks, dynamisch berechnet
	
	// Verhindern dass Bl�cke ohne invalide eintr�ge gecleant werden
	if (level < 1){
		level = 1;
	}	
		
	// Solange noch nicht alle Bloecke durchlaufen wurden oder genug Bloecke gereinigt wurden				
	while (flashDevice->freeBlocks < SPARE_BLOCKS  && k < FL_getBlockCount () ){ 		
		if( i >= FL_getBlockCount() - 1){
			i = 0;
		}
		else{
			i++;
		}
				
		// Wenn Block �ber Schwellwert liegt und benutzt wird				
		if ( flashDevice->blockArray[i].invalidCounter >= level && flashDevice->blockArray[i].status == used){ 					
			wearLeveling(flashDevice, i) ;	
		}
		k++;		
	}
	//�berpr�fe Anzahl freeBlocks
	checkFreeBlocks(flashDevice);

	if (k == FL_getBlockCount() && flashDevice->freeBlocks < SPARE_BLOCKS){
		printf("Fehler beim Cleanen, nicht genug Bl�cke befreit!\n");
		printerr(flashDevice);
	}	
}

void cleanBlock(flash_t *flashDevice, uint32_t block){
	uint16_t i = 0;
	//Hardware Block l�schen
	if (FL_deleteBlock(block) == TRUE){
		//Adressen l�schen
		flashDevice->invalidCounter = flashDevice->invalidCounter - flashDevice->blockArray[block].invalidCounter;
		flashDevice->blockArray[block].deleteCounter++; // block l�schz�hler hochsetzten
		flashDevice->blockArray[block].invalidCounter = 0; // Counter zur�ck setzen
		flashDevice->blockArray[block].writePos = 0;
		flashDevice->blockArray[block].status = ready; // Status auf Ready setzen
		flashDevice->freeBlocks++;
	}
	else {		
		flashDevice->blockArray[block].status = badBlock; // Status auf BadBlock setzen
	}

}

// Lokale Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////


uint8_t readBlockIntern(flash_t *flashDevice, uint32_t block, uint32_t page, uint32_t index, uint8_t *data){
	uint16_t count = FL_readData(block, page, index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Blocksegment auslesen
	if (count != LOGICAL_BLOCK_DATASIZE){ // Pr�fen ob wirklich entsprechende Daten gelesen wurden		}
		printf("Fehler beim Lesen des Datensatzes! %d von %d byte gelesen\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}
	return TRUE;
}

uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data){

	uint32_t block = flashDevice->actWriteBlock;
	uint32_t page = flashDevice->blockArray[flashDevice->actWriteBlock].writePos / FL_getPagesPerBlock();
	uint32_t bp_index = flashDevice->blockArray[flashDevice->actWriteBlock].writePos % FL_getPagesPerBlock();
	uint32_t count;
	uint32_t index_old;
	uint32_t value = MAPPING_TABLE_SIZE;
	// Der Block auf dem geschrieben wird, ist nicht beschreibbar
	if (flashDevice->blockArray[block].status != ready){ 
		/*
			printf("Fehler beim Schreiben des Datensatzes! Fehlerhafter Block Zugriff!\n");			
			printerr(flashDevice);
			return FALSE;*/
		block = nextBlock(flashDevice);		
	}	
	// Daten beschreiben
	count = FL_writeData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data);
	if (count != LOGICAL_BLOCK_DATASIZE){ // Pr�fen ob wirklich entsprechende Daten geschrieben wurden		
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);		
		printerr(flashDevice);
		return FALSE;
	}
	//Invalidiere alten Index
	index_old =  mapping(flashDevice, index);
	if( index_old != MAPPING_TABLE_SIZE){
		// Alten Eintrag in Mapping Table und Block invalidieren		
		invalidationOfOldIndex(flashDevice, (uint16_t)index_old / BLOCKSEGMENTS, (uint16_t)index_old % BLOCKSEGMENTS); 
	}
	//printf("Index! %i (%i/%i/%i) \n" , index, block, page, bp_index);//TODO DEBUG-Ausgabe rausnehmen
	// Setze Mapeintrag
	setMapT(flashDevice, block, (page * FL_getPagesPerBlock()) + bp_index, index);

	// Auswahl des n�chsten Schreibortes
	// position weiterz�hlen wenn innerhalb des selben blockes
	if (flashDevice->blockArray[flashDevice->actWriteBlock].writePos < BLOCKSEGMENTS - 1){ 
		flashDevice->blockArray[flashDevice->actWriteBlock].writePos++;
	}
	// oder einen neuen Block ausw�hlen
	else { 
		// alten Block abschlie�en
		flashDevice->blockArray[flashDevice->actWriteBlock].status = used;
		flashDevice->freeBlocks--;
		// Freien Block finden und nutzen
		value = nextBlock(flashDevice);
		if (value == MAPPING_TABLE_SIZE){
			return FALSE;
		}
		else{
			flashDevice->actWriteBlock = value;
		}
		// Cleaner		
		if (flashDevice->freeBlocks < SPARE_BLOCKS ){
			// Clean
			garbageCollector(flashDevice); 
			flashDevice->actWriteBlock = nextBlock(flashDevice);
		}		
	}
	return TRUE;
}

uint32_t nextBlock(flash_t *flashDevice){
	ListElem_t* element;	
	uint32_t i;
	
	//nehme k�ltesten, beschreibbaren Block aus coldPool			
	for (element = showFirstElement(flashDevice->coldPool); element != NULL; element = getNextElement(element)){
		if (flashDevice->blockArray[element->blockNr].status == ready){
			return element->blockNr;			
		}
	}
	//nehme k�ltesten beschreibbaren Block aus neutralPool
	for (element = showFirstElement(flashDevice->neutralPool); element != NULL; element = getNextElement(element)){		
		if (flashDevice->blockArray[element->blockNr].status == ready){
			return element->blockNr;						
		}
	}
	//nehme k�ltesten, beschreibbaren Block aus hotPool
	for (element = showFirstElement(flashDevice->hotPool); element != NULL; element = getNextElement(element)){
		if (flashDevice->blockArray[element->blockNr].status == ready){
			return element->blockNr;			
		}	
	}
	
	// Fehlerfall, kein beschreibbarer Block gefunden
	printf("Fehler, es wurde kein beschreibbarer Block in einem Pool gefunden!\n");	
	printf("freeBlock %i, aktWriteBlock %i\n",flashDevice->freeBlocks, flashDevice->actWriteBlock);	
	printerr(flashDevice);
	return MAPPING_TABLE_SIZE;
}

// Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

flash_t * mount(flashMem_t *flashHardware){
	flash_t* flashDevice;
	uint32_t i;
	ListElem_t* element = NULL;

	// Initialisieren		
	flashDevice = (flash_t*)malloc(sizeof(flash_t));
	for (i = 0; i < FL_getBlockCount(); i++){			
		flashDevice->blockArray[i].invalidCounter = 0;
		flashDevice->blockArray[i].deleteCounter = 0;
		flashDevice->blockArray[i].writePos = 0;
		flashDevice->blockArray[i].status = ready;
	}
	// setze Adressen auf 0
	for (i = 0; i < MAPPING_TABLE_SIZE; i++){
		flashDevice->mappingTable[i] = 0;
	}
	// setze als ersten zu beschreibenden Block den Block 0, da alle noch gleich sind
	flashDevice->actWriteBlock = 0;
	flashDevice->invalidCounter = 0;	
	flashDevice->freeBlocks = FL_getBlockCount() ;
	
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
		
	/*for(i = 0; i < 50; i++){//TODO herausnehmen, DEBUG-Zwecke List_t
		delBlock(flashDevice->neutralPool, (i % (FL_getBlockCount())) );		
		flashDevice->blockArray[i % (FL_getBlockCount())].deleteCounter = i;
		addBlock(flashDevice->neutralPool, (i % (FL_getBlockCount())) );
		/*
		element = getFirstElement(flashDevice->neutralPool);		
		if(element != NULL){
			printf("%i == ", element->blockNr);
			addBlock(flashDevice->hotPool, element->blockNr);
		}
		

		element = getFirstElement(flashDevice->hotPool);		
		if(element != NULL){			
			printf("%i\n", element->blockNr);
			addBlock(flashDevice->neutralPool, element->blockNr);
		}
		
		free(element)

		//addBlock(flashDevice->hotPool, getLastElement(flashDevice->neutralPool));		
		//addBlock(flashDevice->neutralPool, getLastElement(flashDevice->hotPool));

	}
	printf("neutral\n");
	printList(flashDevice->neutralPool);
	printf("hot\n");
	printList(flashDevice->hotPool);
	scanf_s(&i);
	exit(0);*/

	return flashDevice;
}

flash_t *unmount(flash_t *flashDevice){
	/*uint32_t sizeArray, sizeBlock;
	if (flashDevice == NULL) return FALSE;
	sizeArray = sizeof(*flashDevice) / 8;
	sizeBlock = ((sizeof(*flashDevice) / 512) +1) ;
	printf("Groesse der geunmounteten Datenstruktur: %i (%i)\n", sizeBlock, sizeArray);
	flashDevice->isNoErr = FL_saveState((uint8_t)sizeBlock, flashDevice);*/
	return flashDevice; 
}

uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	uint32_t i;
	if (flashDevice == NULL) return FALSE;
	if(index > (FL_getBlockCount() - SPARE_BLOCKS) * BLOCKSEGMENTS){
		printf("Logischer Block nicht mehr im Wertebereich des Datentraegers. 0 bis %i logische Bloecke adressierbar.\n", LOG_BLOCK_COUNT);
		return FALSE;
	}
	//Index immer um 1 erh�ht, da index == 0 f�r andere Zwecke verwendet wird.
	i = mapping(flashDevice, index + 1); // Mapping
	return readBlockIntern(flashDevice, i / BLOCKSEGMENTS, (i % BLOCKSEGMENTS) / FL_getPagesPerBlock(), ((i % BLOCKSEGMENTS) % FL_getPagesPerBlock()), data); // Blocksegment auslesen

}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	if (flashDevice == NULL) return FALSE;
	if(index > (FL_getBlockCount() - SPARE_BLOCKS) * BLOCKSEGMENTS){
		printf("Logischer Block nicht mehr im Wertebereich des Datentraegers. 0 bis %i logische Bloecke adressierbar.\n", LOG_BLOCK_COUNT);
		return FALSE;
	}
	//Index immer um 1 erh�ht, da index == 0 f�r andere Zwecke verwendet wird.
	return writeBlockIntern(flashDevice, index + 1, data);
}

// DEBUG Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

void printerr(flash_t *flashDevice){
	uint32_t i, j, invCo = 0, del = 0, block, segment, calcLevel = 0;
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
		// Ausgabe der Pools
		printf("AVG: %f\n", flashDevice->AVG);
		printf("ColdPool\n");
		printList(flashDevice->coldPool);
		printf("neutralPool\n");
		printList(flashDevice->neutralPool);
		printf("HotPool\n");
		printList(flashDevice->hotPool);

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
					if (flashDevice->blockArray[i].writePos == j){ 
						marker = '+'; 
					}
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
		printerr(flashDevice);
	}
}
