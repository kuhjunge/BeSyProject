#ifndef __FTL__
#define __FTL__

#include "types.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "flashhardware.h"
//#include "list.h"

// Allocator Konstanten
#define LOGICAL_BLOCK_DATASIZE 16													// Logische Blockgr��e des OS
#define BLOCKSEGMENTS (PAGE_DATASIZE * PAGES_PER_BLOCK  / LOGICAL_BLOCK_DATASIZE )  // Speichersegmente pro Block
#define MAPPING_TABLE_SIZE (BLOCK_COUNT * BLOCKSEGMENTS )							// TODO: Ist es in Ordnung Konstanten bei der Definition der Konstanten zu verwenden?
// Cleaner Konstanten
#define START_CLEANING 3															// Anzahl ab der der Cleaning Algorithmus gestartet wird
#define SPARE_BLOCKS 1																// Anzahl der Reserve Blocks zus�tzlich zum active Block
// Wear-Leveler ([TC11]- Algorithmus) Konstanten
#define THETA 12																	// Definiert die Gr��e des neutralen Pools	
#define DELTA 5																		// Definiert den Bereich f�r BlockNeutralisationen
// ToDo 

/*	Zust�nde f�r die physikalische Liste							
 *	empty =  Speicherzelle benutzt						
 *	assigned =  Speicherzelle beschreibbar						
 *	invalid =  Speicherzelle nicht meht g�ltig
 */
typedef enum
{
	empty, assigned, invalid

} StatusPageElem_t;

/*	Zust�nde f�r die Blockverwaltung
 *	ready =  Block ist bereit um beschrieben zu werden (oder wird gerade beschrieben)
 *	used =  Block beschrieben
 *	badBlock =  Block defekt
 */
typedef enum
{
	ready, used, badBlock, active

} BlockStatus_t;

/*	Datenstruktur f�r die Blockverwaltung
 *	segmentStatus Array mit den Status der einzelnen segmente eines Blocks
 *  deleteCounter h�lt fest wie oft der Block gel�scht wurde
 *  invalidCounter h�lt fest wie viele Segmente in diesem Block invalid markiert sind
 *  status h�lt die Status des Blockes fest -> [BlockStatus_t]
 */
typedef struct Block_struct
{
	uint16_t deleteCounter;
	uint16_t invalidCounter;
	BlockStatus_t status;

} Block_t;

/*	Datenstruktur f�r den FTL 
 *	mappingTable Tabelle in der das Mapping gespeichert wird
 *  blockArray Array mit Block Datenstruktur zur Verwaltung der Bl�cke-> Siehe Block_t
 *  invalidCounter Z�hlt die invaliden Segmente im gesammten FTL
 *  activeBlockPosition Aktuelle Schreibposition
 *  isErr ?????
 *  freeBlocks Anzahl der freien Blocks die zum Schreibzugriff zur verf�gung stehen
 */
typedef struct flash_struct
{
	uint32_t mappingTable[MAPPING_TABLE_SIZE];//[BLOCK_COUNT * PAGES_PER_BLOCK * (PAGE_DATASIZE / LOGICAL_BLOCK_DATASIZE)]; // �bersetzungstabelle
	Block_t blockArray [BLOCK_COUNT]; // Block Verwaltungsstruktur
	uint16_t invalidCounter;
	uint16_t activeBlockPosition;	// Die stelle an der der akutelle Block beschrieben wird
	uint16_t isNoErr; // Information f�r Unmount um Fehler zur�ck zu geben [kann weg ?]
	uint16_t freeBlocks;
 //	List_t* hotPool;
//	List_t* coldPool;
//	List_t* neutralPool;
	uint32_t AVG;// globaler AVG
} flash_t;

// PUBLIC Funktionen
////////////////////////////////////////////////////////////////////
/*
 * Mounten des Flash-Datentr�gers. erzeugt die Datenstrukturen des FTL aus dem im
 * Flash abgelegten Zustand. Nach dem Mounten ist der Datentr�ger bereit
 * zur Verwendung.
 * flashHardware zeigt auf die Datenstruktur vom Typ flashMem_t, die den FlashDatentr�ger
 * modelliert.
 * Der R�ckgabewert ist ein Pointer auf eine von Ihnen definierte Datenstruktur mit
 * Typnamen flash_t, die bei weiteren Funktionen dieses Interfaces als Parameter
 * �bergeben wird, um den Flash-Speicher auszuw�hlen (analog der Datenstruktur FILE f�r
 * Dateien).
 */
flash_t *mount(flashMem_t *flashHardware);

/*
 * Unmounten des Flash-Datentr�gers. Dabei werden die Datenstrukturen des FTL in der
 * Repr�sentation als Byte-Array auf dem Flash abgelegt. Nach dem Unmounten ist
 * eine weitere Verwendung des Datentr�gers erst nach erneutem Mounten m�glich.
 * flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 * dieses Flash-Datentr�gers dient.
 * Der R�ckgabewert ist ein Pointer auf eine Datenstruktur vom Typ flash_t. Im
 *  Erfolgsfall wird NULL zur�ckgegeben(ToDo: ,bei einem Fehler kann die Information des
 * Fehlerfalls in der Datenstruktur abgelegt sein).
 */
flash_t *unmount(flash_t *flashDevice);

/*
 * Liest einen Datenblock an der angegebene Indexposition vom Flashspeicher, der mit der
 * in flashDevice �bergebenen Datenstruktur verwaltet wird.
 * flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 * dieses Flash-Datentr�gers dient.
 * index ist die Nummer des zu lesenden Blocks.
 * data ist ein Pointer auf einen ausreichend gro�en Datenbereich, in den die zu lesenden
 * Daten kopiert werden.
 * Der R�ckgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data);

/*
 * Schreibt einen Datenblock an der angegebene Indexposition auf den Flashspeicher, der
 * mit der in flashDevice �bergebenen Datenstruktur verwaltet wird.
 * flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 * dieses Flash-Datentr�gers dient.
 * index ist die Nummer des zu schreibenden Blocks auf dem Flashdevice
 * data ist ein Pointer auf den Quelldatenblock.
 * Der R�ckgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t writeBlock(flash_t *flashDevice,uint32_t index, uint8_t *data);

// PUBLIC DEBUG Funktionen
////////////////////////////////////////////////////////////////////
/*
 * Debug Funktion
 * gibt die Struktur der flash_t Datenstruktur auf der Konsole aus
 */
void printerr(flash_t *flashDevice);

#endif  /* __FTL__ */ 

