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
uint16_t getLogBlockCount();

// Funktionen Allocator
////////////////////////////////////////////////////////////////////

/* Getter Map 
  *Übersetzt die physikalische Position in die  logischen Nummer eines Datenblocks
  *Gibt die logische Nummer eines Datenblocks zurück
 */
uint32_t getMapT(flash_t *flashDevice, uint16_t block, uint32_t seg);

/* Setter Map
  *Verknüpft die logischen Nummer eines Datenblocks mit der physikalische Position in der Tabelle
  *Gibt die logische Nummer eines Datenblocks zurück
  * Invalidiert vorhandene logische Einträge die auf die physikalische Adresse zeigen
 */
void setMapT(flash_t *flashDevice, uint16_t block, uint32_t seg, uint32_t v);

/* mapping
  *Übersetzt die logischen Nummer eines Datenblocks in die physikalische Position 
  *Gibt die physikalische Position eines Datenblocks zurück
 */
uint32_t mapping(flash_t *flashDevice, uint32_t index);

/* 
  *Gibt Auskunft über den Status eines Elements
 */
StatusPageElem_t segmentStatus(flash_t *flashDevice, uint16_t block, uint32_t segment);

// Funktionen WearLeveling
////////////////////////////////////////////////////////////////////

/* 
 *	Schreibt ein Datensegment in einen Block
 *	Der Rückgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t writeSegmentToBlock(flash_t *flashDevice, uint8_t* data, uint16_t block);

/*
 *	Interne Schreibfunktion
 * flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 * dieses Flash-Datenträgers dient.
 * index ist die logische Adresse
 * *data enthält einen Pointer auf die zu schreibenen Daten
 */
uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data);

/* 
 *	Grouping nach Algorithmus [TC11]
 */
void grouping(flash_t *flashDevice);

/* 
 *	Neutralisation nach Algorithmus [TC11]
 */
void neutralisation(flash_t *flashDevice, List_t *pool, uint16_t deletedBlock, uint8_t hotNeutralisation);

/* 
 *	Löscht den übergebenden Block, und akutalisiert ihn im übergebenden Pool
 */
uint8_t deleteBlock(flash_t *flashDevice, uint16_t deletedBlock, uint16_t inPool);

/*
 *	Kopiert einen Block in den anderen
 */
uint8_t moveBlock(flash_t* flashDevice, uint16_t fromBlock, uint16_t toBlock);

/* 
 *	WearLeveling Algorithmus nach [TC11]
 *	Übergabeparameter ist eine Instanz von flash_t und der gerade gelöschte Block
 */
uint8_t wearLeveling(flash_t *flashDevice, uint16_t deletedBlock);

// Funktionen Garbage Collector
////////////////////////////////////////////////////////////////////
/* 
  * Zusammenfassung der geschriebenen Daten (Bereinigung von invalidierten Segmenten), Löschen kompletter Blöcke.
  * (noRek Parameter ist für einmaligen selbstaufruf um Rekursion zu verhindern )
 */
uint8_t garbageCollector(flash_t *flashDevice, uint8_t noRek);

/* 
 *	Löscht den Block auf der Hardware
 *	der Rückgabewert ist als boolscher Wert zu interpretieren
 */
uint8_t cleanBlock(flash_t *flashDevice, uint16_t block);

/*
 * Prüft ob es einen freien Block gibt.
*/
void checkFreeBlocks(flash_t *flashDevice);

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
 * Interne Hilfsfunktion für nextBlock, sucht in einer Liste den nächsten (leeren oder beschreibbaren) Block
 * gibt ggf.NULL zurück (wenn kein freier Block in der Liste gefunden wurde)
*/
ListElem_t* findNextBlock(flash_t *flashDevice, List_t *list, uint8_t notNew);

/* 
 *	Gibt einen neuen beschreibbaren Block zurück(mit status == ready)
 */
uint16_t nextBlock(flash_t *flashDevice, uint8_t prio);

/* 
 *Liest einen Datenblock an der angegebene physikalischen vom Flashspeicher, der mit der
 *in flashDevice übergebenen Datenstruktur verwaltet wird.
 *flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 *dieses Flash-Datenträgers dient.
 *Block, Page,index geben die physikalische Zieladresse an.
 *data ist ein Pointer auf einen ausreichend großen Datenbereich, in den die zu lesenden
 *Daten kopiert werden.
 *Der Rückgabewert ist als Boolescher Wert zu interpretieren.
*/
uint8_t readBlockIntern(flash_t *flashDevice, uint16_t block, uint16_t page, uint16_t index, uint8_t *data);

