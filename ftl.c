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
 *	Schreibt Daten in aktBlock und sucht, falls der voll ist, einen neuen Block zum beschreiben
 *	data die zu schreibenden Daten
 *	der logische Index, der gesetzt werden soll
 */
uint8_t nextBlock(flash_t *flashDevice);

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

/*
uint8_t swapBlock(flash_t* flashDevice, uint32_t readBlock, uint32_t writeblock){
	uint32_t j = 0;
	uint32_t count;
	uint32_t adress;
	uint8_t temp[LOGICAL_BLOCK_DATASIZE];
	for (j = 0; j < BLOCKSEGMENTS; j++){
		if (segmentStatus(flashDevice, readBlock, j) == assigned){
			adress = getMapT(flashDevice, readBlock, j);
			readBlockIntern(flashDevice, readBlock, j / FL_getPagesPerBlock(), j % FL_getPagesPerBlock(), temp);
			if (flashDevice->freeBlocks > 0){
				//writeBlockIntern(flashDevice, adress, temp, TRUE, readBlock, j);
				moveBlockIntern(flashDevice, writeBlock, readBlock, j);
			}
			else {
				printf("Swapper Zugriffsfehler!\n");
				printerr(flashDevice);
			}
		}
	}
	return TRUE;
} */

uint8_t writeSegmentToBlock(flash_t* flashDevice, uint8_t* segment, uint32_t block){
	uint16_t count = 0;
	uint32_t page = flashDevice->blockArray[block].writePos / FL_getPagesPerBlock();
	uint32_t bp_index = flashDevice->blockArray[block].writePos % FL_getPagesPerBlock();

	//Abfrage, ob in den zu schreibender Block beschreibbar ist
	if (flashDevice->blockArray[block].status != ready){
		return FALSE;
	}

	count = FL_writeData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, segment);
	// Pr�fen ob wirklich entsprechende Daten geschrieben wurden	
	if (count != LOGICAL_BLOCK_DATASIZE){
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}

	flashDevice->blockArray[block].writePos++;
	// Block ist komplett gef�llt	
	if (flashDevice->blockArray[block].writePos == BLOCKSEGMENTS - 1){
		flashDevice->blockArray[block].status = used;
		flashDevice->freeBlocks--;
	}
	return TRUE;
}

void grouping(flash_t* flashDevice){
	ListElem_t* position = NULL;	

	//check condition 1 und 2 => grouping
		//f�r neutral pool obere Grenze
		position = showLastElement(flashDevice->neutralPool);
		while (position != NULL && flashDevice->blockArray[position->blockNr].deleteCounter > flashDevice->AVG + THETA){						
			if( showLastElement(flashDevice->neutralPool) != NULL ){				
				addBlock(flashDevice->hotPool, getLastBlock(flashDevice->neutralPool));
				calculateAVG(flashDevice->neutralPool, flashDevice->blockArray[position->blockNr].deleteCounter, FALSE);
				calculateAVG(flashDevice->hotPool, flashDevice->blockArray[position->blockNr].deleteCounter, TRUE);			
			}			
			position = showLastElement(flashDevice->neutralPool);			
		}
		//f�r neutral pool untere Grenze
		position = showFirstElement(flashDevice->neutralPool);
		while(position != NULL && flashDevice->blockArray[position->blockNr].deleteCounter < flashDevice->AVG - THETA){									
			if( showFirstElement(flashDevice->neutralPool) != NULL ){				
				addBlock(flashDevice->coldPool, getFirstBlock(flashDevice->neutralPool));			
				calculateAVG(flashDevice->neutralPool, flashDevice->blockArray[position->blockNr].deleteCounter, FALSE);
				calculateAVG(flashDevice->coldPool, flashDevice->blockArray[position->blockNr].deleteCounter, TRUE);
			}			
			position = showFirstElement(flashDevice->neutralPool);
		}
		//f�r cold pool obere Grenze
		position = showLastElement(flashDevice->coldPool);
		while (position != NULL && flashDevice->blockArray[position->blockNr].deleteCounter > flashDevice->AVG - THETA){						
			if( showLastElement(flashDevice->coldPool) != NULL ){				
				addBlock(flashDevice->neutralPool, getLastBlock(flashDevice->coldPool));	
				calculateAVG(flashDevice->coldPool, flashDevice->blockArray[position->blockNr].deleteCounter, FALSE);
				calculateAVG(flashDevice->neutralPool, flashDevice->blockArray[position->blockNr].deleteCounter, TRUE);
			}
			position = showLastElement(flashDevice->coldPool);
		}
		//f�r hot pool untere Grenze
		position = showFirstElement(flashDevice->hotPool);
		while (position != NULL && flashDevice->blockArray[position->blockNr].deleteCounter < flashDevice->AVG + THETA){			
			if( showFirstElement(flashDevice->hotPool) != NULL ) {
				addBlock(flashDevice->neutralPool,  getFirstBlock(flashDevice->hotPool));
				calculateAVG(flashDevice->hotPool, flashDevice->blockArray[position->blockNr].deleteCounter, FALSE);
				calculateAVG(flashDevice->neutralPool, flashDevice->blockArray[position->blockNr].deleteCounter, TRUE);
			}
			position = showFirstElement(flashDevice->hotPool);
		}					
}

