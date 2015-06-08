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
List_t *initList(Block_t *blockArray);

/*
*	Gebe allokierten Speicher dieser Liste zurück und gebe diese frei
*/
void freeList(List_t *list);

/*
*	Füge eine BlockNr zu der Liste hinzu
*	der Rückgabewert ist als boolscher Wert zu interpretieren
*/
uint8_t addBlock(List_t *list, uint32_t blockNr);

/*
 *	Lösche Block aus Liste raus
 *	Rückgabewert ist als boolscher Wert zu interpretieren
 */
uint8_t delBlock(List_t *list, uint32_t blockNr);

/*
*	Gebe den ersten Block dieser Liste zurück
*	und entferne aus Liste
*/
uint32_t getFirstBlock(List_t *list);

/*
*	Gebe den letzten Block dieser Liste zurück
*	und entferne aus Liste
*/
uint32_t getLastBlock(List_t *list);

/*
*	Berechne AVG dieser List nach einem neuen Löschvorgang neu
*/
void recalculationAVG(List_t *list);


/*
*	Überprüft, ob gegebene Blocknummer in dieser List enthalten ist
*	und gibt TRUE, wenn enthalten und FALSE, wenn nicht
*/
uint8_t isElementOfList(List_t *list, uint32_t blockNr);

/*
*	Zeigt das erste Element der Liste an, aber löscht es nicht
*/
ListElem_t *showFirstElement(List_t *list);

/*
 *	Zeigt das letzte Element der Liste an, aber löscht es nicht
 */
ListElem_t *showLastElement(List_t *list);

/*
 *	Gibt für ein übergebendes Element den Vorgänger zurück
 */
ListElem_t *getPrevElement(ListElem_t *elem);

/*
 *	Gibt für ein übergebendes Element den Vorgänger zurück
 */
ListElem_t *getNextElement(ListElem_t *elem);

/*
*	Gibt diese Liste auf dem Screen aus
*/
void printList(List_t *list);

/*
*	Gibt den EraseCounter der übergebenden Liste für blockNr zurück
*/
uint16_t EC(List_t *list, uint32_t blockNr);

#endif  / *__LIST__ */ 