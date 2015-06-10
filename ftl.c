/* Implementierung eines FTL's */
#include "ftl.h"

// Getter für "Konstanten"
////////////////////////////////////////////////////////////////////
/* 
 *	Gibt die Anzahl der Segmente pro Block zurück
 */
uint32_t getBlockSegmentCount();

/* 
 *	Gibt die Anzahl der Segment  *gesamte Blöcke zurück
 */
uint32_t getMappingTableSize();

/* 
 *	Gibt die Anzahl der adressierbaren logischen Blöcke zurück
 */
uint32_t getLogBlockCount();

// Funktionen Allocator
////////////////////////////////////////////////////////////////////

/* Getter Map 
  *Übersetzt die physikalische Position in die  logischen Nummer eines Datenblocks
  *Gibt die logische Nummer eines Datenblocks zurück
 */
uint32_t getMapT(flash_t *flashDevice, int block, int seg);

/* Setter Map
  *Verknüpft die logischen Nummer eines Datenblocks mit der physikalische Position in der Tabelle
  *Gibt die logische Nummer eines Datenblocks zurück
 */
void setMapT(flash_t *flashDevice, int block, int seg, uint32_t v);

/* mapping
  *Übersetzt die logischen Nummer eines Datenblocks in die physikalische Position 
  *Gibt die physikalische Position eines Datenblocks zurück
 */
uint32_t mapping(flash_t *flashDevice, uint32_t index);

/* 
  *Gibt Auskunft über den Status eines Elements
 */
StatusPageElem_t segmentStatus(flash_t *flashDevice, uint16_t block, uint16_t segment);

/* 
  *Löscht einen Eintrag in der Map und invalidiert das dazugehörige Segment
 */
void invalidationOfOldIndex(flash_t *flashDevice, uint32_t block, uint32_t segment);

// Funktionen WearLeveling
////////////////////////////////////////////////////////////////////

/* 
 *	Schreibt ein Datensegment in einen Block
 *	Der Rückgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t writeSegmentToBlock(flash_t *flashDevice, uint8_t *segment, uint32_t block);

/* 
 *	Grouping nach Algorithmus
 */
void grouping(flash_t *flashDevice);

/* 
 *	Neutralisation nach Algorithmus
 */
void neutralisation(flash_t *flashDevice, List_t *pool, uint32_t deletedBlock, uint8_t hotNeutralisation);

/* 
 *	Löscht den übergebenden Block, und datet ihn im übergebenden Pool up
 */
void deleteBlock(flash_t *flashDevice, uint32_t deletedBlock, uint16_t inPool);

/* 
 *	Interne Schreibfunktion
 */
uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data);

/* 
 *	WearLeveling Algorithmus nach [TC11]
 *	Übergabeparameter ist eine Instanz von flash_t und der gerade gelöschte Block
 */
void wearLeveling(flash_t *flashDevice, uint32_t deletedBlock);

// Funktionen Garbage Collector
////////////////////////////////////////////////////////////////////
/* 
  *Zusammenfassung der geschriebenen Daten (Bereinigung von invalidierten Segmenten), Löschen kompletter Blöcke. 
 */
void garbageCollector(flash_t *flashDevice);

/* 
 *	Löscht den Block auf der Hardware
 */
void cleanBlock(flash_t *flashDevice, uint32_t block);

// Funktionen FTL lokal
////////////////////////////////////////////////////////////////////

/*
*	Wandelt vier uint8_t zu einem uint32_t um
* /
uint32_t convert8To32(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

/*
*	Wandelt einen uint32_t, source, zu vier uint8_t, data[4], um
*/
void convert32To8(uint8_t *data, uint32_t source);

/* 
 *	Gibt einen neuen beschreibbaren Block zurück(mit status == ready)
 */
uint32_t nextBlock(flash_t *flashDevice);