void neutralisation(flash_t* flashDevice, List_t* pool, uint32_t deletedBlock, uint8_t hotNeutralisation){
	uint16_t p, i;
	uint8_t data[BLOCKSEGMENTS][LOGICAL_BLOCK_DATASIZE];
	uint32_t mappingData[BLOCKSEGMENTS];
	uint16_t data_position = 0;
	uint8_t tempSegment[LOGICAL_BLOCK_DATASIZE];
	ListElem_t* position;
	uint32_t tempAllocData = 0;
	uint32_t tempBlock = 0;

		//step 1 kopiere valide Segmente in tempBlock			
			for (p = 0; p < FL_getPagesPerBlock(); p++){
				for (i = 0; i < FL_getPageDataSize() / LOGICAL_BLOCK_DATASIZE; i++){
					if (segmentStatus(flashDevice, deletedBlock, (FL_getPagesPerBlock()*p) + i) == assigned){
						readBlockIntern(flashDevice, deletedBlock, p, i, data[(FL_getPagesPerBlock()*p) + i]);
						data_position++;
					}
				}
			}
			//step 2 l�sche Block und setze Z�hler hoch
			cleanBlock(flashDevice, deletedBlock);
			//step 3 kopiere alle validen Segmente aus Min(neutralPool)/Max(neutralPool) in deletedBlock			
			for (p = 0; p < FL_getPagesPerBlock(); p++){
				for (i = 0; i < FL_getPageDataSize() / LOGICAL_BLOCK_DATASIZE; i++){
					if (segmentStatus(flashDevice, deletedBlock, (FL_getPagesPerBlock()*p) + i) == assigned){						
						if(hotNeutralisation == TRUE){
							readBlockIntern(flashDevice, showFirstElement(flashDevice->neutralPool)->blockNr, p, i, tempSegment);						
						}
						else{
							readBlockIntern(flashDevice, showLastElement(flashDevice->neutralPool)->blockNr, p, i, tempSegment);						
						}
						writeSegmentToBlock(flashDevice, tempSegment, deletedBlock);						
					}
				}
			}
			//step 4 kopiere aus tempBlock(data) in MAX(neutralPool)/Min(neutralPool)
			if(hotNeutralisation == TRUE){
				position = showLastElement(flashDevice->neutralPool);
			}
			else{
				position = showFirstElement(flashDevice->neutralPool);
			}
			for (i = 0; i < data_position; i++){				
				p = writeSegmentToBlock(flashDevice, data[i], position->blockNr);
				// falls das Segment nicht geschrieben wurde
				if (p == FALSE){
					if(hotNeutralisation == TRUE){
						position = getPrevElement(position);
					}
					else{
						position = getNextElement(position);
					}
					if (position == NULL){
						printf("Fehler im WearLevelingAlgorithmus in step 4. Kein beschreibbarer Block mehr im neutralPool vorhanden\n");
						return;
					}
				}
			}
			//step 5 update der Adressen; deletedBlock <-> showFirstElement(flashDevice->neutralPool)/showLastElement(flashDevice->neutralPool)
			if(hotNeutralisation == TRUE){
				tempBlock = showFirstElement(flashDevice->neutralPool)->blockNr;
			}
			else{
				tempBlock = showLastElement(flashDevice->neutralPool)->blockNr;
			}
			for (i = 0; i < BLOCKSEGMENTS; i++){
				tempAllocData = getMapT(flashDevice, deletedBlock, i);
				setMapT(flashDevice, deletedBlock, i, getMapT(flashDevice, tempBlock, i));
				setMapT(flashDevice, tempBlock, i, tempAllocData);
			}			
			// Berechen AVGs neu nach Neutralisation
			pool->AVG += (double) (flashDevice->blockArray[deletedBlock].deleteCounter - flashDevice->blockArray[tempBlock].deleteCounter) / pool->blockCounter;
			flashDevice->AVG += (double) (flashDevice->blockArray[tempBlock].deleteCounter - flashDevice->blockArray[deletedBlock].deleteCounter) / FL_getBlockCount();
			
			//�bernehme neu in Pool, damit Position richtig gesetzt wird			
		/*	delBlock(pool, deletedBlock);			
			addBlock(pool, deletedBlock);			*/
			
}

