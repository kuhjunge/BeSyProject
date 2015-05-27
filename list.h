#ifndef __LIST__
#define __LIST__

#include "ftl_structs.h"
#include "types.h"
// Public Funktionen für List_t
////////////////////////////////////////////////////////////////////
/*
*	Initialisiere eine neue leere Liste und gebe diese zurück
*	Übergabeparameter ist ein Pointer auf das Blockarray des ftl
*/
List_t* initList(Block_t* blockArray);

/*
*	Gebe allokierten Speicher dieser Liste zurück und gebe diese frei
*/
void freeList(List_t* list);

/*
*	Füge eine BlockNr zu der Liste hinzu
*/
void addBlock(List_t* list, int32_t blockNr);

/*
*	Gebe den ersten Block(BlockNr) dieser Liste zurück
*	und entferne aus Liste
*/
int32_t getFirstBlock(List_t* list);

/*
*	Gebe den letzten Block(BlockNr) dieser Liste zurück
*	und entferne aus Liste
*/
int32_t getLastBlock(List_t* list);

/*
*	Berechne AVG dieser List nach einem neuen Löschvorgang neu
*/
void recalculationAVG(List_t* list);

void calculateAVG(List_t* list, int32_t deleteCounter, uint8_t plus);

/*
*	Überprüft, ob gegebene Blocknummer in dieser List enthalten ist
*	und gibt TRUE, wenn enthalten und FALSE, wenn nicht
*/
uint8_t isElementOfList(List_t* list, int32_t blockNr);

/*
*	Gibt die BlockNr des ersten Blocks zurück
*	-1, wenn es keinen ersten Block gibt
*/
int32_t showFirstBlock(List_t* list);

/*
*	Gibt die BlockNr des letzten Blocks zurück
*	-1, wenn es keinen letzten Block gibt
*/
int32_t showLastBlock(List_t* list);

/*
*	Gibt die Länge der Liste zurück
*/
int32_t listLength(List_t* list);

/*
*	Gibt für ein übergebendes Element den Vorgänger zurück
*/
ListElem_t* getPrevElement(ListElem_t* elem);

/*
*	Gibt für ein übergebendes Element den Vorgänger zurück
*/
ListElem_t* getNextElement(ListElem_t* elem);

/*
*	Gibt diese Liste auf dem Screen aus
*/
void printList(List_t* list);


#endif  /* __LIST__ */ 