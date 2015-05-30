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
 *	F�gt ein Element zu der Liste hinzu
 */
void addElement(List_t* list, ListElem_t* element);

/*
 *	L�sche Block aus Liste raus
 *	R�ckgabewert ist als boolscher Wert zu interpretieren
 */
uint8_t delBlock(List_t* list, uint32_t blockNr);

/*
*	Gebe das Element dieser Liste zur�ck
*	und entferne aus Liste
*/
ListElem_t* getFirstElement(List_t* list);

/*
*	Gebe das letzte Element dieser Liste zur�ck
*	und entferne aus Liste
*/
ListElem_t* getLastElement(List_t* list);

/*
*	Berechne AVG dieser List nach einem neuen L�schvorgang neu
*/
void recalculationAVG(List_t* list);

/*
*	�berpr�ft, ob gegebene Blocknummer in dieser List enthalten ist
*	und gibt TRUE, wenn enthalten und FALSE, wenn nicht
*/
uint8_t isElementOfList(List_t* list, uint32_t blockNr);

/*
*	Zeigt das erste Element der Liste an, aber l�scht es nicht
*/
ListElem_t* showFirstElement(List_t* list);

/*
 *	Zeigt das letzte Element der Liste an, aber l�scht es nicht
 */
ListElem_t* showLastElement(List_t* list);

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