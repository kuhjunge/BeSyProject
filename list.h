#ifndef __LIST__
#define __LIST__

#include "types.h"
#include "ftl.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/*	Datenstruktur f�r ein Listenelement
 *	prev Pointer auf vorheriges Element
 *	next Pointer auf n�chstes Element
 *	blockNr Position des Blocks in flash_t.blockArray
 */
typedef struct ListElem {
	ListElem* prev;
	ListElem* next;
	uint16_t blockNr;
} ListElem_t;

/*	Datenstruktur f�r die nach Anzahl der L�schvorg�nge sortiere doppel verkettete Liste
 *	first Pointer auf erstes Element
 *	last Pointer auf letztes Element
 *	AVG DurchschnittsL�schAnzahl
 *	blockCounter Z�hler der enthaltenen Bl�cke
 *	blockArray Pointer auf das verwendete Blockarray des ftl
 */
typedef struct {
	ListElem_t* first;
	ListElem_t* last;
	uint16_t AVG;
	uint16_t blockCounter;
	Block_t** blockArray;
} List_t;

// PUBLIC Funktionen
////////////////////////////////////////////////////////////////////
/*
 *	Initialisiere eine neue leere Liste und gebe diese zur�ck
 *	�bergabeparameter ist ein Pointer auf das Blockarray des ftl
 */
List_t* initList(Block_t** blockArray);

/*
 *	Gebe allokierten Speicher dieser Liste zur�ck und gebe diese frei
 */
void freeList(List_t* list);

/*
 *	F�ge eine BlockNr zu der Liste hinzu
 */
void addBlock(List_t* list, uint16_t blockNr);

/*
 *	Gebe den ersten Block(BlockNr) dieser Liste zur�ck
 */
uint16_t getFirstBlock(List_t* list);

/*
 *	Gebe den letzten Block(BlockNr) dieser Liste zur�ck
 */ 
uint16_t getLastBlock(List_t* list);

#endif