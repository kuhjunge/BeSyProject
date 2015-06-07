#include "ftl.h"
#include "list.h"

// Lokale Funktionsimplementation List
////////////////////////////////////////////////////////////////////
List_t *initList(Block_t *blockArray){
	List_t *list = (List_t*)malloc(sizeof(List_t));
	list->AVG = 0;
	list->blockArray = blockArray;
	list->blockCounter = 0;
	list->first = NULL;
	list->last = NULL;
	return list;
}

void freeList(List_t *list){
	while(list->blockCounter > 0){
		getFirstBlock(list);
	}
	free(list);
}

void recalculationAVG(List_t *list){	
	list->AVG += (double) 1 / list->blockCounter;
}

ListElem_t *showFirstElement(List_t *list){
	if(list->blockCounter <= 0){
		return NULL;
	}
	else{
		return list->first;
	}
}

ListElem_t *showLastElement(List_t *list){
	if(list->blockCounter <= 0){
		return NULL;
	}
	else{
		return list->last;
	}
}

ListElem_t *getPrevElement(ListElem_t *elem){
	if(elem == NULL){
		return NULL;
	}
	else{
		return elem->prev;	
	}
}

ListElem_t *getNextElement(ListElem_t *elem){
	if(elem == NULL){
		return NULL;
	}
	else{
		return elem->next;	
	}
}

void printList(List_t *list){
	ListElem_t *elem;
	int counter = 0;

	if(list->first == NULL)
		return;

	elem = list->first;
	do{
		if(elem == NULL){
			break;
		}
		printf("%i, ", elem->blockNr);		
		counter++;
		elem = elem->next ;
	}while(elem != NULL);
	printf("AVG: %f, AnzahlElemente: %i/%i\n", list->AVG, counter, list->blockCounter);
}

uint8_t isElementOfList(List_t *list, uint32_t blockNr){	
	ListElem_t *position;

	if( list->blockCounter <= 0 || blockNr < 0){
		return FALSE;
	}

	position = list->first;
	do{
		if(position == NULL){
			return FALSE;
		}
		if(position->blockNr == blockNr){
			return TRUE;
		}
		position = position->next ;
	}while(position != NULL);

	return FALSE;
}

uint16_t countElements(List_t *list){
	uint16_t counter = 1;
	ListElem_t *pos;

	for(pos = list->first; pos != NULL; pos = pos->next)
		counter++;

	return counter;
}

uint8_t delBlock(List_t *list, uint32_t blockNr){	
	ListElem_t *position = NULL;

	//Fehler
	if(list->blockCounter <= 0){
		return FALSE;
	}

	//Sonderfall 1 Element
	if( list->blockCounter == 1){
		list->AVG = 0;
		list->blockCounter = 0;
		free(list->first);
		list->first = NULL;
		list->last = NULL;
		return TRUE;
	}

	position = list->first;
	do{
		if(position == NULL){			
			return FALSE;
		}
		if( position->blockNr == blockNr){
			if(position->next == NULL){
				list->last = list->last->prev;
				list->last->next = NULL;				
			}
			else{
				position->next->prev = position->prev;
			}
			if(position->prev == NULL){	
				list->first = position->next;				
				list->first->prev = NULL;
			}
			else{
				position->prev->next = position->next;
			}

			list->AVG = list->AVG  *list->blockCounter - list->blockArray[blockNr].deleteCounter;
			list->blockCounter--;
			list->AVG = (double)list->AVG / list->blockCounter;
			free(position);			
			return TRUE;
		}

		position = position->next ;
	}while(position != NULL);

	return FALSE;
}

uint16_t EC(List_t *list, uint32_t blockNr){
	return list->blockArray[blockNr].deleteCounter;
}

uint8_t addBlock(List_t *list, uint32_t blockNr){
	ListElem_t *element = (ListElem_t*)malloc(sizeof(ListElem_t));
	ListElem_t *position = NULL;

	//Fehlerfall
	if( blockNr < 0 || isElementOfList(list, blockNr) == TRUE){
		free(element);
		return FALSE;
	}

	element->blockNr = blockNr;
	element->next = NULL;
	element->prev = NULL;

	// 0 Elemente in Liste
	if( list->blockCounter == 0){
		list->AVG = list->blockArray[blockNr].deleteCounter;
		list->blockCounter = 1;
		list->first = element;
		list->last = element;
		return TRUE;
	}
	// 1 Element in Liste
	if( list->blockCounter == 1){
		if( EC(list, list->first->blockNr) > EC(list, blockNr) ){
			list->first = element;
			list->first->next = list->last;
			list->last->prev = element;			
		}
		else{
			list->last = element;
			list->last->prev = list->first;
			list->first->next = element;
		}
				
		list->AVG = list->AVG + list->blockArray[blockNr].deleteCounter;
		list->blockCounter++;
		list->AVG = (double)list->AVG / list->blockCounter;
		return TRUE;
	}
	// mehr als 1 Elemente in Liste
	if( list->blockCounter > 1){		

		list->AVG = list->AVG  *list->blockCounter + list->blockArray[blockNr].deleteCounter;
		list->blockCounter++;
		list->AVG = (double)list->AVG / list->blockCounter;
		
		position = list->first;		
		do{	
			if(position == NULL){
				break;
			}						
			if( EC(list, position->blockNr) > EC(list, blockNr)){				
				element->next = position;
				element->prev = position->prev;								
				if( list->first->blockNr == position->blockNr){
					list->first = element;
					position->prev = element;
				}
				else{				
					position->prev->next = element;
					position->prev = element;
					
				}
				return TRUE;
			}			

			position = position->next ;
		}while(position != NULL);

		//an letzter Position einfügen		
		element->prev = list->last;
		list->last->next = element;
		list->last = element;

		return TRUE;

	}
		
	free(element);
	return FALSE;
}

uint32_t getFirstBlock(List_t *list){
	uint32_t value = -1;

	if(list->blockCounter <= 0){
		return value;
	}

	value = list->first->blockNr;
	if( delBlock(list, value) == TRUE){
		return value;
	}
	else{
		return -1;
	}
}

uint32_t getLastBlock(List_t *list){	
	uint32_t value = -1;

	if(list->blockCounter <= 0){
		return value;
	}

	value = list->last->blockNr;
	if ( delBlock(list, value) == TRUE){
		return value;
	}
	else{
		return -1;
	}
}
