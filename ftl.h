#ifndef __FTL__
#define __FTL__

#include "types.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "flashhardware.h"

#define LOGICAL_BLOCK_DATASIZE 16	// Logische Blockgröße des OS
#define BLOCKSEGMENTS 16 // Speichersegmente pro Block
#define MAPPING_TABLE_SIZE (BLOCK_COUNT * BLOCKSEGMENTS ) +1
#define CLEAN_BLOCK_COUNT 3
#define START_CLEANING 3
#define SPARE_BLOCKS 1
/* Zustände für die physikalische Liste							*/
/* assigned =  Speicherzelle benutzt						*/
/* unused =  Speicherzelle beschreibbar						*/
/* invalid =  Speicherzelle nicht meht gültig						*/
typedef enum
{
	unused, assigned, invalid // evtl ein "inProgress" verwenden ?

} StatusPageElem_t;

typedef enum
{
	ready, used, badBlock

} BlockStatus_t;

typedef struct Block_struct
{
	StatusPageElem_t BlockStatus[BLOCKSEGMENTS];
	int loeschzaehler;
	int invalidCounter;
	BlockStatus_t status;

} Block_t;

typedef struct flash_struct
{
	flashMem_t *flashHardware; // Die Hardware mit den Daten [kann weg ?]
	uint32_t mappingTable[MAPPING_TABLE_SIZE];//[BLOCK_COUNT * PAGES_PER_BLOCK * (PAGE_DATASIZE / LOGICAL_BLOCK_DATASIZE)]; // Übersetzungstabelle
	Block_t blockArray [BLOCK_COUNT]; // Block Verwaltungsstruktur
	int invalidCounter;
	int activeBlockPosition;	// Die stelle an der der akutelle Block beschrieben wird
	uint8_t* state; // pointer auf dem der Flash speicher konserviert werden soll
	int isFehler; // Information für Unmount um Fehler zurück zu geben [kann weg ?]
	int freeBlocks;
} flash_t;

// Funktionen der Aufgabenstellung

flash_t *mount(flashMem_t *flashHardware);

flash_t *unmount(flash_t *flashDevice);

uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data);

uint8_t writeBlock(flash_t *flashDevice,uint32_t index, uint8_t *data);
#endif  /* __FTL__ */ 