int countListElements(List_t* list){
	int i = 0;
	ListElem_t* position = list->first;

	while(position != NULL){
		i++;
		position = position->next;
	}

	return i;
}

void deleteBlock(flash_t* flashDevice, uint32_t deletedBlock, uint16_t inPool){
	uint16_t p, i;
	uint8_t data[BLOCKSEGMENTS][LOGICAL_BLOCK_DATASIZE];
	uint32_t mappingData[BLOCKSEGMENTS];
	uint16_t data_position = 0;

	//L�sche Block
	// kopiere Inhalt zwischen, l�sche deletedBlock und schreibe Inhalt neu
			for (p = 0; p < FL_getPagesPerBlock(); p++){
				for (i = 0; i < FL_getPageDataSize() / LOGICAL_BLOCK_DATASIZE; i++){
					if (segmentStatus(flashDevice, deletedBlock, (FL_getPagesPerBlock()*p) + i) == assigned){
						readBlockIntern(flashDevice, deletedBlock, p, i, data[data_position]);
						mappingData[data_position] = getMapT(flashDevice, deletedBlock, (p * FL_getPagesPerBlock()) + i);
						setMapT(flashDevice, deletedBlock, (p * FL_getPagesPerBlock()) + i, 0);//TODO �berfl�ssig, da eigentlich in writeBlockIntern geschieht,oder?
						data_position++;
					}
				}
			}
			cleanBlock(flashDevice, deletedBlock);
			//�bernehme neu in Pool, damit Position in Pool richtig gesetzt wird
		/*	if(inPool == 1){				
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
			}*/

			//schreibe Daten zur�ck
			for (i = 0; i < data_position; i++){				
				writeBlockIntern(flashDevice, mappingData[i], data[i]);
			}			
}