/* 
 *Liest einen Datenblock an der angegebene physikalischen vom Flashspeicher, der mit der
 *in flashDevice übergebenen Datenstruktur verwaltet wird.
 *flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 *dieses Flash-Datenträgers dient.
 *index ist die Nummer des zu lesenden Blocks.
 *data ist ein Pointer auf einen ausreichend großen Datenbereich, in den die zu lesenden
 *Daten kopiert werden.
 *Der Rückgabewert ist als Boolescher Wert zu interpretieren.
*/
uint8_t readBlockIntern(flash_t *flashDevice, uint32_t block, uint32_t page, uint32_t index, uint8_t *data);

// Funktionsimplementation Wear-Leveler ([TC11]- Algorithmus)
////////////////////////////////////////////////////////////////////

uint8_t writeSegmentToBlock(flash_t *flashDevice, uint8_t *segment, uint32_t block){
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

	count = FL_writeData(block, page, bp_index  *LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, segment);
	// Prüfen ob wirklich entsprechende Daten geschrieben wurden	
	if (count != LOGICAL_BLOCK_DATASIZE){
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}

	flashDevice->blockArray[block].writePos++;		
	// Block ist komplett gefüllt	
	if (flashDevice->blockArray[block].writePos == getBlockSegmentCount() ){
		flashDevice->blockArray[block].writePos = getBlockSegmentCount() -1;
		flashDevice->blockArray[block].status = used;
		flashDevice->freeBlocks--;
	}
	return TRUE;
}

