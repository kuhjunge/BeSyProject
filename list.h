#ifndef __LIST__
#define __LIST__

#include "types.h"
#include "ftl.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/*	Datenstruktur für ein Listenelement
 *	prev Pointer auf vorheriges Element
 *	next Pointer auf nächstes Element
 *	blockNr Position des Blocks in flash_t.blockArray
 */
typedef struct ListElem {
	ListElem* prev;
	ListElem* next;
	uint32_t blockNr;
} ListElem_t;

/*	Datenstruktur für die nach Anzahl der Löschvorgänge sortiere doppel verkettete Liste
 *	first Pointer auf erstes Element
 *	last Pointer auf letztes Element
 *	AVG DurchschnittsLöschAnzahl
 *	blockCounter Zähler der enthaltenen Blöcke
 *	blockArray Pointer auf das verwendete Blockarray des ftl
 */
typedef struct {
	ListElem_t* first;
	ListElem_t* last;
	uint32_t AVG;
	uint32_t blockCounter;
	Block_t* blockArray;
} List_t;

// PUBLIC Funktionen
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
void addBlock(List_t* list, uint16_t blockNr);

/*
 *	Gebe den ersten Block(BlockNr) dieser Liste zurück
 */
uint32_t getFirstBlock(List_t* list);

/*
 *	Gebe den letzten Block(BlockNr) dieser Liste zurück
 */ 
uint32_t getLastBlock(List_t* list);

/*
 *	Berechne AVG dieser List nach einem neuen Löschvorgang neu
 */
void recalculationAVG(List_t* list);

/*
 *	Überprüft, ob gegebene Blocknummer in dieser List enthalten ist
 *	und gibt TRUE, wenn enthalten und FALSE, wenn nicht
 */
bool isElementOfList(List_t* list, uint32_t blockNr);

#endif