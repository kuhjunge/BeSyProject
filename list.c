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
	ListElem_t* posElem;// Pointer auf aktuelles Element	
	ListElem_t* element = (ListElem_t*)malloc(sizeof(ListElem_t));

	//Fehlerfall
	if(blockNr < 0){				
		free(element);
		return;
	}
	//Abbruch, wenn element schon in Liste vorhanden ist
	if( isElementOfList(list, blockNr) == TRUE){
		free(element);
		return;
	}

	element->blockNr = blockNr;
	element->next = NULL;
	element->prev = NULL;
	
	if(list->blockCounter == 0){//erster Block wird hinzugefügt					
		list->first = element;
		list->last = element;
		list->blockCounter = 1;
		list->AVG = 0;
		return;
	}
	// es ist ein Block schon in Liste enthalten		
	if(list->blockCounter == 1){		
		posElem = list->first;
		if( list->blockArray[posElem->blockNr].deleteCounter >= list->blockArray[element->blockNr].deleteCounter ){
			element->next = list->first;
			list->first->prev = element;
			list->first = element;
		}		
		else{
			element->prev = list->last;
			list->last->next = element;
			list->last = element;
		}

		list->blockCounter++;
		return;
	}
	// es sind mehr als ein Block schon in Liste enthalten		
	else{				
		list->blockCounter++;
		//Einfügen am Anfang
		posElem = list->first;
		if( list->blockArray[posElem->blockNr].deleteCounter >= list->blockArray[element->blockNr].deleteCounter ){
			element->next = list->first;
			list->first->prev = element;
			list->first = element;
			return;
		}		

		//Einfügen in Mitte
		while( posElem->next != NULL && posElem->next != list->last->prev){
			posElem = posElem->next;		
			if( list->blockArray[posElem->blockNr].deleteCounter >= list->blockArray[element->blockNr].deleteCounter ){
				element->next = posElem;
				element->prev = posElem->prev;
				posElem->prev->next = element;
				posElem->prev = element;								
				return;
			}
		}		

		//Einfügen am Ende
		if( list->blockArray[list->last->blockNr].deleteCounter >= list->blockArray[element->blockNr].deleteCounter ){
			element->prev = list->last;
			list->last->next = element;
			list->last = element;		
		}
		else{
			element->next = list->last;
			list->last->prev = element;
			element->prev = list->last->prev;
		}
	}
}

uint32_t getFirstBlock(List_t* list){	
	uint32_t blockNr = -1;	
	ListElem_t* element = NULL;

	if(list->blockCounter <= 0){		
		return -1;
	}
	if(list->blockCounter == 1){
		
		blockNr = list->first->blockNr;
		element = list->first;
		list->first = NULL;
		list->last = NULL;
		list->AVG = 0;
		list->blockCounter = 0;		

		free(element);
		return blockNr;
	}

	if(list->blockCounter == 2){
		blockNr = list->first->blockNr;
		element = list->first;
		
		list->first = list->first->next;
		list->first->prev = NULL;
		list->last = list->first;
		
		list->blockCounter = 1;
		free(element);

		return blockNr;
	}


	blockNr = list->first->blockNr;
	element = list->first;

	list->first = list->first->next;
	list->first->prev = NULL;	
		
	list->blockCounter--;	
		
	free(element);
	return blockNr;	
}

uint32_t getLastBlock(List_t* list){	
	uint32_t blockNr = -1;	
	ListElem_t* element = NULL;

	if(list->blockCounter <= 0){		
		return -1;
	}
	
	if(list->blockCounter == 1){
		
		blockNr = list->last->blockNr;
		element = list->last;
		list->last = NULL;
		list->first = NULL;
		list->AVG = 0;
		list->blockCounter = 0;		

		free(element);
		return blockNr;
	}

	if(list->blockCounter == 2){
		blockNr = list->last->blockNr;
		element = list->last;
		
		list->last = list->last->prev;
		list->last->next = NULL;
		list->first = list->last;
		
		list->blockCounter = 1;
		free(element);

		return blockNr;
	}


	blockNr = list->last->blockNr;
	element = list->last;

	list->last = list->last->prev;
	list->last->next = NULL;	
		
	list->blockCounter--;	
		
	free(element);
	return blockNr;	
}

void recalculationAVG(List_t* list){	
	list->AVG += (double) 1 / list->blockCounter;
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
		printf("%i[%i], ", elem->blockNr, list->blockArray[elem->blockNr].deleteCounter);
		elem = elem->next;
	}
	printf("AVG: %f, AnzahlElemente: %i\n", list->AVG, list->blockCounter);
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

	for(element = list->first; element->next != NULL; element = element->next){
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