void wearLeveling(flash_t* flashDevice, uint32_t deletedBlock){		

	printf("\n\ncoldPool\n");
	printList(flashDevice->coldPool);
	printf("neutralPool\n");
	printList(flashDevice->neutralPool);
	printf("hotPool\n");
	printList(flashDevice->hotPool);//TODO: DEBUG rausnehmen
	printf("\n");

	//Average Recalculation gesamt
	flashDevice->AVG += (double)1 / FL_getBlockCount();

	//erase operation in neutral pool?	
	if (isElementOfList(flashDevice->neutralPool, deletedBlock) == TRUE){
		//Average Recalculation Neutral
		recalculationAVG(flashDevice->neutralPool);
	
		//l�sche deletedBlock
		deleteBlock(flashDevice, deletedBlock, 1);

		//Grouping			
		if( flashDevice->blockArray[deletedBlock].deleteCounter > flashDevice->AVG + THETA ){
			addBlock(flashDevice->hotPool, getLastBlock(flashDevice->neutralPool));
		}
		if( flashDevice->blockArray[deletedBlock].deleteCounter < flashDevice->AVG - THETA ){
			addBlock(flashDevice->coldPool, getFirstBlock(flashDevice->neutralPool));
		}
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
	uint32_t deleteCount = 0;
	uint32_t level = flashDevice->invalidCounter / (FL_getBlockCount() - flashDevice->freeBlocks); //Anzahl der zu bereinigen Blocks, dynamisch berechnet
	
	// Solange noch nicht alle Bloecke durchlaufen wurden oder genug Bloecke gereinigt wurden				
	while (deleteCount < SPARE_BLOCKS  && k < FL_getBlockCount () ){ 		
		if( i >= FL_getBlockCount() - 1){
			i = 0;
		}
		else{
			i++;
		}

		// Wenn Block �ber Schwellwert liegt und benutzt wird				
		if (flashDevice->blockArray[i].invalidCounter >= level && flashDevice->blockArray[i].status != badBlock){ 							
			deleteCount++;
				printf("\n\n\ncoldPool\n");
	printList(flashDevice->coldPool);
	printf("neutralPool\n");
	printList(flashDevice->neutralPool);
	printf("hotPool\n.......\n");
	printList(flashDevice->hotPool);//TODO: DEBUG rausnehmen
			wearLeveling(flashDevice, i);					
				printf("coldPool\n");
	printList(flashDevice->coldPool);
	printf("neutralPool\n");
	printList(flashDevice->neutralPool);
	printf("hotPool\n\n________________________________\n");
	printList(flashDevice->hotPool);//TODO: DEBUG rausnehmen
		}
		k++;		
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
	/*
	// Unn�tig, da in einem zum cleanen markierten Block keine Daten mehr geschrieben sein d�rfen!!
	for (i = 0; i < BLOCKSEGMENTS; i++){
		setMapT(flashDevice, block, i, 0);
	}*/

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


/*uint8_t moveBlockIntern(flash_t *flashDevice, uint32_t block, uint16_t mappingBlockSource, uint16_t mappingSegmentSource)
{
	uint32_t count;
	uint32_t page = flashDevice->blockArray[block].writePos / FL_getPagesPerBlock();
	uint32_t bp_index = flashDevice->blockArray[block].writePos % FL_getPagesPerBlock();
	uint8_t* data;

	if (flashDevice->blockArray[block].writePos >= BLOCKSEGMENTS - 1){ // position weiterz�hlen wenn innerhalb des selben blockes
		// Diese Funktion w�hlt keinen neuen Block selbst�ndig aus
		printf("Fehler! aktueller Block voll! \n");
		return FALSE;
	}
	readBlockIntern(flashDevice, mappingBlockSource, mappingSegmentSource / FL_getPagesPerBlock(), mappingSegmentSource% FL_getPagesPerBlock(), data);
	count = FL_writeData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Daten beschreiben
	if (count != LOGICAL_BLOCK_DATASIZE){ // Pr�fen ob wirklich entsprechende Daten geschrieben wurden		}
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}
	setMapT(flashDevice, block, flashDevice->blockArray[block].writePos, getMapT(flashDevice, mappingBlockSource, mappingSegmentSource)); // Mapping Eintrag tauschen
	invalidationOfOldIndex(flashDevice, mappingBlockSource, mappingSegmentSource); // Alten Eintrag in Mapping Table und Block invalidieren
	flashDevice->blockArray[block].writePos++;
	// Auswahl des n�chsten Schreibortes


	return TRUE;
// flashDevice->actWriteBlock = flashDevice->blockArray[element->blockNr].writePos;
}*//*
uint8_t writeSegmentToBlock(flash_t* flashDevice, uint8_t* segment, uint32_t block){
	uint16_t count = 0;
	uint32_t page = flashDevice->blockArray[block].writePos / FL_getPagesPerBlock();
	uint32_t bp_index = flashDevice->blockArray[block].writePos % FL_getPagesPerBlock();

	//Abfrage, ob in den zu schreibender Block beschreibbar ist
	if (flashDevice->blockArray[block].status != ready){
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
	if (flashDevice->blockArray[block].writePos == BLOCKSEGMENTS - 1){
		flashDevice->blockArray[block].status = used;
		flashDevice->freeBlocks--;
	}
	return TRUE;
}*/


uint8_t writeBlockIntern(flash_t *flashDevice, uint32_t index, uint8_t *data){

	uint32_t block = flashDevice->actWriteBlock;
	uint32_t page = flashDevice->blockArray[flashDevice->actWriteBlock].writePos / FL_getPagesPerBlock();
	uint32_t bp_index = flashDevice->blockArray[flashDevice->actWriteBlock].writePos % FL_getPagesPerBlock();
	uint32_t count;
	uint32_t index_old;
	if (index < 1){ // Fehlerhafter Index wurde �bergeben
		printf("Fehlerhafter Index wurde uebergeben. Index darf nicht < 1  sein!\n");
		printerr(flashDevice);
		return FALSE;
	}

	// Der Block auf dem geschrieben wird, ist nicht beschreibbar
	if (flashDevice->blockArray[block].status != ready){ 
			printf("Fehler beim Schreiben des Datensatzes! Fehlerhafter Block Zugriff!\n");			
			printerr(flashDevice);
			return FALSE;
	}
	count = FL_writeData(block, page, bp_index * LOGICAL_BLOCK_DATASIZE, LOGICAL_BLOCK_DATASIZE, data); // Daten beschreiben
	if (count != LOGICAL_BLOCK_DATASIZE){ // Pr�fen ob wirklich entsprechende Daten geschrieben wurden		
		printf("Fehler beim Schreiben des Datensatzes! %d von %d byte geschrieben\n", count, LOGICAL_BLOCK_DATASIZE);
		printerr(flashDevice);
		return FALSE;
	}
	if (index > 0){
		index_old = mapping(flashDevice, index);
		if (index_old < MAPPING_TABLE_SIZE){
			invalidationOfOldIndex(flashDevice, (uint16_t)index_old / BLOCKSEGMENTS, (uint16_t)index_old % BLOCKSEGMENTS); // Alten Eintrag in Mapping Table und Block invalidieren
		}
	}
	/*else {
		index = getMapT(flashDevice, block_index, segment_index); // alten Eintrag zwischenspeichern
		invalidationOfOldIndex(flashDevice, block_index, segment_index); // Alten Eintrag in Mapping Table und Block invalidieren
		}*/
	//printf("Index! %i (%i/%i/%i) \n" , index, block, page, bp_index);//TODO DEBUG-Ausgabe rausnehmen
	setMapT(flashDevice, block, (page * FL_getPagesPerBlock()) + bp_index, index); // Setze Mapeintrag
	// Auswahl des n�chsten Schreibortes
	if (flashDevice->blockArray[flashDevice->actWriteBlock].writePos < BLOCKSEGMENTS - 1){ // position weiterz�hlen wenn innerhalb des selben blockes
		flashDevice->blockArray[flashDevice->actWriteBlock].writePos++;
	}
	else { // oder einen neuen Block ausw�hlen
		// alten Block abschlie�en
		flashDevice->blockArray[flashDevice->actWriteBlock].status = used;
		flashDevice->freeBlocks--;
		nextBlock(flashDevice); // Freien Block finden und nutzen
		if (flashDevice->freeBlocks <= SPARE_BLOCKS + START_CLEANING){ // Cleaner
			garbageCollector(flashDevice); // Clean
		}
		if (flashDevice->freeBlocks <= SPARE_BLOCKS ) { // Fehler falls zu wenig freie Bl�cke da sind
			printf("%i flashDevice->freeBlocks ;",flashDevice->freeBlocks);
			printf("Fehler! Festplatte zu voll! [ueber %i byte geschrieben]\n", (BLOCKSEGMENTS * (FL_getBlockCount() - SPARE_BLOCKS) * LOGICAL_BLOCK_DATASIZE) - 1);
			printerr(flashDevice);
			return FALSE;
		}
	}
	// �berpr�fung auf fehler 
	//TODO DEBUG-Info rausnehmen
	/*readBlock(flashDevice, index, data);
	if (!(index == getMapT(flashDevice, block, (page * FL_getPagesPerBlock()) + bp_index)) || (data[0] != 'A')){		
		printf("index=%i == %i=getMapT();",data[0], index, getMapT(flashDevice, block, (page * FL_getPagesPerBlock()) + bp_index));
		printf("Fehler bei Index! %i (%i/%i/%i) \n" , index, block, page, bp_index);
		return FALSE;
	}*/

	return TRUE;
}

uint8_t nextBlock(flash_t *flashDevice){
	ListElem_t* element;	
	uint32_t i;
	
	//nehme k�ltesten, beschreibbaren Block aus coldPool			
	for (element = showFirstElement(flashDevice->coldPool); element != NULL; element = getNextElement(element)){
		if (flashDevice->blockArray[element->blockNr].status == ready){
			flashDevice->actWriteBlock = element->blockNr;
			return TRUE;
		}
	}
	//nehme k�ltesten beschreibbaren Block aus neutralPool
	for (element = showFirstElement(flashDevice->neutralPool); element != NULL; element = getNextElement(element)){		
		if (flashDevice->blockArray[element->blockNr].status == ready){
			flashDevice->actWriteBlock = element->blockNr;			
			return TRUE;
		}
	}
	//nehme k�ltesten, beschreibbaren Block aus hotPool
	for (element = showFirstElement(flashDevice->hotPool); element != NULL; element = getNextElement(element)){
		if (flashDevice->blockArray[element->blockNr].status == ready){
			flashDevice->actWriteBlock = element->blockNr;
			return TRUE;
		}	
	}
	
	// Fehlerfall, kein beschreibbarer Block gefunden
	printf("Fehler, es wurde kein beschreibbarer Block in einem Pool gefunden!\n");
	printerr(flashDevice);
	return FALSE;
	
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
	i = mapping(flashDevice, index); // Mapping
	return readBlockIntern(flashDevice, i / BLOCKSEGMENTS, (i % BLOCKSEGMENTS) / FL_getPagesPerBlock(), ((i % BLOCKSEGMENTS) % FL_getPagesPerBlock()), data); // Blocksegment auslesen

}

uint8_t writeBlock(flash_t *flashDevice, uint32_t index, uint8_t *data){
	if (flashDevice == NULL) return FALSE;
	return writeBlockIntern(flashDevice, index, data);
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
	}
}
