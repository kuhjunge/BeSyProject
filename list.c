#include "ftl.h"
#include "list.h"

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
		getFirstElement(list);
	}
	free(list);
}

void addBlock(List_t* list, uint32_t blockNr){
	ListElem_t* elem = (ListElem_t*)malloc(sizeof(ListElem_t));//Erzeuge neues Element
	ListElem_t* posElem;// Pointer auf aktuelles Element	

	//Fehlerfall
	if(blockNr < 0){		
		free(elem);
		return;
	}

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
		list->blockCounter++;

		//Einfügen am Anfang
		posElem = list->first;
		if( list->blockArray[posElem->blockNr].deleteCounter > list->blockArray[blockNr].deleteCounter ){
			elem->next = list->first;
			list->first->prev = elem;
			list->first = elem;
			return;
		}		

		//Einfügen in Mitte
		for(posElem = posElem->next; posElem != NULL; posElem = posElem->next){
			if( list->blockArray[posElem->blockNr].deleteCounter > list->blockArray[blockNr].deleteCounter ){
				elem->next = posElem;
				elem->prev = posElem->prev;
				posElem->prev = elem;
				return;
			}
		}

		//Einfügen am Ende
		elem->prev = list->last;
		list->last->next = elem;
		list->last = elem;		
	}
}

ListElem_t* getFirstElement(List_t* list){	
	ListElem_t* elem;

	if(list->blockCounter <= 0){
		return NULL;
	}
	if(list->blockCounter == 1){
		elem = list->first;
		list->first = NULL;
		list->last = NULL;
		list->AVG = 0;
		list->blockCounter = 0;
		return elem;
	}

	elem = list->first;
	list->first = elem->next;
	list->first->prev = NULL;
	elem->next = NULL;
	list->blockCounter--;

	return elem;	
}

ListElem_t* getLastElement(List_t* list){	
	ListElem_t* elem;

	if(list->blockCounter <= 0){
		return NULL;
	}
	if(list->blockCounter == 1){
		elem = list->first;
		list->first = NULL;
		list->last = NULL;
		list->AVG = 0;
		list->blockCounter = 0;
		return elem;
	}

	elem = list->last;
	list->last = elem->prev;
	list->last->next = NULL;
	elem->prev = NULL;
	list->blockCounter--;

	return elem;	
}

void recalculationAVG(List_t* list){	
	list->AVG += (double) 1 / list->blockCounter;
}

uint8_t isElementOfList(List_t* list, uint32_t blockNr){
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

ListElem_t* showFirstElement(List_t* list){
	return list->first;
}

ListElem_t* showLastElement(List_t* list){
	return list->last;
}

uint32_t listLength(List_t* list){
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
	printf("AVG: %f, AnzahlElemente: %i\n", list->AVG, list->blockCounter);
}

void calculateAVG(List_t* list, uint32_t deleteCounter, uint8_t plus){	
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

uint8_t delBlock(List_t* list, uint32_t blockNr){
	ListElem_t* element;
	ListElem_t* next;
	ListElem_t* prev;

	if(list->blockCounter == 0){
		return FALSE;
	}

	if(list->blockCounter == 1){
		if(list->first->blockNr == blockNr){
			list->AVG = 0;
			list->first = NULL;
			list->last = NULL;
			list->blockCounter = 0;
			return TRUE;
		}
		else{
			return FALSE;
		}
	}

	for(element = list->first; element != NULL; element = element->next){
		if(element->blockNr == blockNr){
			next = element->next;
			prev = element->prev;
						
			if(prev == NULL){
				list->first = next;
				list->first->prev = NULL;
			}
			else{
				prev->next = next;
			}
			if( next == NULL){
				list->last = prev;
				list->last->next = NULL;
			}
			else{
				next->prev = prev;
			}

			free(element);
			list->blockCounter--;
			return TRUE;
		}
	}
	return FALSE;
}