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
List_t *initList(Block_t *blockArray);

/*
*	Gebe allokierten Speicher dieser Liste zur�ck und gebe diese frei
*/
void freeList(List_t *list);

/*
*	F�ge eine BlockNr zu der Liste hinzu
*	der R�ckgabewert ist als boolscher Wert zu interpretieren
*/
uint8_t addBlock(List_t *list, uint32_t blockNr);

/*
 *	L�sche Block aus Liste raus
 *	R�ckgabewert ist als boolscher Wert zu interpretieren
 */
uint8_t delBlock(List_t *list, uint32_t blockNr);

/*
*	Gebe den ersten Block dieser Liste zur�ck
*	und entferne aus Liste
*/
uint16_t getFirstBlock(List_t *list);

/*
*	Gebe den letzten Block dieser Liste zur�ck
*	und entferne aus Liste
*/
uint16_t getLastBlock(List_t *list);

/*
*	Berechne AVG dieser List nach einem neuen L�schvorgang neu
*/
void recalculationAVG(List_t *list);


/*
*	�berpr�ft, ob gegebene Blocknummer in dieser List enthalten ist
*	und gibt TRUE, wenn enthalten und FALSE, wenn nicht
*/
uint8_t isElementOfList(List_t *list, uint32_t blockNr);

/*
*	Zeigt das erste Element der Liste an, aber l�scht es nicht
*/
ListElem_t *showFirstElement(List_t *list);

/*
 *	Zeigt das letzte Element der Liste an, aber l�scht es nicht
 */
ListElem_t *showLastElement(List_t *list);

/*
 *	Gibt f�r ein �bergebendes Element den Vorg�nger zur�ck
 */
ListElem_t *getPrevElement(ListElem_t *elem);

/*
 *	Gibt f�r ein �bergebendes Element den Vorg�nger zur�ck
 */
ListElem_t *getNextElement(ListElem_t *elem);

/*
*	Gibt diese Liste auf dem Screen aus
*/
void printList(List_t *list);

/*
*	Gibt den EraseCounter der �bergebenden Liste f�r blockNr zur�ck
*/
uint16_t EC(List_t *list, uint32_t blockNr);

#endif  / *__LIST__ */ 