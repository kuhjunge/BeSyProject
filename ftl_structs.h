#ifndef __FTL_STRUCT__
#define __FTL_STRUCT__

/*
TODO: Die MAPPING_TABLE_SIZE und BLOCKSEGMENTS dürfen nicht auf Konstanten aus der Aufgabenstellung basieren und müssen deshalb geändert werden (alloc & lokale Variablen)
*/
// Allocator Konstanten
#define LOGICAL_BLOCK_DATASIZE 16													// Logische Blockgröße des OS
#define BLOCKSEGMENTS (PAGE_DATASIZE * PAGES_PER_BLOCK  / LOGICAL_BLOCK_DATASIZE )  // Speichersegmente pro Block
#define MAPPING_TABLE_SIZE (BLOCK_COUNT * BLOCKSEGMENTS )							
// Cleaner Konstanten
#define SPARE_BLOCKS 2															// Anzahl der Reserve Blocks, die für Kopiervorgänge gebraucht werden 
// Wear-Leveler ([TC11]- Algorithmus) Konstanten
#define THETA 10															// Definiert die Größe des neutralen Pools	
#define DELTA 10	// Definiert den Bereich für BlockNeutralisationen
//allgemeine Konstante
#define LOG_BLOCK_COUNT ( (BLOCK_COUNT - SPARE_BLOCKS) * BLOCKSEGMENTS) 

/*	Zustände für die physikalische Liste
*	empty =  Speicherzelle beschreibbar
*	assigned =  Speicherzelle benutzt
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
	ready, used, badBlock

} BlockStatus_t;

/*	Datenstruktur für die Blockverwaltung
*	segmentStatus Array mit den Status der einzelnen segmente eines Blocks
*  deleteCounter hält fest wie oft der Block gelöscht wurde
*  invalidCounter hält fest wie viele Segmente in diesem Block invalid markiert sind
*  status hält die Status des Blockes fest -> [BlockStatus_t]
*/
typedef struct Block_struct
{
	uint32_t writePos;
	uint32_t deleteCounter;
	uint32_t invalidCounter;
	BlockStatus_t status;

} Block_t;

/*	Datenstruktur für ein Listenelement
*	prev Pointer auf vorheriges Element
*	next Pointer auf nächstes Element
*	blockNr Position des Blocks in flash_t.blockArray
*/
typedef struct ListElem {
	struct ListElem* prev;
	struct ListElem* next;
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
	double AVG;
	uint32_t blockCounter;
	Block_t* blockArray;
} List_t;

/*	Datenstruktur für den FTL
*	mappingTable Tabelle in der das Mapping gespeichert wird
*  blockArray Array mit Block Datenstruktur zur Verwaltung der Blöcke-> Siehe Block_t
*  invalidCounter Zählt die invaliden Segmente im gesammten FTL
*  activeBlockPosition Aktuelle Schreibposition
*  freeBlocks Anzahl der freien Blocks die zum Schreibzugriff zur verfügung stehen
*	TODO Kommentare zu List_t ergänzen
*/
typedef struct flash_struct
{	
	uint32_t mappingTable[MAPPING_TABLE_SIZE];//[BLOCK_COUNT * PAGES_PER_BLOCK * (PAGE_DATASIZE / LOGICAL_BLOCK_DATASIZE)]; // Übersetzungstabelle
	Block_t blockArray[BLOCK_COUNT]; // Block Verwaltungsstruktur
	uint32_t invalidCounter;	
	uint32_t freeBlocks;
	uint32_t actWriteBlock;
	List_t* hotPool;
	List_t* coldPool;
	List_t* neutralPool;
	double AVG;// globaler AVG
} flash_t;

#endif  /* __FTL_STRUCT__ */ 