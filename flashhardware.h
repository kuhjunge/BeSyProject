/* Include-Datei für die Simulation eines Flash-Speichers */
#ifndef __FLASH_HW__
#define __FLASH_HW__

#include "types.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// globale Defines, die die Simulation der Eigenschaften steuern
#define FL_WEAR_OUT_LIMIT 5000		// Anzahl möglicher Löschzyklen
#define FL_RANDOM_FAIL_PROBABILITY	10 // Wahrscheinlichkeit (in %), dass ein Block ausfällt, wenn DeleteCount den Schwellwert FL_WEAR_OUT_LIMIT erreicht

// Auswahl der Simulationsmethode für Wear-Out: 
// 0: Feste Grenze, wenn DeleteCount den Schwellwert FL_WEAR_OUT_LIMIT erreicht ist der Block defekt. 
// 1: Bei Überschreiten von FL_WEAR_OUT_LIMIT besteht bei jedem Löschen eine durch FL_RANDOM_FAIL_PROBABILITY gegebene Wahrscheinlichkeit, dass der Block beim Löschen ausfällt.  
#define FL_USE_RANDOM	0			


// globale Defines, die die physikalische Struktur des Flash parametrieren
#define PARTITION_COUNT	1		// Anzahl der Partitionen, nur eine Unterstützt
#define BLOCK_COUNT 32			// Anzahl der Blöcke 
#define PAGES_PER_BLOCK 4		// Anzahl der Pages in einem Block 
#define PAGE_DATASIZE 64		// Größe des Datenbereichs einer Page in Byte
#define PAGE_SPARESIZE 64		// Größe des Reserevebereichs einer Page in Byte 


#define FLASH_INITIALISED 0xA2		// indicates an initialised flash memory hardware
#define STATEBLOCKSIZE 512			// Größe der Datenblöcke in denen der Zustand bei unmount gespeichert wird


typedef struct flashPage_struct 
	{
		uint8_t data[PAGE_DATASIZE];
		uint8_t spare[PAGE_SPARESIZE];
	} flashPage_t; 

typedef struct flashBlock_struct
	{
		uint8_t dead; 		// zero to indicate usable block, else defunct
		flashPage_t page[PAGES_PER_BLOCK]; 
		uint32_t deleteCount; 
	} flashBlock_t; 


typedef struct flashMem_struct
	{
		uint8_t initialised; 		// MagicValue FLASH_INITIALISED to indicate initialised instance all other uninitialised
		flashBlock_t block[BLOCK_COUNT]; 
		uint16_t partitionCount;
		uint16_t blockCount;
		uint16_t pagesPerBlock;
		uint32_t pageDataSize;
		uint16_t pageSpareSize; 
		uint8_t	*stateStorage; 
		uint8_t stateStorageSize;		// in multiples of STATEBLOCKSIZE
		uint32_t wearOutLimit;
		uint8_t randomFailProbability; 
	} flashMem_t; 


////////////////////////////////////////////////////////////////////
// Deklaration der Instanz eines Flash-Speichermoduls
extern flashMem_t flashMem; 

////////////////////////////////////////////////////////////////////
// PUBLIC Funktionen

uint8_t FL_resetFlash();
// Initialisiert das Flashmodul, alle Blöcke sind gelöscht und die Hardware ungenutzt. 

uint8_t FL_deleteBlock (uint16_t block); 
//Löscht den angegebenen Block. Er enthält danach nur Bytes mit dem Wert 0xFF. 
//Der Rückgabewert ist als Boolescher Wert zu interpretieren. 

uint16_t FL_writeData (uint16_t block, uint16_t page, uint16_t index, 
				uint16_t len, void *data);
