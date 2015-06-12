/* Implementierung eines FTL's */
#include "ftl.h"

// Getter f�r "Konstanten"
////////////////////////////////////////////////////////////////////
/* 
 *	Gibt die Anzahl der Segmente pro Block zur�ck
 */
uint32_t getBlockSegmentCount();

/* 
 *	Gibt die Anzahl der Segment  *gesamte Bl�cke zur�ck
 */
uint32_t getMappingTableSize();

/* 
 *	Gibt die Anzahl der adressierbaren logischen Bl�cke zur�ck
 */
uint16_t getLogBlockCount();

// Funktionen Allocator
////////////////////////////////////////////////////////////////////

/* Getter Map 
  *�bersetzt die physikalische Position in die  logischen Nummer eines Datenblocks
  *Gibt die logische Nummer eines Datenblocks zur�ck
 */
uint32_t getMapT(flash_t *flashDevice, uint16_t block, uint32_t seg);

/* Setter Map
  *Verkn�pft die logischen Nummer eines Datenblocks mit der physikalische Position in der Tabelle
  *Gibt die logische Nummer eines Datenblocks zur�ck
 */
void setMapT(flash_t *flashDevice, uint16_t block, uint32_t seg, uint32_t v);

/* mapping
  *�bersetzt die logischen Nummer eines Datenblocks in die physikalische Position 
  *Gibt die physikalische Position eines Datenblocks zur�ck
 */
uint32_t mapping(flash_t *flashDevice, uint32_t index);

/* 
  *Gibt Auskunft �ber den Status eines Elements
 */
StatusPageElem_t segmentStatus(flash_t *flashDevice, uint16_t block, uint32_t segment);

/* 
  *L�scht einen Eintrag in der Map und invalidiert das dazugeh�rige Segment
 */
void invalidationOfOldIndex(flash_t *flashDevice, uint16_t block, uint32_t segment);

// Funktionen WearLeveling
////////////////////////////////////////////////////////////////////

/* 
 *	Schreibt ein Datensegment in einen Block
 *	Der R�ckgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t writeSegmentToBlock(flash_t *flashDevice, uint8_t* data, uint16_t block);

/* 
 *	Grouping nach Algorithmus
 */
void grouping(flash_t *flashDevice);

/* 
 *	Neutralisation nach Algorithmus
 */
void neutralisation(flash_t *flashDevice, List_t *pool, uint16_t deletedBlock, uint8_t hotNeutralisation);

/* 
 *	L�scht den �bergebenden Block, und datet ihn im �bergebenden Pool up
 */
uint8_t deleteBlock(flash_t *flashDevice, uint16_t deletedBlock, uint16_t inPool);

/* 
 *	Interne Schreibfunktion
 */
uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data);

/*
 *	Kopiert einen Block in den anderen
 */
void moveBlock(flash_t* flashDevice, uint16_t fromBlock, uint16_t toBlock);

/* 
 *	WearLeveling Algorithmus nach [TC11]
 *	�bergabeparameter ist eine Instanz von flash_t und der gerade gel�schte Block
 */
uint8_t wearLeveling(flash_t *flashDevice, uint16_t deletedBlock);

// Funktionen Garbage Collector
////////////////////////////////////////////////////////////////////
/* 
  *Zusammenfassung der geschriebenen Daten (Bereinigung von invalidierten Segmenten), L�schen kompletter Bl�cke. 
 */
uint8_t garbageCollector(flash_t *flashDevice);

/* 
 *	L�scht den Block auf der Hardware
 *	der R�ckgabewert ist als boolscher Wert zu interpretieren
 */
uint8_t cleanBlock(flash_t *flashDevice, uint16_t block);

// Funktionen FTL lokal
////////////////////////////////////////////////////////////////////

