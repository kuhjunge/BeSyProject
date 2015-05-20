#ifndef __FTL__
#define __FTL__

#include "types.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "flashhardware.h"
//#include "list.h"

// Allocator Konstanten
#define LOGICAL_BLOCK_DATASIZE 16													// Logische Blockgröße des OS
#define BLOCKSEGMENTS (PAGE_DATASIZE * PAGES_PER_BLOCK  / LOGICAL_BLOCK_DATASIZE )  // Speichersegmente pro Block
#define MAPPING_TABLE_SIZE (BLOCK_COUNT * BLOCKSEGMENTS )							// TODO: Ist es in Ordnung Konstanten bei der Definition der Konstanten zu verwenden?
// Cleaner Konstanten
#define START_CLEANING 3															// Anzahl ab der der Cleaning Algorithmus gestartet wird
#define SPARE_BLOCKS 1																// Anzahl der Reserve Blocks zusätzlich zum active Block
// Wear-Leveler ([TC11]- Algorithmus) Konstanten
#define THETA 12																	// Definiert die Größe des neutralen Pools	
#define DELTA 5																		// Definiert den Bereich für BlockNeutralisationen
// ToDo 

/*	Zustände für die physikalische Liste							
 *	empty =  Speicherzelle benutzt						
 *	assigned =  Speicherzelle beschreibbar						
 *	invalid =  Speicherzelle nicht meht gültig
 */
typedef enum
{
	empty, assigned, invalid

} StatusPageElem_t;

/*	Zustände für die Blockverwaltung
 *	ready =  Block ist bereit um beschrieben zu werden (oder wird gerade beschrieben)
 *	used =  Block beschrieben
 *	badBlock =  Block defekt
 */
typedef enum
{
	ready, used, badBlock, active

} BlockStatus_t;

/*	Datenstruktur für die Blockverwaltung
 *	segmentStatus Array mit den Status der einzelnen segmente eines Blocks
 *  deleteCounter hält fest wie oft der Block gelöscht wurde
 *  invalidCounter hält fest wie viele Segmente in diesem Block invalid markiert sind
 *  status hält die Status des Blockes fest -> [BlockStatus_t]
 */
typedef struct Block_struct
{
	uint16_t writePos;
	uint16_t deleteCounter;
	uint16_t invalidCounter;
	BlockStatus_t status;

} Block_t;

/*	Datenstruktur für den FTL 
 *	mappingTable Tabelle in der das Mapping gespeichert wird
 *  blockArray Array mit Block Datenstruktur zur Verwaltung der Blöcke-> Siehe Block_t
 *  invalidCounter Zählt die invaliden Segmente im gesammten FTL
 *  activeBlockPosition Aktuelle Schreibposition
 *  isErr ?????
 *  freeBlocks Anzahl der freien Blocks die zum Schreibzugriff zur verfügung stehen
 */
typedef struct flash_struct
{
	uint32_t mappingTable[MAPPING_TABLE_SIZE];//[BLOCK_COUNT * PAGES_PER_BLOCK * (PAGE_DATASIZE / LOGICAL_BLOCK_DATASIZE)]; // Übersetzungstabelle
	Block_t blockArray [BLOCK_COUNT]; // Block Verwaltungsstruktur
	uint16_t invalidCounter;
	uint16_t isNoErr; // Information für Unmount um Fehler zurück zu geben [kann weg ?]
	uint16_t freeBlocks;
	uint16_t actWriteBlock;
 //	List_t* hotPool;
//	List_t* coldPool;
//	List_t* neutralPool;
	uint32_t AVG;// globaler AVG
} flash_t;

// PUBLIC Funktionen
////////////////////////////////////////////////////////////////////
/*
 * Mounten des Flash-Datenträgers. erzeugt die Datenstrukturen des FTL aus dem im
 * Flash abgelegten Zustand. Nach dem Mounten ist der Datenträger bereit
 * zur Verwendung.
 * flashHardware zeigt auf die Datenstruktur vom Typ flashMem_t, die den FlashDatenträger
 * modelliert.
 * Der Rückgabewert ist ein Pointer auf eine von Ihnen definierte Datenstruktur mit
 * Typnamen flash_t, die bei weiteren Funktionen dieses Interfaces als Parameter
 * übergeben wird, um den Flash-Speicher auszuwählen (analog der Datenstruktur FILE für
 * Dateien).
 */
flash_t *mount(flashMem_t *flashHardware);

/*
 * Unmounten des Flash-Datenträgers. Dabei werden die Datenstrukturen des FTL in der
 * Repräsentation als Byte-Array auf dem Flash abgelegt. Nach dem Unmounten ist
 * eine weitere Verwendung des Datenträgers erst nach erneutem Mounten möglich.
 * flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 * dieses Flash-Datenträgers dient.
 * Der Rückgabewert ist ein Pointer auf eine Datenstruktur vom Typ flash_t. Im
 *  Erfolgsfall wird NULL zurückgegeben(ToDo: ,bei einem Fehler kann die Information des
 * Fehlerfalls in der Datenstruktur abgelegt sein).
 */
flash_t *unmount(flash_t *flashDevice);

/*
 * Liest einen Datenblock an der angegebene Indexposition vom Flashspeicher, der mit der
 * in flashDevice übergebenen Datenstruktur verwaltet wird.
 * flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 * dieses Flash-Datenträgers dient.
 * index ist die Nummer des zu lesenden Blocks.
 * data ist ein Pointer auf einen ausreichend großen Datenbereich, in den die zu lesenden
 * Daten kopiert werden.
 * Der Rückgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data);

/*
 * Schreibt einen Datenblock an der angegebene Indexposition auf den Flashspeicher, der
 * mit der in flashDevice übergebenen Datenstruktur verwaltet wird.
 * flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
 * dieses Flash-Datenträgers dient.
 * index ist die Nummer des zu schreibenden Blocks auf dem Flashdevice
 * data ist ein Pointer auf den Quelldatenblock.
 * Der Rückgabewert ist als Boolescher Wert zu interpretieren.
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