void neutralisation(flash_t *flashDevice, List_t *pool, uint32_t deletedBlock, uint8_t hotNeutralisation){
	uint16_t p, i, k;
	uint8_t **data;
	uint8_t data2[LOGICAL_BLOCK_DATASIZE];
	uint32_t *mappingData;
	uint16_t data_position = 0;
	uint8_t tempSegment[LOGICAL_BLOCK_DATASIZE];	
	uint32_t tempAddress = 0;
	uint32_t tempBlock = 0;
	uint32_t blockNr = getMappingTableSize();
	//allokiere Speicher
	data = (uint8_t**)malloc(getBlockSegmentCount()*sizeof(uint8_t*) );
	for(i = 0; i < LOGICAL_BLOCK_DATASIZE; i++){
		data[i] = (uint8_t*)malloc(sizeof(uint8_t));
	}
	mappingData = (uint32_t*) malloc(getBlockSegmentCount()*sizeof(uint32_t));

		//step 1 kopiere valide Segmente in tempBlock			
			for (p = 0; p < getBlockSegmentCount(); p++){				
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
			//step 2 lösche Block und setze Zähler hoch
			cleanBlock(flashDevice, deletedBlock);			
			//step 3 kopiere alle validen Segmente aus Min(neutralPool)/Max(neutralPool) in deletedBlock			
			// und step 5 update der Adressen
			//TODO Fehlerfall, es sind keine Elemente mehr im neutralPool [Übergangslösung überarbeiten!!]
			if (flashDevice->neutralPool->blockCounter < 1){
				if (flashDevice->coldPool->blockCounter < 1){
					blockNr = showFirstElement(flashDevice->hotPool)->blockNr;
					if (delBlock(flashDevice->hotPool, blockNr) == TRUE){
						addBlock(flashDevice->neutralPool, blockNr);
					}
				}
				else {
					blockNr = showLastElement(flashDevice->coldPool)->blockNr;
					if (delBlock(flashDevice->coldPool, blockNr) == TRUE){
						addBlock(flashDevice->neutralPool, blockNr);
					}
				}

			}
			if(hotNeutralisation == TRUE){							
				blockNr = showFirstElement(flashDevice->neutralPool)->blockNr;
			}
			else{
				blockNr = showLastElement(flashDevice->neutralPool)->blockNr;
			}
			for (p = 0; p < getBlockSegmentCount(); p++){				
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
			//falls dieser nicht genügend Kapazität hat, lösche den anderen und kopiere da den Rest rein
			// und step 5 update der Adressen
			if(hotNeutralisation == TRUE){
				blockNr = showLastElement(flashDevice->neutralPool)->blockNr;
			}
			else{
				blockNr = showFirstElement(flashDevice->neutralPool)->blockNr;
			}			
			for (i = 0; i < data_position; i++){
				do {
					k = flashDevice->blockArray[blockNr].writePos;
					p = writeSegmentToBlock(flashDevice, data[i], blockNr);
					// falls das Segment nicht geschrieben wurde
					if (p == FALSE){
						blockNr = nextBlock(flashDevice);
						/*	if(hotNeutralisation == TRUE){
								blockNr = showFirstElement(flashDevice->neutralPool)->blockNr;
								}
								else{
								blockNr = showLastElement(flashDevice->neutralPool)->blockNr;
								}
								cleanBlock(flashDevice, blockNr);
								delBlock(flashDevice->neutralPool, blockNr);
								addBlock(flashDevice->neutralPool, blockNr);
								k = flashDevice->blockArray[blockNr].writePos;
								p = writeSegmentToBlock(flashDevice, data[i], blockNr);	*/
					}
					else {
						//Adressupdate								
						setMapT(flashDevice, blockNr, k, mappingData[i]);
					}
				} while (p == FALSE);
			}
			// Berechen AVGs neu nach Neutralisation
			//TODO nachfragen
			//pool->AVG += (double) (EC(flashDevice->neutralPool, tempBlock) - EC(pool, deletedBlock)) / pool->blockCounter;
			//flashDevice->AVG += (double) (EC(pool, deletedBlock) - EC(flashDevice->neutralPool, tempBlock)) / FL_getBlockCount();
			
			//Update des Blocks in Pool
			if (delBlock(pool, deletedBlock) == TRUE){
				addBlock(pool, deletedBlock);
			}

			//free für allozierte Variablen
		    free(mappingData);
			/*for(i = 0; i < LOGICAL_BLOCK_DATASIZE; i++){
				free(data[i]);
			}// */ //TODO nachfragen wie zu free-en ist
			free(data);
			//printf("Neutralisation beendet\n");
}

void checkFreeBlocks(flash_t *flashDevice){
	uint32_t i, counter = 0;

	for(i = 0; i < FL_getBlockCount(); i++){
		if(flashDevice->blockArray[i].status == ready){
			counter++;
		}
	}
	flashDevice->freeBlocks = counter;
}

void deleteBlock(flash_t *flashDevice, uint32_t deletedBlock, uint16_t inPool){
	uint32_t p, i;
	uint8_t **data;
	uint8_t data2[LOGICAL_BLOCK_DATASIZE];
	uint32_t *mappingData;
	uint32_t data_position = 0;
	//allokiere Speicher
	data = (uint8_t**)malloc(getBlockSegmentCount()*sizeof(uint8_t*) );
	for(i = 0; i < LOGICAL_BLOCK_DATASIZE; i++){
		data[i] = (uint8_t*)malloc(sizeof(uint8_t));
	}
	mappingData = (uint32_t*) malloc(getBlockSegmentCount()*sizeof(uint32_t));

	//Lösche Block
	// kopiere Inhalt zwischen, lösche deletedBlock und schreibe Inhalt neu
			for (p = 0; p < getBlockSegmentCount(); p++){								
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
		
			//schreibe Daten zurück 
			for (i = 0; i < data_position; i++){
				setMapT(flashDevice, deletedBlock, flashDevice->blockArray[deletedBlock].writePos, mappingData[i]);
				writeSegmentToBlock(flashDevice, data[i], deletedBlock);
			}				

			//free für allozierte Variablen
		    free(mappingData);
			/*for(i = getBlockSegmentCount() - 1; i >= 0; i--){					
					free(data[i]);			
			}*/
			free(data);
}

void grouping(flash_t *flashDevice){
	ListElem_t *element = NULL;
	uint32_t blockNr;

	//Überprüfung der Condition 1 und 2	
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

void wearLeveling(flash_t *flashDevice, uint32_t deletedBlock){		
	
	//Average Recalculation gesamt
	flashDevice->AVG += (double)1 / FL_getBlockCount();

	//erase operation in neutral pool?	
	if (isElementOfList(flashDevice->neutralPool, deletedBlock) == TRUE){
		
		//Average Recalculation Neutral
		recalculationAVG(flashDevice->neutralPool);		
	
		//lösche deletedBlock		
		deleteBlock(flashDevice, deletedBlock, 1);
				
		//Grouping für alle Blöcke in neutralPool
		grouping(flashDevice);
		
	}
	//erase operation in hot pool
	if (isElementOfList(flashDevice->hotPool, deletedBlock) == TRUE){
						
		//Average Recalculation Hot
		recalculationAVG(flashDevice->hotPool);
		
		//check condition 3
		if (flashDevice->blockArray[deletedBlock].deleteCounter > flashDevice->hotPool->AVG + DELTA){
					
			//Neutralisation
			//printf("\nwarme neutralisation \n");
			neutralisation(flashDevice, flashDevice->hotPool, deletedBlock, TRUE);		
			return;
		}
		
		//lösche deletedBlock		
		deleteBlock(flashDevice, deletedBlock, 2);
	}
	//erase operation in cold pool
	if (isElementOfList(flashDevice->coldPool, deletedBlock) == TRUE){
				
		//Average Recalculation Cold
		recalculationAVG(flashDevice->coldPool);
		
		//check condition 3
		if (flashDevice->blockArray[deletedBlock].deleteCounter < flashDevice->coldPool->AVG - DELTA){
			//Neutralisation
			//printf("\nkalte neutralisation Block: %i \n", deletedBlock);
			neutralisation(flashDevice, flashDevice->coldPool, deletedBlock, FALSE);		
			return;			
		}
		
		//lösche deletedBlock		
		deleteBlock(flashDevice, deletedBlock, 3);
	}	

}

// Lokale Funktionsimplementation Allocator
////////////////////////////////////////////////////////////////////
uint32_t getMapT(flash_t *flashDevice, int block, int seg){
	return flashDevice->mappingTable[(block  *getBlockSegmentCount()) + seg];
}

void setMapT(flash_t *flashDevice, int block, int seg, uint32_t v){
	// Sonderfall Index auf Null setzten (invalidieren)
	if (v == getMappingTableSize()){
		flashDevice->mappingTableRev[getMapT(flashDevice, block, seg)] = getMappingTableSize();
	}
	else {
		// Index updaten
		flashDevice->mappingTableRev[v] = (block  *getBlockSegmentCount()) + seg;
	}
	flashDevice->mappingTable[(block  *getBlockSegmentCount()) + seg] = v;
}

// Gibt 512 Speicherplatz representation zurück [Blocksegment]
uint32_t mapping(flash_t *flashDevice, uint32_t index){
	/*uint32_t target;
	uint32_t i = 0;
	uint32_t x = getMappingTableSize();

	for (i = 0; i < x; i++){
		target = getMapT(flashDevice, i / getBlockSegmentCount(), i % getBlockSegmentCount());
		if (target == index){ return i; }
	}
	return x;*/
	if (index == 0) return getMappingTableSize();
	return flashDevice->mappingTableRev[index];
}

StatusPageElem_t segmentStatus(flash_t *flashDevice, uint16_t block, uint16_t segment){
	if ((flashDevice->blockArray[block].status == ready  && getMapT(flashDevice, block, segment) == 0)){ // Kompletter Block muss leer sein
		if (flashDevice->actWriteBlock == block && flashDevice->blockArray[flashDevice->actWriteBlock].writePos > segment){
			return invalid;
		}
		return empty;
		// Kleine Grauzone, Direkt als ungültig markierte Datensätze werden auch als empty angezeigt, es darf aber sowieso nicht zurück gesprungen werden ???
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
	
	// Verhindern dass Blöcke ohne invalide einträge gecleant werden
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
				
		// Wenn Block über Schwellwert liegt und benutzt wird				
		if ( flashDevice->blockArray[i].invalidCounter >= level && flashDevice->blockArray[i].status == used){ 					
			wearLeveling(flashDevice, i) ;	
		}
		k++;		
	}
	//Überprüfe Anzahl freeBlocks
	checkFreeBlocks(flashDevice);

	/*if (k == FL_getBlockCount() && flashDevice->freeBlocks < SPARE_BLOCKS){
		printf("Fehler beim Aufraeumen, nicht genug Blöcke befreit!\n");
		printerr(flashDevice);
	}	*/
}

void cleanBlock(flash_t *flashDevice, uint32_t block){
	uint16_t i = 0;
	//Hardware Block löschen
	if (FL_deleteBlock(block) == TRUE){
		//Adressen löschen
		flashDevice->invalidCounter = flashDevice->invalidCounter - flashDevice->blockArray[block].invalidCounter;
		flashDevice->blockArray[block].deleteCounter++; // block löschzähler hochsetzten
		flashDevice->blockArray[block].invalidCounter = 0; // Counter zurück setzen
		flashDevice->blockArray[block].writePos = 0;
		flashDevice->blockArray[block].status = ready; // Status auf Ready setzen
		flashDevice->freeBlocks++;
	}
	else {
		flashDevice->blockArray[block].status = badBlock; // Status auf BadBlock setzen
		printerr(flashDevice);
	}

}

// Lokale Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////


uint8_t readBlockIntern(flash_t *flashDevice, uint32_t block, uint32_t page, uint32_t index, uint8_t *data){
	uint16_t count = FL_readData(block, page, index  *LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Blocksegment auslesen
	if (count != LOGICAL_BLOCK_DATASIZE){ // Prüfen ob wirklich entsprechende Daten gelesen wurden		}
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
	uint32_t value = getMappingTableSize();
	// Der Block auf dem geschrieben wird, ist nicht beschreibbar
	if (flashDevice->blockArray[block].status != ready){ 
		/* 
			printf("Fehler beim Schreiben des Datensatzes! Fehlerhafter Block Zugriff!\n");			
			printerr(flashDevice);
			return FALSE;*/
		block = nextBlock(flashDevice);		
	}	
	// Daten beschreiben
	count = FL_writeData(block, page, bp_index  *LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data);
	if (count != LOGICAL_BLOCK_DATASIZE){ // Prüfen ob wirklich entsprechende Daten geschrieben wurden		
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);		
		printerr(flashDevice);
		return FALSE;
	}
	//Invalidiere alten Index
	index_old =  mapping(flashDevice, index);
	if( index_old != getMappingTableSize()){
		// Alten Eintrag in Mapping Table und Block invalidieren		
		invalidationOfOldIndex(flashDevice, (uint16_t)index_old / getBlockSegmentCount(), (uint16_t)index_old % getBlockSegmentCount()); 
	}
	//printf("Index! %i (%i/%i/%i) \n" , index, block, page, bp_index);//TODO DEBUG-Ausgabe rausnehmen
	// Setze Mapeintrag
	setMapT(flashDevice, block, (page  *FL_getPagesPerBlock()) + bp_index, index);

	// Auswahl des nächsten Schreibortes
	// position weiterzählen wenn innerhalb des selben blockes
	if (flashDevice->blockArray[flashDevice->actWriteBlock].writePos < getBlockSegmentCount() - 1){ 
		flashDevice->blockArray[flashDevice->actWriteBlock].writePos++;
	}
	// oder einen neuen Block auswählen
	else { 
		// alten Block abschließen
		flashDevice->blockArray[flashDevice->actWriteBlock].status = used;
		flashDevice->freeBlocks--;
		// Freien Block finden und nutzen
		value = nextBlock(flashDevice);
		if (value == getMappingTableSize()){
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
	ListElem_t *element;	
	uint32_t i;
	
	//nehme kältesten, beschreibbaren Block aus coldPool			
	for (element = showFirstElement(flashDevice->coldPool); element != NULL; element = getNextElement(element)){
		if (flashDevice->blockArray[element->blockNr].status == ready){
			return element->blockNr;			
		}
	}
	//nehme kältesten beschreibbaren Block aus neutralPool
	for (element = showFirstElement(flashDevice->neutralPool); element != NULL; element = getNextElement(element)){		
		if (flashDevice->blockArray[element->blockNr].status == ready){
			return element->blockNr;						
		}
	}
	//nehme kältesten, beschreibbaren Block aus hotPool
	for (element = showFirstElement(flashDevice->hotPool); element != NULL; element = getNextElement(element)){
		if (flashDevice->blockArray[element->blockNr].status == ready){
			return element->blockNr;			
		}	
	}
	
	// Fehlerfall, kein beschreibbarer Block gefunden
	garbageCollector(flashDevice);
	printf("NextBlock Rekursionsfehler!");
	return nextBlock(flashDevice);	
}

// Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////


flash_t  *mount(flashMem_t *flashHardware){
	flash_t *flashDevice;
	uint32_t i, temp, k, p;
	ListElem_t *element = NULL;
	uint8_t *state;
	// uint32_t pos = 0;
	uint8_t data[4];
	uint32_t blockCount = 0;
	uint32_t mappingTableCount = 0;
	uint32_t size = 0;
	uint32_t charCounter = 0;

	// Laden von flash_t	
	state = (uint8_t*)malloc(SAVE_STATE_SIZE* sizeof(uint8_t));
	flashDevice = (flash_t*)malloc(sizeof(flash_t));

	if (FL_restoreState(state) == state){

		//laden
		i = 0;
		size = convert8To32(state[i++], state[i++], state[i++], state[i++]);		
		mappingTableCount = convert8To32(state[i++], state[i++], state[i++], state[i++]);
		flashDevice->mappingTable = (uint32_t*)malloc(mappingTableCount*sizeof(uint32_t));
		flashDevice->mappingTableRev = (uint32_t*)malloc((getMappingTableSize() + 1)*sizeof(uint32_t));
		for (k = 0; k < mappingTableCount; k++){
			temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
			flashDevice->mappingTable[k] = temp;
		}

		for (k = 0; k < getMappingTableSize(); k++){
			flashDevice->mappingTableRev[k] = getMapT(flashDevice, k / FL_getBlockCount(), k % FL_getBlockCount());
		}
		
		blockCount = convert8To32(state[i++], state[i++], state[i++], state[i++]);
		flashDevice->blockArray = (Block_t*)malloc(blockCount*sizeof(Block_t));
		for (k = 0; k < blockCount; k++){
			temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
			flashDevice->blockArray[k].invalidCounter = temp;
			temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
			flashDevice->blockArray[k].deleteCounter = temp;
			temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
			flashDevice->blockArray[k].writePos = temp;
			temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
			flashDevice->blockArray[k].status = (BlockStatus_t)temp;
		}
		temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
		flashDevice->invalidCounter = temp;
		temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
		flashDevice->freeBlocks = temp;
		temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
		flashDevice->actWriteBlock = temp;

		// Initialisiere Pools
		flashDevice->hotPool = initList(flashDevice->blockArray);
		flashDevice->neutralPool = initList(flashDevice->blockArray);
		flashDevice->coldPool = initList(flashDevice->blockArray);

		// befülle neutralPool
		for (i = 0; i < FL_getBlockCount(); i++){
			addBlock(flashDevice->neutralPool, i);
		}

		//setze flashDevice->AVG 
		flashDevice->AVG = flashDevice->neutralPool->AVG;

		//führe grouping durch
		grouping(flashDevice);

	}
	else{
		flashDevice->mappingTable = (uint32_t*)malloc(getMappingTableSize()*sizeof(uint32_t));
		flashDevice->mappingTableRev = (uint32_t*)malloc((getMappingTableSize() + 1)*sizeof(uint32_t));
		flashDevice->blockArray = (Block_t*)malloc(FL_getBlockCount()*sizeof(Block_t));

		for (i = 0; i < FL_getBlockCount(); i++){
			flashDevice->blockArray[i].invalidCounter = 0;
			flashDevice->blockArray[i].deleteCounter = 0;
			flashDevice->blockArray[i].writePos = 0;
			flashDevice->blockArray[i].status = ready;
		}
		// setze Adressen auf 0
		for (i = 0; i < getMappingTableSize(); i++){
			flashDevice->mappingTable[i] = 0;
		}
		
		for (i = 0; i < getMappingTableSize(); i++){
			flashDevice->mappingTableRev[i] = getMappingTableSize();
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
		// befülle neutralPool
		for (i = 0; i < FL_getBlockCount(); i++){
			addBlock(flashDevice->neutralPool, i);
		}
	}

	printf("SSD initialisiert!\n");
	free(state);
	return flashDevice;
}

void convert32To8(uint8_t *data, uint32_t source){
	data[0] = source;
	data[1] = source >> 8;
	data[2] = source >> 16;
	data[3] = source >> 24;
}

uint32_t convert8To32(uint8_t a, uint8_t b, uint8_t c, uint8_t d){
	uint32_t temp = a << 24 | b << 16 | c << 8 | d;
	return temp;
}

flash_t *unmount(flash_t *flashDevice){
	uint8_t *state;
	uint32_t i, k, temp;
	uint32_t pos = 0;
	uint8_t data[4];
	uint32_t blockCount = 0;
	uint32_t mappingTableCount = 0;
	uint32_t size = 0;
	uint32_t charCounter = 0;
	uint8_t charElement = 0;

	if (flashDevice == NULL) return NULL;

	state = (uint8_t*)malloc(SAVE_STATE_SIZE* sizeof(uint8_t));

	//Gesamtgröße
	convert32To8(data, 0);
	for (i = 0; i < 4; i++){
		state[pos++] = data[i];
	}
	//MappingTable	
	convert32To8(data, getMappingTableSize());
	for (i = 0; i < 4; i++){
		state[pos++] = data[i];
	}
	for (k = 0; k < getMappingTableSize(); k++){
		convert32To8(data, flashDevice->mappingTable[k]);
		for (i = 0; i < 4; i++){
			state[pos++] = data[i];
		}
	}
	//BlockCount
	convert32To8(data, FL_getBlockCount());
	for (i = 0; i < 4; i++){
		state[pos++] = data[i];
	}
	//Blöcke	
	for (k = 0; k < FL_getBlockCount(); k++){
		convert32To8(data, flashDevice->blockArray[k].invalidCounter);
		for (i = 0; i < 4; i++){
			state[pos++] = data[i];
		}
		convert32To8(data, flashDevice->blockArray[k].deleteCounter);
		for (i = 0; i < 4; i++){
			state[pos++] = data[i];
		}
		convert32To8(data, flashDevice->blockArray[k].writePos);
		for (i = 0; i < 4; i++){
			state[pos++] = data[i];
		}
		convert32To8(data, flashDevice->blockArray[k].status);
		for (i = 0; i < 4; i++){
			state[pos++] = data[i];
		}
	}
	//sonstige Variablen	
	convert32To8(data, flashDevice->invalidCounter);
	for (i = 0; i < 4; i++){
		state[pos++] = data[i];
	}
	convert32To8(data, flashDevice->freeBlocks);
	for (i = 0; i < 4; i++){
		state[pos++] = data[i];
	}
	convert32To8(data, flashDevice->actWriteBlock);
	for (i = 0; i < 4; i++){
		state[pos++] = data[i];
	}

	size = pos;
	convert32To8(data, pos);
	pos = 0;
	for (i = 0; i < 4; i++){
		state[pos++] = data[i];
	}

	//Anzahl an 512 Blöcken für Speichern
	i = size / 512;
	if (k % 512 != 0){
		i++;
	}

	//Speichere auf Hardware
	if (FL_saveState(i, state) == TRUE){
		printf("flash_t-Abbild erfolgreich gespeichert\n");
	}

	//gebe allen allozierten Speicher wieder frei
	free(state);
	freeList(flashDevice->neutralPool);
	freeList(flashDevice->hotPool);
	freeList(flashDevice->coldPool);
	free(flashDevice->mappingTable);
	free(flashDevice->mappingTableRev);
	free(flashDevice->blockArray);
	free(flashDevice);
	return flashDevice;
}

uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	uint32_t i;
	if (flashDevice == NULL) return FALSE;
	if(index > (FL_getBlockCount() - SPARE_BLOCKS)  *getBlockSegmentCount()){
		printf("Logischer Block nicht mehr im Wertebereich des Datentraegers. 0 bis %i logische Bloecke adressierbar.\n", getLogBlockCount());
		return FALSE;
	}
	//Index immer um 1 erhöht, da index == 0 für andere Zwecke verwendet wird.
	i = mapping(flashDevice, index + 1); // Mapping
	return readBlockIntern(flashDevice, i / getBlockSegmentCount(), (i % getBlockSegmentCount()) / FL_getPagesPerBlock(), ((i % getBlockSegmentCount()) % FL_getPagesPerBlock()), data); // Blocksegment auslesen

}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	if (flashDevice == NULL) return FALSE;
	if(index > (FL_getBlockCount() - SPARE_BLOCKS)  *getBlockSegmentCount()){
		printf("Logischer Block nicht mehr im Wertebereich des Datentraegers. 0 bis %i logische Bloecke adressierbar.\n", getLogBlockCount());
		return FALSE;
	}
	//Index immer um 1 erhöht, da index == 0 für andere Zwecke verwendet wird.
	return writeBlockIntern(flashDevice, index + 1, data);
}


// Getter für "Konstanten" Implementation
////////////////////////////////////////////////////////////////////
uint32_t getBlockSegmentCount(){
	return FL_getPagesPerBlock()  *FL_getPageDataSize() / LOGICAL_BLOCK_DATASIZE;
}
uint32_t getMappingTableSize(){
	return FL_getBlockCount()  *getBlockSegmentCount();
}
uint32_t getLogBlockCount(){
	return (FL_getBlockCount() - SPARE_BLOCKS)  *getBlockSegmentCount();
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
		printf("Freie Blocks: %i\nInvalide Segmente: %i (Schwellwert %i)\nAktuelle Schreibposition B:%i / S:%i\nUnmount Fehler: %c \nMapping Table Size: %i \n\n"
			, flashDevice->freeBlocks, flashDevice->invalidCounter, calcLevel, block, segment, unMountErr, getMappingTableSize());
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
				for (j = 0; j < getBlockSegmentCount(); j++){
					marker = ' ';
					error = ' ';
					if (flashDevice->blockArray[i].writePos == j){ 
						marker = '+'; 
					}
					if (block == i && segment == j){ marker = 'S'; }
					if (segmentStatus(flashDevice, i, j) != assigned && getMapT(flashDevice, i, j) != 0){ error = '!'; }
					if (segmentStatus(flashDevice, i, j) == assigned && getMapT(flashDevice, i, j) == 0){ error = '!'; }
					//if (mapping(flashDevice, getMapT(flashDevice, i, j)) == getMappingTableSize() && getMapT(flashDevice, i, j) == 0){ error = '!'; }
					//if (mapping(flashDevice, getMapT(flashDevice, i, j)) != getMappingTableSize() && getMapT(flashDevice, i, j) != 0){ error = '!'; }
					//del = mapping(flashDevice, getMapT(flashDevice, i, j));
					if (segmentStatus(flashDevice, i, j) > 2){ error = '?'; }
					printf("Segment %02i: Table: %03i ( %04i ) - %i %c %c\n", 
						j, getMapT(flashDevice, i, j), mapping(flashDevice,getMapT(flashDevice, i, j)), segmentStatus(flashDevice, i, j), error, marker);
				}
				printf("Segment: empty (0) / assigned (1) / invalid (2)\n");				
				getchar();
			}
		}
		printerr(flashDevice);
	}
}