/*
*	Wandelt vier uint8_t zu einem uint32_t um
* */
uint32_t convert8To32(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

/*
*	Wandelt einen uint32_t, source, zu vier uint8_t, data[4], um
*/
void convert32To8(uint8_t *data, uint32_t source);

/* 
 *	Gibt einen neuen beschreibbaren Block zur�ck(mit status == ready)
 */
uint16_t nextBlock(flash_t *flashDevice);

/* 
 *Liest einen Datenblock an der angegebene physikalischen vom Flashspeicher, der mit der
 *in flashDevice �bergebenen Datenstruktur verwaltet wird.
 *flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 *dieses Flash-Datentr�gers dient.
 *index ist die Nummer des zu lesenden Blocks.
 *data ist ein Pointer auf einen ausreichend gro�en Datenbereich, in den die zu lesenden
 *Daten kopiert werden.
 *Der R�ckgabewert ist als Boolescher Wert zu interpretieren.
*/
uint8_t readBlockIntern(flash_t *flashDevice, uint16_t block, uint16_t page, uint16_t index, uint8_t *data);

void printblock(flash_t *flashDevice, uint16_t block, uint32_t segment);

// Funktionsimplementation Wear-Leveler ([TC11]- Algorithmus)
////////////////////////////////////////////////////////////////////

uint8_t writeSegmentToBlock(flash_t *flashDevice, uint8_t* data, uint16_t block){
	uint16_t count = 0;
	uint16_t page;
	uint16_t bp_index;

	if(block == -1){
		return FALSE;
	}

	//Abfrage, ob in den zu schreibender Block beschreibbar ist
	if (flashDevice->blockArray[block].status != ready){
		return FALSE;
	}
	
	page = flashDevice->blockArray[block].writePos / FL_getPagesPerBlock();
	bp_index = flashDevice->blockArray[block].writePos % FL_getPagesPerBlock();

	count = FL_writeData(block, page, bp_index  * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data);
	// Pr�fen ob wirklich entsprechende Daten geschrieben wurden	
	if (count != LOGICAL_BLOCK_DATASIZE){
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}

	flashDevice->blockArray[block].writePos++;		
	// Block ist komplett gef�llt	
	if (flashDevice->blockArray[block].writePos == getBlockSegmentCount() ){
		flashDevice->blockArray[block].writePos = getBlockSegmentCount() -1;
		flashDevice->blockArray[block].status = used;
		flashDevice->freeBlocks--;
	}
	return TRUE;
}

void moveBlock(flash_t* flashDevice, uint16_t fromBlock, uint16_t toBlock){
	uint8_t data[LOGICAL_BLOCK_DATASIZE];
	uint32_t tempAddress = 0, writePos, p;

	for (p = 0; p < getBlockSegmentCount(); p++){
		if (segmentStatus(flashDevice, fromBlock, p) == assigned){
			tempAddress = getMapT(flashDevice, fromBlock, p);
			readBlockIntern(flashDevice, fromBlock, (uint16_t)p / FL_getPagesPerBlock(), (uint16_t) p % FL_getPagesPerBlock(), data);
			invalidationOfOldIndex(flashDevice, fromBlock, p);
			writePos = flashDevice->blockArray[toBlock].writePos;
			writeSegmentToBlock(flashDevice, data, toBlock);
			//Adressupdate																				
			setMapT(flashDevice, toBlock, writePos, tempAddress);
		}
	}
	if (DEBUG_MESSAGE == TRUE) { printf("Inhalt von Block %i nach %i verschoben\n", fromBlock, toBlock); }
}

void neutralisation(flash_t *flashDevice, List_t *pool, uint16_t deletedBlock, uint8_t hotNeutralisation){
	uint32_t p, i;
	uint8_t data[LOGICAL_BLOCK_DATASIZE];	
	uint32_t tempAddress = 0;
	uint16_t spareBlock = 0;
	uint16_t blockNr = FL_getBlockCount();
	uint16_t writePos, oldBlock;
	
	//w�hle spareBlock aus
	spareBlock = nextBlock(flashDevice);

		//step 1 kopiere valide Segmente in spareBlock
			moveBlock(flashDevice, deletedBlock, spareBlock);
			printf("Tausche Block %i mit %i\n", deletedBlock, spareBlock);
			//step 2 l�sche Block und setze Z�hler hoch
			cleanBlock(flashDevice, deletedBlock);			

			//Fehlerfall, es sind keine Elemente mehr im neutralPool [Workaround]
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

			//step 3 kopiere alle validen Segmente aus Min(neutralPool)/Max(neutralPool) in deletedBlock			
			// und step 5 update der Adressen
			if(hotNeutralisation == TRUE){							
				blockNr = showFirstElement(flashDevice->neutralPool)->blockNr;
			}
			else{
				blockNr = showLastElement(flashDevice->neutralPool)->blockNr;
			}
			moveBlock(flashDevice, blockNr, deletedBlock);
			printf("Tausche Block %i mit %i\n", blockNr, deletedBlock);

			//speichere den eben verwendeten Block zwischen
			oldBlock = blockNr;

			//step 4 kopiere aus spareBlock(data) in MAX(neutralPool)/Min(neutralPool)
			//falls dieser nicht gen�gend Kapazit�t hat, hole neuen leeren Block mit nextBlock()
			// und step 5 update der Adressen
			if(hotNeutralisation == TRUE){
				blockNr = showLastElement(flashDevice->neutralPool)->blockNr;
			}
			else{
				blockNr = showFirstElement(flashDevice->neutralPool)->blockNr;
			}			

			for (i = 0; i < flashDevice->blockArray[spareBlock].writePos; i++){
				printf("Tausche Block %i mit [%i]\n", spareBlock, blockNr);
				do {
					tempAddress = getMapT(flashDevice, spareBlock, i);
					readBlockIntern(flashDevice, spareBlock, (uint16_t)i / FL_getPagesPerBlock(), (uint16_t)(i % FL_getPagesPerBlock()), data);
					writePos = flashDevice->blockArray[blockNr].writePos;
					p = writeSegmentToBlock(flashDevice, data, blockNr);
					// falls das Segment nicht geschrieben wurde
					if (p == FALSE){
						//l�sche den alten Min/Max des neutralen Pools
						printerr(flashDevice);
						cleanBlock(flashDevice, oldBlock);
						delBlock(flashDevice->neutralPool, oldBlock);
						addBlock(flashDevice->neutralPool, oldBlock);
						blockNr = oldBlock;
					}
					else {
						invalidationOfOldIndex(flashDevice, spareBlock, i);
						//Adressupdate								
						setMapT(flashDevice, blockNr, writePos, tempAddress);
					}
				} while (p == FALSE);
			}
			
			//l�sche den verwendeten SpareBlock
			cleanBlock(flashDevice, spareBlock);
			if( isElementOfList(flashDevice->neutralPool, spareBlock) == TRUE){
				delBlock(flashDevice->neutralPool, spareBlock);
				addBlock(flashDevice->neutralPool, spareBlock);
			}
			if( isElementOfList(flashDevice->hotPool, spareBlock) == TRUE){
				delBlock(flashDevice->hotPool, spareBlock);
				addBlock(flashDevice->hotPool, spareBlock);
			}
			if( isElementOfList(flashDevice->coldPool, spareBlock) == TRUE){
				delBlock(flashDevice->coldPool, spareBlock);
				addBlock(flashDevice->coldPool, spareBlock);
			}
			
			//Update des Blocks in Pool
			if (delBlock(pool, deletedBlock) == TRUE){
				addBlock(pool, deletedBlock);
			}
						
			if (DEBUG_MESSAGE == TRUE) { printf("Block %i neutralisiert (hot = %i)\n", deletedBlock, hotNeutralisation); }
}

void checkFreeBlocks(flash_t *flashDevice){
	uint16_t i, counter = 0;

	for(i = 0; i < FL_getBlockCount(); i++){
		if(flashDevice->blockArray[i].status == ready){
			counter++;
		}
	}
	flashDevice->freeBlocks = counter;
}

uint8_t deleteBlock(flash_t *flashDevice, uint16_t deletedBlock, uint16_t inPool){
	uint16_t spareBlock;
	uint8_t temp;
	if (DEBUG_MESSAGE == TRUE) { printf("loesche Block %i\n", deletedBlock); }
	//Abfrage f�r badBlock-Verhalten
	if (flashDevice->freeBlocks == 0){
		return FALSE;
	}
	spareBlock = nextBlock(flashDevice);

	//L�sche Block
	// kopiere Inhalt in spareBlock, l�sche deletedBlock
	moveBlock(flashDevice, deletedBlock, spareBlock);
	temp = cleanBlock(flashDevice, deletedBlock);
	//falls ein BadBlock entstanden ist
	if (temp == FALSE){
		return FALSE;
	}
	//Update des Blocks in Pool
	if (inPool == 1){
		delBlock(flashDevice->neutralPool, deletedBlock);
		addBlock(flashDevice->neutralPool, deletedBlock);
	}
	if (inPool == 2){
		delBlock(flashDevice->hotPool, deletedBlock);
		addBlock(flashDevice->hotPool, deletedBlock);
	}
	if (inPool == 3){
		delBlock(flashDevice->coldPool, deletedBlock);
		addBlock(flashDevice->coldPool, deletedBlock);
	}
	return TRUE;
}

void grouping(flash_t *flashDevice){
	ListElem_t *element = NULL;
	uint16_t blockNr;

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
	//�berpr�fe coldPool
	element = showLastElement(flashDevice->coldPool);
	while(element != NULL && EC(flashDevice->coldPool, element->blockNr) > flashDevice->AVG - THETA) {
		blockNr = element->blockNr;
		delBlock(flashDevice->coldPool, blockNr);
		addBlock(flashDevice->neutralPool, blockNr);
		element = showLastElement(flashDevice->coldPool);
	}
	//�berpr�fe hotPool
	element = showFirstElement(flashDevice->hotPool);
	while(element != NULL && EC(flashDevice->hotPool, element->blockNr) < flashDevice->AVG + THETA) {
		blockNr = element->blockNr;
		delBlock(flashDevice->hotPool, blockNr);
		addBlock(flashDevice->neutralPool, blockNr);
		element = showFirstElement(flashDevice->hotPool);
	}
}

uint8_t wearLeveling(flash_t *flashDevice, uint16_t deletedBlock){
	uint8_t temp = TRUE;
	if (DEBUG_MESSAGE == TRUE) { printf("Wear Leveling von Block %i\n", deletedBlock); }
	//Average Recalculation gesamt
	flashDevice->AVG += (double)1 / FL_getBlockCount();

	//erase operation in neutral pool?	
	if (isElementOfList(flashDevice->neutralPool, deletedBlock) == TRUE){
		
		//Average Recalculation Neutral
		recalculationAVG(flashDevice->neutralPool);		
	
		//l�sche deletedBlock		
		temp = deleteBlock(flashDevice, deletedBlock, 1);
		if(temp == FALSE){
			return FALSE;
		}

		//Grouping f�r alle Bl�cke in neutralPool
		grouping(flashDevice);
		
	}
	//erase operation in hot pool
	if (isElementOfList(flashDevice->hotPool, deletedBlock) == TRUE){
						
		//Average Recalculation Hot
		recalculationAVG(flashDevice->hotPool);
		/*
		//check condition 3
		if (flashDevice->blockArray[deletedBlock].deleteCounter > flashDevice->hotPool->AVG + DELTA){
					
			//Neutralisation
			//printf("\nwarme neutralisation \n");
			neutralisation(flashDevice, flashDevice->hotPool, deletedBlock, TRUE);		
			return;
		}
		*/
		//l�sche deletedBlock		
		deleteBlock(flashDevice, deletedBlock, 2);
	}
	//erase operation in cold pool
	if (isElementOfList(flashDevice->coldPool, deletedBlock) == TRUE){
				
		//Average Recalculation Cold
		recalculationAVG(flashDevice->coldPool);
		/*
		//check condition 3
		if (flashDevice->blockArray[deletedBlock].deleteCounter < flashDevice->coldPool->AVG - DELTA){
			//Neutralisation
			//printf("\nkalte neutralisation Block: %i \n", deletedBlock);
			neutralisation(flashDevice, flashDevice->coldPool, deletedBlock, FALSE);		
			return;			
		}*/
		
		//l�sche deletedBlock		
		deleteBlock(flashDevice, deletedBlock, 3);
	}	
	if(temp == FALSE){
		return FALSE;
	}
	else{
		return TRUE;
	}
}

// Lokale Funktionsimplementation Allocator
////////////////////////////////////////////////////////////////////
uint32_t getMapT(flash_t *flashDevice, uint16_t block, uint32_t seg){
	return flashDevice->mappingTable[(block  *getBlockSegmentCount()) + seg];
}

void setMapT(flash_t *flashDevice, uint16_t block, uint32_t seg, uint32_t v){
	uint32_t hw_addr = (block  *getBlockSegmentCount()) + seg;
	// Auto Invalidieren
	/*uint32_t inval = mapping(flashDevice, v);
	if (inval != getMappingTableSize()){
		flashDevice->mappingTable[inval] = 0;
		flashDevice->invalidCounter++;
		flashDevice->blockArray[inval / FL_getBlockCount()].invalidCounter++;
		flashDevice->mappingTableRev[v] = getMappingTableSize();
	}*/

	// Sonderfall Index auf Null setzten (invalidieren)
	if (v == getMappingTableSize()){
		hw_addr = getMappingTableSize();
	}
		// Index updaten
	flashDevice->mappingTableRev[v] = hw_addr;
	flashDevice->mappingTable[hw_addr] = v;
	//if (DEBUG_MESSAGE == TRUE) { printf("Mappe L: %03i auf B: %02i S: %02i\n", v, block, seg); }
}

// Gibt 512 Speicherplatz representation zur�ck [Blocksegment]
uint32_t mapping(flash_t *flashDevice, uint32_t index){
	if (index == 0) return getMappingTableSize();
	return flashDevice->mappingTableRev[index];
}

StatusPageElem_t segmentStatus(flash_t *flashDevice, uint16_t block, uint32_t segment){
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

void invalidationOfOldIndex(flash_t *flashDevice, uint16_t block, uint32_t segment){
	setMapT(flashDevice, block , segment , 0);	
	flashDevice->invalidCounter++;
	flashDevice->blockArray[block].invalidCounter++;	
}

// Lokale Funktionsimplementation  Cleaner
////////////////////////////////////////////////////////////////////
uint8_t garbageCollector(flash_t *flashDevice){	
	uint16_t i = flashDevice->actWriteBlock, k = 0;		
	uint8_t temp = TRUE;
	uint32_t level = flashDevice->invalidCounter / FL_getBlockCount(); //Anzahl der zu bereinigen Blocks, dynamisch berechnet
	
	if (DEBUG_MESSAGE == TRUE) { printf("Garbage Collection\n"); }

	// Verhindern dass Bl�cke ohne invalide eintr�ge gecleant werden
	if (level < 1){
		level = 1;
	}	
		
	// Solange noch nicht alle Bloecke durchlaufen wurden oder genug Bloecke gereinigt wurden				
	while (flashDevice->freeBlocks <= SPARE_BLOCKS  && k  < FL_getBlockCount ()  ){ 		
		if( i >= FL_getBlockCount() - 1){
			i = 0;
		}
		else{
			i++;
		}
				
		// Wenn Block �ber Schwellwert liegt und benutzt wird				
		if ( flashDevice->blockArray[i].invalidCounter >= level && flashDevice->blockArray[i].status == used){ 					
			temp = wearLeveling(flashDevice, i);				
			if(temp == FALSE){
				return FALSE;
			}
		}
		k++;		
	}
	//�berpr�fe Anzahl freeBlocks
	checkFreeBlocks(flashDevice);	
	if(flashDevice->freeBlocks == 0){
		return FALSE;
	}
	return TRUE;
}

uint8_t cleanBlock(flash_t *flashDevice, uint16_t block){
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
		return TRUE;
	}
	else {
		flashDevice->invalidCounter = flashDevice->invalidCounter - flashDevice->blockArray[block].invalidCounter;
		flashDevice->blockArray[block].status = badBlock; // Status auf BadBlock setzen
		flashDevice->badBlockCounter++;
		printf("Fehler! BadBlock (%i) \n", block);
		//Herausnehmen aus Pools
		if( isElementOfList(flashDevice->neutralPool, block) == TRUE){
			delBlock(flashDevice->neutralPool, block);
		}
		if( isElementOfList(flashDevice->hotPool, block) == TRUE){
			delBlock(flashDevice->hotPool, block);
		}
		if( isElementOfList(flashDevice->coldPool, block) == TRUE){
			delBlock(flashDevice->coldPool, block);			
		}
		return FALSE;
	}

}

// Lokale Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////


uint8_t readBlockIntern(flash_t *flashDevice, uint16_t block, uint16_t page, uint16_t index, uint8_t *data){
	uint16_t count = FL_readData(block, page, index  *LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Blocksegment auslesen
	if (count != LOGICAL_BLOCK_DATASIZE){ // Pr�fen ob wirklich entsprechende Daten gelesen wurden		}
		printf("Fehler beim Lesen des Datensatzes! %d von %d byte gelesen\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}
	return TRUE;
}

uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data){

	uint16_t block = flashDevice->actWriteBlock;
	uint16_t page = flashDevice->blockArray[flashDevice->actWriteBlock].writePos / FL_getPagesPerBlock();
	uint16_t bp_index = flashDevice->blockArray[flashDevice->actWriteBlock].writePos % FL_getPagesPerBlock();
	uint16_t count;
	uint32_t index_old;

	if(flashDevice->freeBlocks == 0){
		return FALSE;
	}
	// Der Block auf dem geschrieben wird, ist nicht beschreibbar
	if (flashDevice->blockArray[block].status != ready){ 
		block = nextBlock(flashDevice);		
	}	
	// Daten beschreiben
	count = FL_writeData(block, page, bp_index  *LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data);
	if (count != LOGICAL_BLOCK_DATASIZE){ // Pr�fen ob wirklich entsprechende Daten geschrieben wurden		
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);		
		printerr(flashDevice);
		return FALSE;
	}
	//Invalidiere alten Index
	index_old =  mapping(flashDevice, index);
	if( index_old != getMappingTableSize()){
		// Alten Eintrag in Mapping Table und Block invalidieren		
		invalidationOfOldIndex(flashDevice, (uint16_t)index_old / getBlockSegmentCount(),index_old % getBlockSegmentCount()); 
	}
	//printf("Index! %i (%i/%i/%i) \n" , index, block, page, bp_index);//TODO DEBUG-Ausgabe rausnehmen
	// Setze Mapeintrag
	setMapT(flashDevice, block, (page  *FL_getPagesPerBlock()) + bp_index, index);

	// Auswahl des n�chsten Schreibortes
	// position weiterz�hlen wenn innerhalb des selben blockes
	if (flashDevice->blockArray[flashDevice->actWriteBlock].writePos < getBlockSegmentCount() - 1){ 
		flashDevice->blockArray[flashDevice->actWriteBlock].writePos++;
	}
	// oder einen neuen Block ausw�hlen
	else { 
		// alten Block abschlie�en
		flashDevice->blockArray[flashDevice->actWriteBlock].status = used;
		flashDevice->freeBlocks--;		
		// Freien Block finden und nutzen
		flashDevice->actWriteBlock = nextBlock(flashDevice);	
		if (DEBUG_MESSAGE == TRUE) {
			printblock(flashDevice,flashDevice->actWriteBlock, 15);
		}
		// Cleaner		
		if (flashDevice->freeBlocks <= SPARE_BLOCKS ){
			// Clean
			return garbageCollector(flashDevice); 			
		}
	}
	return TRUE;
}

ListElem_t* findNextBlock(flash_t *flashDevice, List_t *list, uint8_t notNew){
	ListElem_t *element;
	for (element = showFirstElement(list); element != NULL; element = getNextElement(element)){
		if (flashDevice->blockArray[element->blockNr].status == ready){
			if (flashDevice->blockArray[element->blockNr].writePos == 0 || notNew == TRUE){
				return element;
			}
		}
	}
	return NULL;
}

uint16_t nextBlock(flash_t *flashDevice){
	ListElem_t *element;
	if (DEBUG_MESSAGE == TRUE) { printf("Waehle naechsten Block\n"); }
	//Auswahl eines komplett leeren Blocks
	//nehme k�ltesten, beschreibbaren Block aus coldPool			
	element = findNextBlock(flashDevice, flashDevice->coldPool, FALSE);
	if (element != NULL) return element->blockNr;
	//nehme k�ltesten beschreibbaren Block aus neutralPool
	element = findNextBlock(flashDevice, flashDevice->neutralPool, FALSE);
	if (element != NULL) return element->blockNr;
	//nehme k�ltesten, beschreibbaren Block aus hotPool
	element = findNextBlock(flashDevice, flashDevice->hotPool, FALSE);
	if (element != NULL) return element->blockNr;
	//nehme jetzt einen nichtleeren und nichtvollen Block
	element = findNextBlock(flashDevice, flashDevice->coldPool, TRUE);
	if (element != NULL) return element->blockNr;
	//nehme k�ltesten beschreibbaren Block aus neutralPool
	element = findNextBlock(flashDevice, flashDevice->neutralPool, TRUE);
	if (element != NULL) return element->blockNr;
	//nehme k�ltesten, beschreibbaren Block aus hotPool
	element = findNextBlock(flashDevice, flashDevice->hotPool, TRUE);
	if (element != NULL) return element->blockNr;
	//nehme jetzt einen nichtleeren und nichtvollen Block	
	printf("Keinen freien Block in Pools gefunden.\n");
	printerr(flashDevice);
	return flashDevice->actWriteBlock; // Aktuellen Schreibblock zur�ck geben;
}

// Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////


flash_t  *mount(flashMem_t *flashHardware){
	flash_t *flashDevice;
	uint32_t i, k, temp;
	ListElem_t *element = NULL;
	uint8_t *state;
	// uint32_t pos = 0;
	//uint8_t data[4];
	uint16_t blockCount = 0;
	uint32_t mappingTableCount = 0;
	uint32_t size = 0;
	uint32_t charCounter = 0;	
			
	state = (uint8_t*)malloc(SAVE_STATE_SIZE * sizeof(uint8_t));
	flashDevice = (flash_t*)malloc(sizeof(flash_t));
	flashDevice->mappingTable = (uint32_t*)malloc(getMappingTableSize()*sizeof(uint32_t));
	flashDevice->mappingTableRev = (uint32_t*)malloc((getMappingTableSize())*sizeof(uint32_t));
	flashDevice->blockArray = (Block_t*)malloc(FL_getBlockCount()*sizeof(Block_t));
	// Initialisiere Pools		
	flashDevice->hotPool = initList(flashDevice->blockArray);
	flashDevice->neutralPool = initList(flashDevice->blockArray);
	flashDevice->coldPool = initList(flashDevice->blockArray);

	// Laden von flash_t
	if (FL_restoreState(state) == state){
				
		//laden		
		i = 0;		
		size = convert8To32(state[i++], state[i++], state[i++], state[i++]);		
		mappingTableCount = convert8To32(state[i++], state[i++], state[i++], state[i++]);			
		for (k = 0; k < mappingTableCount; k++){
			temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
			flashDevice->mappingTable[k] = temp;			
		}

		for (k = 0; k < mappingTableCount; k++){
			flashDevice->mappingTableRev[k] = getMapT(flashDevice, k / FL_getBlockCount(), k % FL_getBlockCount());
		}
		
		blockCount = convert8To32(state[i++], state[i++], state[i++], state[i++]);		
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
		temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
		flashDevice->badBlockCounter = temp;
		
		// bef�lle neutralPool
		for (i = 0; i < FL_getBlockCount(); i++){
			addBlock(flashDevice->neutralPool, i);
		}

		//setze flashDevice->AVG 
		flashDevice->AVG = flashDevice->neutralPool->AVG;

		//f�hre grouping durch
		grouping(flashDevice);
		
	}
	else{				

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
		flashDevice->badBlockCounter = 0;

		// Initialisiere AVG
		flashDevice->AVG = 0;
		// bef�lle neutralPool
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
	uint32_t i, k;
	uint32_t pos = 0;
	uint8_t data[4];
	uint32_t blockCount = 0;
	uint32_t mappingTableCount = 0;
	uint32_t size = 0;
	uint32_t charCounter = 0;
	uint8_t charElement = 0;

	if (flashDevice == NULL) return NULL;

	state = (uint8_t*)malloc(SAVE_STATE_SIZE* sizeof(uint8_t));
	
	//Gesamtgr��e
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
	//Bl�cke	
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
	convert32To8(data, flashDevice->badBlockCounter);
	for (i = 0; i < 4; i++){
		state[pos++] = data[i];
	}

	size = pos;
	convert32To8(data, pos);
	pos = 0;
	for (i = 0; i < 4; i++){
		state[pos++] = data[i];
	}

	//Anzahl an 512 Bl�cken f�r Speichern
	i = size / 512;
	if (k % 512 != 0){
		i++;
	}

	//Speichere auf Hardware
	if (FL_saveState((uint8_t)i, state) == TRUE){
		printf("flash_t-Abbild erfolgreich gespeichert\n");
	}

	//gebe allen allozierten Speicher wieder frei
	free(state);	
	freeList(flashDevice->neutralPool);
	freeList(flashDevice->hotPool);
	freeList(flashDevice->coldPool);
	free(flashDevice->blockArray);
	free(flashDevice->mappingTable);
	free(flashDevice->mappingTableRev);		
	free(flashDevice);	
	flashDevice = NULL;
	return flashDevice;
}

uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	uint32_t i;
	if (flashDevice == NULL) return FALSE;
	if (DEBUG_MESSAGE == TRUE) { printf("Lesezugriff %i\n", index); }
	if (index > (FL_getBlockCount() - (uint16_t)SPARE_BLOCKS)  * getBlockSegmentCount()){
		printf("Logischer Block nicht mehr im Wertebereich des Datentraegers. 0 bis %i logische Bloecke adressierbar.\n", getLogBlockCount());
		return FALSE;
	}
	//Index immer um 1 erh�ht, da index == 0 f�r andere Zwecke verwendet wird.
	i = mapping(flashDevice, index + 1); // Mapping
	return readBlockIntern(flashDevice, (uint16_t)i / getBlockSegmentCount(), (uint16_t)(i % getBlockSegmentCount()) / FL_getPagesPerBlock(), (uint16_t)((i % getBlockSegmentCount()) % FL_getPagesPerBlock()), data); // Blocksegment auslesen

}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	if (flashDevice == NULL) return FALSE;
	if (DEBUG_MESSAGE == TRUE) { printf("Schreibzugriff %i\n", index); }
	if (index > (FL_getBlockCount() - (uint16_t)SPARE_BLOCKS)  *getBlockSegmentCount()){
		printf("Logischer Block nicht mehr im Wertebereich des Datentraegers. 0 bis %i logische Bloecke adressierbar.\n", getLogBlockCount());
		return FALSE;
	}	
	if( flashDevice->freeBlocks == 0){
		printf("Anzahl an BadBlocks = %i. Datentraeger nicht mehr beschreibbar.\n", flashDevice->badBlockCounter);
		return FALSE;
	}
	//Index immer um 1 erh�ht, da index == 0 f�r andere Zwecke verwendet wird.
	return writeBlockIntern(flashDevice, index + 1, data);
}


// Getter f�r "Konstanten" Implementation
////////////////////////////////////////////////////////////////////
uint32_t getBlockSegmentCount(){
	return  FL_getPagesPerBlock()  * FL_getPageDataSize() / LOGICAL_BLOCK_DATASIZE;
}
uint32_t getMappingTableSize(){
	return FL_getBlockCount()  *getBlockSegmentCount();
}
uint16_t getLogBlockCount(){
	return   (uint16_t) (FL_getBlockCount() - SPARE_BLOCKS)  * getBlockSegmentCount();
}

// DEBUG Funktionsimplementation FLT
////////////////////////////////////////////////////////////////////

void printblock(flash_t *flashDevice, uint16_t block, uint32_t segment){
	uint32_t j;
	char marker, error;
	printf("\nBlock %02i:\n", block);
	if (flashDevice->blockArray[block].status == ready){
			printf(" Status: Ready (0)\n");
		}
	else if (flashDevice->blockArray[block].status == used){
			printf(" Status. Used (1)\n");
		}
		else{
			printf(" Status: Bad Block (2)\n");
		}
		printf(" Invalide Segmente: %i\n Loeschanzahl: %i\n\n", flashDevice->blockArray[block].invalidCounter, flashDevice->blockArray[block].deleteCounter);
		for (j = 0; j < getBlockSegmentCount(); j++){
			marker = ' ';
			error = ' ';
			if (flashDevice->blockArray[block].writePos == j){
				marker = '+';
			}
			if (flashDevice->actWriteBlock == block && segment == j){ marker = 'S'; }
			if (segmentStatus(flashDevice, block, j) != assigned && getMapT(flashDevice, block, j) != 0){ error = '!'; }
			if (segmentStatus(flashDevice, block, j) == assigned && getMapT(flashDevice, block, j) == 0){ error = '!'; }
			//if (mapping(flashDevice, getMapT(flashDevice, i, j)) == getMappingTableSize() && getMapT(flashDevice, i, j) == 0){ error = '!'; }
			//if (mapping(flashDevice, getMapT(flashDevice, i, j)) != getMappingTableSize() && getMapT(flashDevice, i, j) != 0){ error = '!'; }
			//del = mapping(flashDevice, getMapT(flashDevice, i, j));
			if (segmentStatus(flashDevice, block, j) > 2){ error = '?'; }
			printf("Segment %02i: Table: %03i ( %04i ) - %i %c %c\n",
				j, getMapT(flashDevice, block, j), mapping(flashDevice, getMapT(flashDevice, block, j)), segmentStatus(flashDevice, block, j), error, marker);
		}
		printf("Segment: empty (0) / assigned (1) / invalid (2)\n");
}


void printerr(flash_t *flashDevice){
	uint32_t invCo = 0, del = 0, segment, calcLevel = 0;
	uint16_t block, i;
	char userInput, unMountErr = 'n';
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
			 // Block f�r Block
				printblock(flashDevice, i, segment);
				getchar();
			}
		}
		printerr(flashDevice);
	}
}
