#include "list.h"

// Lokale Funktionsimplementation List
////////////////////////////////////////////////////////////////////
List_t* initList(Block_t** blockArray){
	List_t* list = (List_t*)malloc(sizeof(List_t*));
	list->AVG = 0;
	list->blockArray = blockArray;
	list->blockCounter = 0;
	list->first = NULL;
	list->last = NULL;
	return list;
}

void freeList(List_t* list);

void addBlock(List_t* list, uint16_t blockNr){
	ListElem_t* elem = (ListElem_t*)malloc(sizeof(ListElem_t));//Erzeuge neues Element
	ListElem_t* posElem;// Pointer auf aktuelles Element

	if(list->blockCounter == 0){//erster Block wird hinzugefügt
		elem->blockNr = blockNr;
		elem->next = NULL;
		elem->prev = NULL;
		list->first = elem;
		list->last = elem;
		list->blockCounter++;
		list->AVG = list->blockArray[blockNr]->deleteCounter;
	}
	else{// es ist min. ein Block schon in Liste enthalten
		elem->blockNr = blockNr;
		posElem = list->first;
		while(list->blockArray[posElem->blockNr] > list->blockArray[blockNr] && posElem != list->last){
			posElem = posElem->next;
		}
		if(posElem == list->first){//wenn vor erstes Element eingefügt werden soll
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
		if(posElem == list->last){//wenn letztes Element ersetzt wurde, ändere dies auch in List
			list->last = elem;
		}
		//Berechne AVG neu
		list->AVG = list->AVG * list->blockCounter + list->blockArray[blockNr]->deleteCounter;
		list->blockCounter++;
		list->AVG = list->AVG / list->blockCounter;
	}
}

uint16_t getFirstBlock(List_t* list){
	ListElem_t* elem = list->first;
	uint16_t blockNr = 0;

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
		return NULL;
	}
	if(list->blockCounter >= 2){
		list->blockCounter--;
	}
	//Berechne AVG neu
	list->AVG = list->AVG * (list->blockCounter + 1) - list->blockArray[elem->blockNr]->deleteCounter;	
	list->AVG = list->AVG / list->blockCounter;

	//hänge Pointer um
	blockNr = elem->blockNr;
	list->first = elem->next;
	list->first->prev = NULL;
	free(elem);
	return blockNr;	
}

uint16_t getLastBlock(List_t* list){
	ListElem_t* elem = list->last;
	uint16_t blockNr = 0;

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
		return NULL;
	}
	if(list->blockCounter >= 2){
		list->blockCounter--;
	}
	//Berechne AVG neu
	list->AVG = list->AVG * (list->blockCounter + 1) - list->blockArray[elem->blockNr]->deleteCounter;	
	list->AVG = list->AVG / list->blockCounter;

	//hänge Pointer um
	blockNr = elem->blockNr;
	list->last = elem->prev;
	list->last->next = NULL;
	free(elem);
	return blockNr;
}
