/* Include-Datei f�r die Simulation eines Flash-Speichers */
#ifndef __FLASH_HW__
#define __FLASH_HW__

#include "types.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// globale Defines, die die physikalische Struktur des Flash parametrieren
#define PARTITION_COUNT	1	// Anzahl der Partitionen, nur eine Unterst�tzt
#define BLOCK_COUNT 4		// Anzahl der Bl�cke 
#define PAGES_PER_BLOCK 2	// Anzahl der Pages in einem Block 
#define PAGE_DATASIZE 8		// Gr��e des Datenbereichs einer Page in Byte
#define PAGE_SPARESIZE 4	// Gr��e des Reserevebereichs einer Page in Byte 

#define FLASH_INITIALISED 0xA2		// indicates an initialised flash memory hardware
#define STATEBLOCKSIZE 512			// Gr��e der Datenbl�cke in denen der Zustand bei unmount gespeichert wird

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
	} flashMem_t; 


////////////////////////////////////////////////////////////////////
// Deklaration der Instanz eines Flash-Speichermoduls
extern flashMem_t flashMem; 

////////////////////////////////////////////////////////////////////
// PUBLIC Funktionen

uint8_t FL_resetFlash();
// Initialisiert das Flashmodul, alle Bl�cke sind gel�scht und die Hardware ungenutzt. 

uint8_t FL_deleteBlock (uint16_t block); 
//L�scht den angegebenen Block. Er enth�lt danach nur Bytes mit dem Wert 0xFF. 
//Der R�ckgabewert ist als Boolescher Wert zu interpretieren. 

uint16_t FL_writeData (uint16_t block, uint16_t page, uint16_t index, 
				uint16_t len, void *data);
//Schreibt in den Nutzdatenbereich einer Pages des angegebenen Datenblocks. Die Eigenschaft von Flash, nur Nullen programmieren zu k�nnen wird ber�cksichtigt. Damit die Funktion korrekt arbeiten kann, muss der zu schreibende Bereich daher vorher komplett auf 0xFF gesetzt worden sein (mit deleteBlock). 
//block gibt die Nummer des Blocks an, in dem geschrieben werden soll
//page gibt die Nummer der Page in diesem Block an
//index gibt die Nummer des ersten Bytes in der adressierten Page an, die geschrieben werden soll. 
//len gibt die Anzahl der zu schreibenden Byte an. 
//data ist ein Pointer auf die zu schreibenden Daten. 
//Der R�ckgabewert gibt die Anzahl der wirklich geschriebenen Byte an. 

 
uint16_t FL_readData (uint16_t block, uint16_t page, uint16_t index, uint16_t len, void *data);
//Liest aus dem Nutzdatenbereich einer Pages des angegebenen Datenblocks. 
//block gibt die Nummer des Blocks an, aus dem gelesen werden soll
//page gibt die Nummer der Page in diesem Block an
//index gibt die Nummer des ersten Bytes in der adressierten Page an, die gelesen werden soll. 
//len gibt die Anzahl der zu lesenden Byte an. 
//data ist ein Pointer auf den Datenbereich, in den die gelesenen Byte kopiert werden. 
//Der R�ckgabewert gibt die Anzahl der wirklich gelesenen Byte an. 

uint16_t FL_writeSpare (uint16_t block, uint16_t page, uint16_t index, uint16_t len, void *data);
//Analog zu writeData, nur dass die f�r das Dateisystem nicht erreichbare Spare Area geschrieben wird. 

uint16_t FL_readSpare (uint16_t block, uint16_t page, uint16_t index, uint16_t len, void *data);
//Analog zu readData, nur dass die f�r das Dateisystem nicht erreichbare Spare Area gelesen wird.

uint8_t FL_saveState(uint8_t blockCount, uint8_t *state);
//Speichert den als Byte-Array repr�sentierten Zustand des Flash-Speichers auf diesem ab. 
//blockCount gibt an wie viele Bl�cke der Gr��e 512 Byte zu schreiben sind. 
//state ist der Pointer auf die Datenstruktur innerhalb des FTL, die auf dem Flash zu konservieren ist. 
//Der R�ckgabewert ist als Boolescher Wert zu interpretieren. 

uint8_t FL_getStateSize(void);
//Liefert die Gr��e des zum Ablegen des Zustands des Flash verwendeten Byte-Arrays in Vielfachen von 512 Byte. Eine Gr��e von Null bedeutet, dass kein Zustand gespeichert ist. 

uint8_t *FL_restoreState(uint8_t *state);
//Liest den als Byte-Array repr�sentierten Zustand des Flash-Speichers von diesem aus.  
//state ist der Pointer auf die Datenstruktur innerhalb des FTL. Hier werden die Daten hin kopiert. Der Speicherbereich muss ausreichend gro� sein. 
//Der R�ckgabewert ist im Erfolgsfall gleich dem Parameter state. Es wird NULL zur�ckgegeben, wenn der Zustand nicht gelesen werden konnte. 

uint16_t FL_getPartitionCount ();
//	Liefert die Anzahl an Partitionen auf dem Datentr�ger. 
//	Gegeben durch #define PARTITION_COUNT, hier immer 1.

uint16_t FL_getBlockCount ();
//	Liefert die Anzahl an Bl�cken auf dem Datentr�ger. 
//	Gegeben durch #define BLOCK_COUNT, default hier 32

uint16_t FL_getPagesPerBlock ();
//	Liefert die Anzahl an Pages in einem Block. 
//Gegeben durch #define PAGES_PER_BLOCK, default hier 4

uint32_t FL_getPageDataSize ();
//	Liefert die Gr��e des Datenbereichs einer Page in Byte in einem Block. 
//Gegeben durch #define PAGE_DATASIZE, default hier 64

uint16_t FL_getPageSpareSize ();
//	Liefert die Gr��e des Reserevebereichs einer Page in Byte in einem Block. 
//	Gegeben durch #define PAGE_SPARESIZE, default hier 64


#endif  /* __FLASH_HW__ */ 