/*
 * Interne Helferfunktion von printerr, gibt einzelnen Block auf der Konsole aus
 */
void printblock(flash_t *flashDevice, uint16_t block, uint32_t segment);

// Funktionsimplementation Wear-Leveler ([TC11]- Algorithmus)
////////////////////////////////////////////////////////////////////

uint8_t writeSegmentToBlock(flash_t *flashDevice, uint8_t* data, uint16_t block){
	uint16_t count = 0;
	uint16_t page;
	uint16_t bp_index;

	if(block == FL_getBlockCount()){
		return FALSE;
	}

	//Abfrage, ob in den zu schreibender Block beschreibbar ist
	if (flashDevice->blockArray[block].status != ready){
		return FALSE;
	}
	
	page = flashDevice->blockArray[block].writePos / FL_getPagesPerBlock();
	bp_index = flashDevice->blockArray[block].writePos % FL_getPagesPerBlock();

	count = FL_writeData(block, page, bp_index  * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data);
	// Prüfen ob wirklich entsprechende Daten geschrieben wurden	
	if (count != LOGICAL_BLOCK_DATASIZE){
		if (DEBUG_LEVEL < 5){
			printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
			printerr(flashDevice);
		}
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

uint8_t moveBlock(flash_t* flashDevice, uint16_t fromBlock, uint16_t toBlock){
	uint8_t data[LOGICAL_BLOCK_DATASIZE];
	uint32_t tempAddress = 0, writePos, p = 0;

	while (p < getBlockSegmentCount()){		
		if (segmentStatus(flashDevice, fromBlock, p) == assigned){
			tempAddress = getMapT(flashDevice, fromBlock, p);
			readBlockIntern(flashDevice, fromBlock, (uint16_t)p / FL_getPagesPerBlock(), (uint16_t)p % FL_getPagesPerBlock(), data);
			writePos = flashDevice->blockArray[toBlock].writePos;
			if (writeSegmentToBlock(flashDevice, data, toBlock) == FALSE){
				if (flashDevice->freeBlocks > 0)
				{
					toBlock = nextBlock(flashDevice, FALSE);
					p--;					
				}
				else{
					if (DEBUG_LEVEL < 5){
						printf("Warnung! MoveBlock beendet unplanmaessig! Keine freien Bloecke mehr?");
						printerr(flashDevice);
					}
					return FALSE;
				}
			}
			else {
				setMapT(flashDevice, toBlock, writePos, tempAddress);
			}			
		}		
		p++;
	}
	if (DEBUG_LEVEL < 3) { printf("Inhalt von Block %i nach %i verschoben\n", fromBlock, toBlock); }
	return TRUE;
}

void neutralisation(flash_t *flashDevice, List_t *pool, uint16_t deletedBlock, uint8_t hotNeutralisation){
	uint32_t p, i;
	uint8_t data[LOGICAL_BLOCK_DATASIZE];	
	uint32_t tempAddress = 0;
	uint16_t spareBlock = 0;
	uint16_t blockNr = FL_getBlockCount();
	uint16_t writePos, oldBlock;
	
	//wähle spareBlock aus
	spareBlock = nextBlock(flashDevice, TRUE);
	
		//step 1 kopiere valide Segmente in spareBlock
			moveBlock(flashDevice, deletedBlock, spareBlock);
			if (DEBUG_LEVEL < 4) {
				printf("1. Tausche Block %i mit %i\n", deletedBlock, spareBlock);
			}
			//step 2 lösche Block und setze Zähler hoch
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
			cleanBlock(flashDevice, blockNr);
			if (DEBUG_LEVEL < 4) {
				printf("2. Tausche Block %i mit %i\n", blockNr, deletedBlock);
			}
			//speichere den eben verwendeten Block zwischen
			//oldBlock = blockNr;

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

			//step 4 kopiere aus spareBlock(data) in MAX(neutralPool)/Min(neutralPool)
			//falls dieser nicht genügend Kapazität hat, hole neuen leeren Block mit nextBlock()
			// und step 5 update der Adressen
			if(hotNeutralisation == TRUE){
				blockNr = showLastElement(flashDevice->neutralPool)->blockNr;
			}
			else{
				blockNr = showFirstElement(flashDevice->neutralPool)->blockNr;
			}			
			if (DEBUG_LEVEL < 4) {
				printf("3. Tausche Block %i mit [%i]\n", spareBlock, blockNr);
			}
			moveBlock(flashDevice, spareBlock, blockNr);

			//lösche den verwendeten SpareBlock
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
						
			if (DEBUG_LEVEL < 4) { printf("Block %i neutralisiert (hot = %i)\n", deletedBlock, hotNeutralisation); }
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
	// Sonderfall Block kann direkt gelöscht werden
		//Abfrage für badBlock-Verhalten
		if (flashDevice->freeBlocks == 0){
			if (DEBUG_LEVEL < 3) { printf("loesche Block %i NICHT!\n", deletedBlock); }
			return FALSE;
		}
		spareBlock = nextBlock(flashDevice, TRUE);

		//Lösche Block
		// kopiere Inhalt in spareBlock, lösche deletedBlock
		moveBlock(flashDevice, deletedBlock, spareBlock);
		temp = cleanBlock(flashDevice, deletedBlock);
		//falls ein BadBlock entstanden ist
		if (temp == FALSE){
			if (DEBUG_LEVEL < 3) { printf("loesche Block %i NICHT!\n", deletedBlock); }
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
	if (DEBUG_LEVEL < 3) { printf("loesche Block %i\n", deletedBlock); }
	return TRUE;
}

void grouping(flash_t *flashDevice){
	ListElem_t *element = NULL;
	uint16_t blockNr;

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
	//Überprüfe coldPool
	element = showLastElement(flashDevice->coldPool);
	while(element != NULL && EC(flashDevice->coldPool, element->blockNr) > flashDevice->AVG - THETA) {
		blockNr = element->blockNr;
		delBlock(flashDevice->coldPool, blockNr);
		addBlock(flashDevice->neutralPool, blockNr);
		element = showLastElement(flashDevice->coldPool);
	}
	//überprüfe hotPool
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
	if (DEBUG_LEVEL < 4) { printf("Wear Leveling von Block %i (%i)\n", deletedBlock, flashDevice->blockArray[deletedBlock].invalidCounter); }
	//Average Recalculation gesamt
	flashDevice->AVG += (double)1 / FL_getBlockCount();

	//erase operation in neutral pool?	
	if (isElementOfList(flashDevice->neutralPool, deletedBlock) == TRUE){
		
		//Average Recalculation Neutral
		recalculationAVG(flashDevice->neutralPool);		
	
		//lösche deletedBlock		
		temp = deleteBlock(flashDevice, deletedBlock, 1);
		if(temp == FALSE){
			return FALSE;
		}

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
			return TRUE;
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
			return TRUE;			
		}
		
		//lösche deletedBlock		
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
	uint32_t inval = mapping(flashDevice, v);
	if (v == 0){
		inval = hw_addr;
		hw_addr = getMappingTableSize();
	}
	if (mapping(flashDevice,getMapT(flashDevice, block, seg)) != getMappingTableSize()){
		if (DEBUG_LEVEL < 5) {
			printf("Fehler! Mappingtabelle inkonsistent! (%i - %i (%i)", block, seg, v);
			printerr(flashDevice);
		}
	}
	if (inval != getMappingTableSize() && inval != hw_addr){
		flashDevice->invalidCounter++;
		flashDevice->blockArray[inval / getBlockSegmentCount()].invalidCounter++;
		//flashDevice->mappingTableRev[flashDevice->mappingTable[inval]] = getMappingTableSize();
		flashDevice->mappingTable[inval] = 0;
		//flashDevice->mappingTableRev[v] = getMappingTableSize();
	}
	// Sonderfall Index auf Null setzten (invalidieren)
	if (hw_addr >= getMappingTableSize()){
		flashDevice->mappingTableRev[v] = getMappingTableSize();
	}
	else {
		// Index updaten
		flashDevice->mappingTableRev[v] = hw_addr;
		flashDevice->mappingTable[hw_addr] = v;
	}
	if (DEBUG_LEVEL < 2) { printf("Mappe L: %03i auf B: %02i S: %02i\n", v, block, seg); }
}

// Gibt 512 Speicherplatz representation zurück [Blocksegment]
uint32_t mapping(flash_t *flashDevice, uint32_t index){
	if (index == 0) return getMappingTableSize();
	return flashDevice->mappingTableRev[index];
}

StatusPageElem_t segmentStatus(flash_t *flashDevice, uint16_t block, uint32_t segment){
	if ((flashDevice->blockArray[block].status == ready  && getMapT(flashDevice, block, segment) == 0)){ // Kompletter Block muss leer sein
		if ((flashDevice->actWriteBlock == block && flashDevice->blockArray[flashDevice->actWriteBlock].writePos > segment)
			|| mapping(flashDevice, getMapT(flashDevice, block, segment)) != getMappingTableSize()){
			return invalid;
		}
		return empty;
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

// Lokale Funktionsimplementation  Garbage Collector
////////////////////////////////////////////////////////////////////
uint8_t garbageCollector(flash_t *flashDevice, uint8_t noRek){	
	uint16_t i = flashDevice->actWriteBlock, k = 0;		
	uint8_t temp = TRUE;
	uint32_t level; //Anzahl der zu bereinigen Blocks, dynamisch berechnet
	level = 1 + (flashDevice->invalidCounter / FL_getBlockCount());
	if (DEBUG_LEVEL < 3) { printf(" --- Garbage Collection (%i) --- \n", level); }
		
	// Solange noch nicht alle Bloecke durchlaufen wurden oder genug Bloecke gereinigt wurden				
	while (flashDevice->freeBlocks < SPARE_BLOCKS   && k  < FL_getBlockCount()   ){ 		
		if( i < FL_getBlockCount()){
			i++;
		}
		else{
			i = 0;
		}
				
		// Wenn Block über Schwellwert liegt und benutzt wird				
		if ( flashDevice->blockArray[i].invalidCounter >= level && flashDevice->blockArray[i].status == used){ 					
			temp = wearLeveling(flashDevice, i);			
			if(temp == FALSE){
				return FALSE;
			}
		}
		k++;
	}
	//Überprüfe Anzahl freeBlocks
	checkFreeBlocks(flashDevice);
	if(flashDevice->freeBlocks == 0 && noRek == FALSE){
		if (DEBUG_LEVEL < 4) printf("rekursion\n");
		return garbageCollector(flashDevice, TRUE);
		//return FALSE;
	}
	return TRUE;
}

uint8_t cleanBlock(flash_t *flashDevice, uint16_t block){
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
		return TRUE;
	}
	else {
		flashDevice->invalidCounter = flashDevice->invalidCounter - flashDevice->blockArray[block].invalidCounter;
		flashDevice->blockArray[block].status = badBlock; // Status auf BadBlock setzen
		flashDevice->blockArray[block].invalidCounter = 0;
		flashDevice->badBlockCounter++;
		if (DEBUG_LEVEL < 5) printf("Fehler! BadBlock (%i) \n", block);
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
	if (count != LOGICAL_BLOCK_DATASIZE){ // Prüfen ob wirklich entsprechende Daten gelesen wurden		}
		if (DEBUG_LEVEL < 5){
			printf("Fehler beim Lesen des Datensatzes! %d von %d byte gelesen\n", count, LOGICAL_BLOCK_DATASIZE);
			printerr(flashDevice);
		}
		return FALSE;
	}
	return TRUE;
}

uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data){

	uint16_t block = flashDevice->actWriteBlock;
	uint16_t page = flashDevice->blockArray[block].writePos / FL_getPagesPerBlock();
	uint16_t bp_index = flashDevice->blockArray[block].writePos % FL_getPagesPerBlock();
	uint16_t count;
	uint8_t ret;


	if(flashDevice->freeBlocks == 0){
		return FALSE;
	}
	// Der Block auf dem geschrieben wird, ist nicht beschreibbar
	if (flashDevice->blockArray[block].status == used){
		flashDevice->actWriteBlock = nextBlock(flashDevice, FALSE);
		block = flashDevice->actWriteBlock;
		page = flashDevice->blockArray[block].writePos / FL_getPagesPerBlock();
		bp_index = flashDevice->blockArray[block].writePos % FL_getPagesPerBlock();
	}	
	// Daten beschreiben
	count = FL_writeData(block, page, bp_index  *LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data);
	if (count != LOGICAL_BLOCK_DATASIZE){ // Prüfen ob wirklich entsprechende Daten geschrieben wurden		
		if (DEBUG_LEVEL < 5) {
			printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
			printerr(flashDevice);
		}
		return FALSE;
	}
	// Setze Mapeintrag
	setMapT(flashDevice, block, (page  *FL_getPagesPerBlock()) + bp_index, index);

	// Auswahl des nächsten Schreibortes
	// position weiterzählen wenn innerhalb des selben blockes
	if (flashDevice->blockArray[block].writePos < getBlockSegmentCount() - 1){
		flashDevice->blockArray[block].writePos++;
	}
	// oder einen neuen Block auswählen
	else { 
		// alten Block abschließen
		flashDevice->blockArray[block].status = used;
		flashDevice->freeBlocks--;			
		// Freien Block finden und nutzen
		if (DEBUG_LEVEL < 1) {
			printblock(flashDevice, flashDevice->actWriteBlock, 15);
		}
		flashDevice->actWriteBlock = nextBlock(flashDevice, FALSE);	
		// Cleaner		
		if (flashDevice->freeBlocks < SPARE_BLOCKS ){
			// Clean
			ret = garbageCollector(flashDevice, FALSE); 
			//flashDevice->actWriteBlock = nextBlock(flashDevice, FALSE);
			return ret;
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

uint16_t nextBlock(flash_t *flashDevice, uint8_t prio){
	ListElem_t *element;
	if (DEBUG_LEVEL < 2) { printf("Waehle naechsten Block\n"); }
	//Auswahl eines komplett leeren Blocks
	//nehme kältesten, beschreibbaren Block aus coldPool			
	element = findNextBlock(flashDevice, flashDevice->coldPool, (FALSE == prio));
	if (element != NULL) return element->blockNr;
	//nehme kältesten beschreibbaren Block aus neutralPool
	element = findNextBlock(flashDevice, flashDevice->neutralPool, (FALSE == prio));
	if (element != NULL) return element->blockNr;
	//nehme kältesten, beschreibbaren Block aus hotPool
	element = findNextBlock(flashDevice, flashDevice->hotPool, (FALSE == prio));
	if (element != NULL) return element->blockNr;
	//nehme jetzt einen nichtleeren und nichtvollen Block
	element = findNextBlock(flashDevice, flashDevice->coldPool, (FALSE != prio));
	if (element != NULL) return element->blockNr;
	//nehme kältesten beschreibbaren Block aus neutralPool
	element = findNextBlock(flashDevice, flashDevice->neutralPool, (FALSE != prio));
	if (element != NULL) return element->blockNr;
	//nehme kältesten, beschreibbaren Block aus hotPool
	element = findNextBlock(flashDevice, flashDevice->hotPool, (FALSE != prio));
	if (element != NULL) return element->blockNr;
	//nehme jetzt einen nichtleeren und nichtvollen Block
	if (DEBUG_LEVEL < 5) {
		printf("Keinen freien Block in Pools gefunden.\n");
		printerr(flashDevice);
	}
	return flashDevice->actWriteBlock; // Aktuellen Schreibblock zurück geben;
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
	uint32_t blockCount = 0;
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
		// Mapping Tabellen mit Null initialisieren
		for (k = 0; k < getMappingTableSize(); k++){
			flashDevice->mappingTable[k] = 0;
		}

		for (k = 0; k < getMappingTableSize(); k++){
			flashDevice->mappingTableRev[k] = getMappingTableSize();
		}
		for (k = 0; k < mappingTableCount; k++){
			temp = convert8To32(state[i++], state[i++], state[i++], state[i++]);
			//flashDevice->mappingTable[k] = temp;
			setMapT(flashDevice, k / getBlockSegmentCount(), k % getBlockSegmentCount(), temp);
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
		// befülle neutralPool
		for (i = 0; i < FL_getBlockCount(); i++){
			addBlock(flashDevice->neutralPool, i);
		}
	}

	if (DEBUG_LEVEL < 5) printf("SSD initialisiert!\n");
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

	//Anzahl an 512 Blöcken für Speichern
	i = size / 512;
	if (k % 512 != 0){
		i++;
	}

	//Speichere auf Hardware
	if (FL_saveState((uint8_t)i, state) == TRUE){
		if (DEBUG_LEVEL < 5) printf("flash_t-Abbild erfolgreich gespeichert\n");
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
	uint8_t ret;
	if (flashDevice == NULL)
		return FALSE;
	if (DEBUG_LEVEL < 2) { printf("Lesezugriff %i\n", index); }
	if (index > (FL_getBlockCount() - (uint16_t)SPARE_BLOCKS)  * getBlockSegmentCount()){
		if (DEBUG_LEVEL < 5) printf("Logischer Block nicht mehr im Wertebereich des Datentraegers. 0 bis %i logische Bloecke adressierbar.\n", getLogBlockCount());
		return FALSE;
	}
	//Index immer um 1 erhöht, da index == 0 für andere Zwecke verwendet wird.
	i = mapping(flashDevice, index + 1); // Mapping
	ret =  readBlockIntern(flashDevice, (uint16_t)i / getBlockSegmentCount(), (uint16_t)(i % getBlockSegmentCount()) / FL_getPagesPerBlock(), (uint16_t)((i % getBlockSegmentCount()) % FL_getPagesPerBlock()), data); // Blocksegment auslesen
	return ret;
}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	if (flashDevice == NULL) return FALSE;
	if (DEBUG_LEVEL < 2) { printf("Schreibzugriff %i\n", index); }
	if (index > (FL_getBlockCount() - (uint16_t)SPARE_BLOCKS)  *getBlockSegmentCount()){
		if (DEBUG_LEVEL < 5) printf("Logischer Block nicht mehr im Wertebereich des Datentraegers. 0 bis %i logische Bloecke adressierbar.\n", getLogBlockCount());
		return FALSE;
	}	
	if( flashDevice->freeBlocks == 0){
		if (DEBUG_LEVEL < 5) printf("Anzahl an BadBlocks = %i. Datentraeger nicht mehr beschreibbar.\n", flashDevice->badBlockCounter);
		return FALSE;
	}
	//Index immer um 1 erhöht, da index == 0 für andere Zwecke verwendet wird.
	return writeBlockIntern(flashDevice, index + 1, data);
}


// Getter für "Konstanten" Implementation
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

void printLogicalToHW(flash_t *flashDevice, uint32_t index){
	uint32_t hw_addr = mapping(flashDevice, index);
	if (DEBUG_LEVEL < 5) printf("(%i) B: % i S: %i\n", index, hw_addr / getBlockSegmentCount(), hw_addr % getBlockSegmentCount());
}

void printblock(flash_t *flashDevice, uint16_t block, uint32_t segment){
	uint32_t j;
	uint8_t data[LOGICAL_BLOCK_DATASIZE];
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
			readBlockIntern(flashDevice, block, segment / FL_getPagesPerBlock(), segment % FL_getPagesPerBlock(), data);

			printf("Segment %02i: Table: %03i ( %04i ) - %i %c %c (%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c)\n",
				j, getMapT(flashDevice, block, j), mapping(flashDevice, getMapT(flashDevice, block, j)), segmentStatus(flashDevice, block, j), error, marker,
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
				data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
		}
		printf("Segment: empty (0) / assigned (1) / invalid (2)\n");
}


void printerr(flash_t *flashDevice){
	uint32_t invCo = 0, del = 0, segment, calcLevel = 0, j, addr, invaddr;
	uint16_t block, i;
	char userInput, unMountErr = 'n', check;
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
			printf("   %02i |   %i    |         %02i        |     %i  \n", i
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
			printf("Es folgt die umgekehrte Detail Mapping Table\n");
			for (j = 1; j < getMappingTableSize() - (SPARE_BLOCKS * LOGICAL_BLOCK_DATASIZE) + 1; j += 2){
				addr = mapping(flashDevice, j);
				invaddr = getMapT(flashDevice, addr / getBlockSegmentCount(), addr % getBlockSegmentCount());
				/*	check = '*';
					if (j != invaddr) {
					check = '!';
					}*/
				printf("Tbl: %03i = %03i ->(%03i) %02i %02i", j, invaddr
					, addr, addr / getBlockSegmentCount(), addr % getBlockSegmentCount());
				// Teil 2
				addr = mapping(flashDevice, j + 1);
				invaddr = getMapT(flashDevice, addr / getBlockSegmentCount(), addr % getBlockSegmentCount());
				printf("Tbl: %03i = %03i ->(%03i) %02i %02i\n", j + 1, invaddr
					, addr, addr / getBlockSegmentCount(), addr % getBlockSegmentCount());
			}
			getchar();
			for (i = 0; i < FL_getBlockCount(); i++)
			{
				// Block für Block
				printblock(flashDevice, i, segment);
				getchar();
			}
		}
		printerr(flashDevice);
	}
}
