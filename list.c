#include "ftl.h"

// Lokale Funktionsimplementation List
////////////////////////////////////////////////////////////////////
List_t* initList(Block_t* blockArray){
	List_t* list = (List_t*)malloc(sizeof(List_t));
	list->AVG = 0;
	list->blockArray = blockArray;
	list->blockCounter = 0;
	list->first = NULL;
	list->last = NULL;
	return list;
}

void freeList(List_t* list){
	while(list->blockCounter > 0){
		getFirstBlock(list);
	}
	free(list);
}

void addBlock(List_t* list, int32_t blockNr){
	ListElem_t* elem = (ListElem_t*)malloc(sizeof(ListElem_t));//Erzeuge neues Element
	ListElem_t* posElem;// Pointer auf aktuelles Element

	if(list->blockCounter == 0){//erster Block wird hinzugefügt
		elem->blockNr = blockNr;
		elem->next = NULL;
		elem->prev = NULL;
		list->first = elem;
		list->last = elem;
		list->blockCounter++;
		list->AVG = 0;
	}
	else{// es ist min. ein Block schon in Liste enthalten
		elem->blockNr = blockNr;
		elem->next = NULL;
		elem->prev = NULL;
		posElem = list->first;
		while(list->blockArray[posElem->blockNr].deleteCounter > list->blockArray[blockNr].deleteCounter && posElem->next != NULL){
			posElem = posElem->next;
		}
		if(posElem->prev == NULL){//wenn vor erstes Element eingefügt werden soll
			elem->next = posElem;
			elem->prev = NULL;
			posElem->prev = elem;
			list->first = elem;
		}
		else{			
			elem->next = posElem->next;
			posElem->next = elem;
			elem->prev = posElem;
		}
		if(posElem->next == NULL){//wenn letztes Element ersetzt wurde, ändere dies auch in List
			list->last = elem;
		}

		list->blockCounter++;
		//Berechne AVG neu
	/*	list->AVG = list->AVG * list->blockCounter + list->blockArray[blockNr].deleteCounter;		
		list->AVG = list->AVG / list->blockCounter;*/
	}
}

int32_t getFirstBlock(List_t* list){
	ListElem_t* elem = list->first;
	int32_t blockNr = 0;

	//Sonderfall nur noch 1 Block
	if(list->blockCounter == 1){
		list->first = NULL;
		list->last = NULL;
		list->blockCounter = 0;
		list->AVG = 0;
		blockNr = elem->blockNr;
		free(elem);
		return blockNr;
	}
	// list ist leer
	if(list->blockCounter == 0){				
		return -1;
	}
	if(list->blockCounter >= 2){
		list->blockCounter--;
	}
	//Berechne AVG neu
/*list->AVG = list->AVG * (list->blockCounter + 1) - list->blockArray[elem->blockNr].deleteCounter;	
	list->AVG = list->AVG / list->blockCounter;
	*/
	//hänge Pointer um
	blockNr = elem->blockNr;
	list->first = elem->next;
	list->first->prev = NULL;
	free(elem);
	return blockNr;	
}

int32_t getLastBlock(List_t* list){
	ListElem_t* elem = list->last;
	int32_t blockNr = 0;

	//Sonderfall nur noch 1 Block
	if(list->blockCounter == 1){
		list->first = NULL;
		list->last = NULL;
		list->blockCounter = 0;
		list->AVG = 0;
		blockNr = elem->blockNr;
		free(elem);
		return blockNr;
	}
	// list ist leer
	if(list->blockCounter == 0){				
		return -1;
	}
	if(list->blockCounter >= 2){
		list->blockCounter--;
	}
	//Berechne AVG neu
	/*list->AVG = list->AVG * (list->blockCounter + 1) - list->blockArray[elem->blockNr].deleteCounter;	
	list->AVG = list->AVG / list->blockCounter;*/

	//hänge Pointer um		
	blockNr = elem->blockNr;
	list->last = elem->prev;
	list->last->next = NULL;

	free(elem);
	return blockNr;
}

void recalculationAVG(List_t* list){	
	list->AVG += (double) 1 / list->blockCounter;
}

uint8_t isElementOfList(List_t* list, int32_t blockNr){
	ListElem_t* elem;	

	if(list->first == NULL){
		return FALSE;
	}

	elem = list->first;
	while(elem->next != NULL){
		if(elem->blockNr == blockNr){
			return TRUE;
		}
		elem = elem->next;
	}

	return FALSE;
}

int32_t showFirstBlock(List_t* list){
	if(list->first == NULL){
		return -1;
	}
	else{
		return list->first->blockNr;
	}
}

int32_t showLastBlock(List_t* list){
	if(list->last == NULL){
		return -1;
	}
	else{
		return list->last->blockNr;
	}
}

int32_t listLength(List_t* list){
	return list->blockCounter;
}


ListElem_t* getPrevElement(ListElem_t* elem){
	return elem->prev;	
}

ListElem_t* getNextElement(ListElem_t* elem){
	return elem->next;	
}

void printList(List_t* list){
	ListElem_t* elem;

	if(list->first == NULL)
		return;

	elem = list->first;
	while(elem != NULL){
		printf("%i, ", elem->blockNr);
		elem = elem->next;
	}
	printf("AVG: %f\n,", list->AVG);
}

void calculateAVG(List_t* list, int32_t deleteCounter, uint8_t plus){	
	double temp;

	if( plus == TRUE){
		 temp = list->AVG * (list->blockCounter-1);
		 temp = temp + deleteCounter;
		 list->AVG = (double)temp / list->blockCounter;
	}
	else{
		temp = list->AVG * (list->blockCounter+1);
		temp = temp - deleteCounter;
		list->AVG = (double)temp / list->blockCounter;
	}
}