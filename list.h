#ifndef __LIST__
#define __LIST__

#include "ftl_structs.h"
#include "types.h"
// Public Funktionen f�r List_t
////////////////////////////////////////////////////////////////////
/*
*	Initialisiere eine neue leere Liste und gebe diese zur�ck
*	�bergabeparameter ist ein Pointer auf das Blockarray des ftl
*/
List_t* initList(Block_t* blockArray);

/*
*	Gebe allokierten Speicher dieser Liste zur�ck und gebe diese frei
*/
void freeList(List_t* list);

/*
*	F�ge eine BlockNr zu der Liste hinzu
*/
void addBlock(List_t* list, uint32_t blockNr);

/*
 *	L�sche Block aus Liste raus
 *	R�ckgabewert ist als boolscher Wert zu interpretieren
 */
uint8_t delBlock(List_t* list, uint32_t blockNr);

/*
*	Gebe den ersten Block(BlockNr) dieser Liste zur�ck
*	und entferne aus Liste
*/
uint32_t getFirstBlock(List_t* list);

/*
*	Gebe den letzten Block(BlockNr) dieser Liste zur�ck
*	und entferne aus Liste
*/
uint32_t getLastBlock(List_t* list);

/*
*	Berechne AVG dieser List nach einem neuen L�schvorgang neu
*/
void recalculationAVG(List_t* list);

void calculateAVG(List_t* list, uint32_t deleteCounter, uint8_t plus);

/*
*	�berpr�ft, ob gegebene Blocknummer in dieser List enthalten ist
*	und gibt TRUE, wenn enthalten und FALSE, wenn nicht
*/
uint8_t isElementOfList(List_t* list, uint32_t blockNr);

/*
*	Gibt die BlockNr des ersten Blocks zur�ck
*	-1, wenn es keinen ersten Block gibt
*/
uint32_t showFirstBlock(List_t* list);

/*
*	Gibt die BlockNr des letzten Blocks zur�ck
*	-1, wenn es keinen letzten Block gibt
*/
uint32_t showLastBlock(List_t* list);

/*
*	Gibt die L�nge der Liste zur�ck
*/
uint32_t listLength(List_t* list);

/*
*	Gibt f�r ein �bergebendes Element den Vorg�nger zur�ck
*/
ListElem_t* getPrevElement(ListElem_t* elem);

/*
*	Gibt f�r ein �bergebendes Element den Vorg�nger zur�ck
*/
ListElem_t* getNextElement(ListElem_t* elem);

/*
*	Gibt diese Liste auf dem Screen aus
*/
void printList(List_t* list);


#endif  /* __LIST__ */ 