//Schreibt in den Nutzdatenbereich einer Pages des angegebenen Datenblocks. Die Eigenschaft von Flash, nur Nullen programmieren zu können wird berücksichtigt. Damit die Funktion korrekt arbeiten kann, muss der zu schreibende Bereich daher vorher komplett auf 0xFF gesetzt worden sein (mit deleteBlock). 
//block gibt die Nummer des Blocks an, in dem geschrieben werden soll
//page gibt die Nummer der Page in diesem Block an
//index gibt die Nummer des ersten Bytes in der adressierten Page an, die geschrieben werden soll. 
//len gibt die Anzahl der zu schreibenden Byte an. 
//data ist ein Pointer auf die zu schreibenden Daten. 
//Der Rückgabewert gibt die Anzahl der wirklich geschriebenen Byte an. 

 
uint16_t FL_readData (uint16_t block, uint16_t page, uint16_t index, uint16_t len, void *data);
//Liest aus dem Nutzdatenbereich einer Pages des angegebenen Datenblocks. 
//block gibt die Nummer des Blocks an, aus dem gelesen werden soll
//page gibt die Nummer der Page in diesem Block an
//index gibt die Nummer des ersten Bytes in der adressierten Page an, die gelesen werden soll. 
//len gibt die Anzahl der zu lesenden Byte an. 
//data ist ein Pointer auf den Datenbereich, in den die gelesenen Byte kopiert werden. 
//Der Rückgabewert gibt die Anzahl der wirklich gelesenen Byte an. 

uint16_t FL_writeSpare (uint16_t block, uint16_t page, uint16_t index, uint16_t len, void *data);
//Analog zu writeData, nur dass die für das Dateisystem nicht erreichbare Spare Area geschrieben wird. 

uint16_t FL_readSpare (uint16_t block, uint16_t page, uint16_t index, uint16_t len, void *data);
//Analog zu readData, nur dass die für das Dateisystem nicht erreichbare Spare Area gelesen wird.

uint8_t FL_saveState(uint8_t blockCount, uint8_t *state);
//Speichert den als Byte-Array repräsentierten Zustand des Flash-Speichers auf diesem ab. 
//blockCount gibt an wie viele Blöcke der Größe 512 Byte zu schreiben sind. 
//state ist der Pointer auf die Datenstruktur innerhalb des FTL, die auf dem Flash zu konservieren ist. 
//Der Rückgabewert ist als Boolescher Wert zu interpretieren. 

uint8_t FL_getStateSize(void);
//Liefert die Größe des zum Ablegen des Zustands des Flash verwendeten Byte-Arrays in Vielfachen von 512 Byte. Eine Größe von Null bedeutet, dass kein Zustand gespeichert ist. 

uint8_t *FL_restoreState(uint8_t *state);
//Liest den als Byte-Array repräsentierten Zustand des Flash-Speichers von diesem aus.  
//state ist der Pointer auf die Datenstruktur innerhalb des FTL. Hier werden die Daten hin kopiert. Der Speicherbereich muss ausreichend groß sein. 
//Der Rückgabewert ist im Erfolgsfall gleich dem Parameter state. Es wird NULL zurückgegeben, wenn der Zustand nicht gelesen werden konnte. 

uint16_t FL_getPartitionCount ();
//	Liefert die Anzahl an Partitionen auf dem Datenträger. 
//	Gegeben durch #define PARTITION_COUNT, hier immer 1.

uint16_t FL_getBlockCount ();
//	Liefert die Anzahl an Blöcken auf dem Datenträger. 
//	Gegeben durch #define BLOCK_COUNT, default hier 32

uint16_t FL_getPagesPerBlock ();
//	Liefert die Anzahl an Pages in einem Block. 
//Gegeben durch #define PAGES_PER_BLOCK, default hier 4

uint32_t FL_getPageDataSize ();
//	Liefert die Größe des Datenbereichs einer Page in Byte in einem Block. 
//Gegeben durch #define PAGE_DATASIZE, default hier 64

uint16_t FL_getPageSpareSize ();
//	Liefert die Größe des Reserevebereichs einer Page in Byte in einem Block. 
//	Gegeben durch #define PAGE_SPARESIZE, default hier 64


#endif  /* __FLASH_HW__ */ 