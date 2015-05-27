#ifndef __FTL_STRUCT__
#define __FTL_STRUCT__

// Allocator Konstanten
#define LOGICAL_BLOCK_DATASIZE 16													// Logische Blockgr��e des OS
#define BLOCKSEGMENTS (PAGE_DATASIZE * PAGES_PER_BLOCK  / LOGICAL_BLOCK_DATASIZE )  // Speichersegmente pro Block
#define MAPPING_TABLE_SIZE (BLOCK_COUNT * BLOCKSEGMENTS )							
// Cleaner Konstanten
#define START_CLEANING 3															// Anzahl ab der der Cleaning Algorithmus gestartet wird
#define SPARE_BLOCKS 1																// Anzahl der Reserve Blocks zus�tzlich zum active Block
// Wear-Leveler ([TC11]- Algorithmus) Konstanten
#define THETA 5																	// Definiert die Gr��e des neutralen Pools	
#define DELTA 4																	// Definiert den Bereich f�r BlockNeutralisationen

/*	Zust�nde f�r die physikalische Liste
*	empty =  Speicherzelle beschreibbar
*	assigned =  Speicherzelle benutzt
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
	ready, used, badBlock

} BlockStatus_t;

/*	Datenstruktur f�r die Blockverwaltung
*	segmentStatus Array mit den Status der einzelnen segmente eines Blocks
*  deleteCounter h�lt fest wie oft der Block gel�scht wurde
*  invalidCounter h�lt fest wie viele Segmente in diesem Block invalid markiert sind
*  status h�lt die Status des Blockes fest -> [BlockStatus_t]
*/
typedef struct Block_struct
{
	uint32_t writePos;
	uint32_t deleteCounter;
	uint32_t invalidCounter;
	BlockStatus_t status;

} Block_t;

/*	Datenstruktur f�r ein Listenelement
*	prev Pointer auf vorheriges Element
*	next Pointer auf n�chstes Element
*	blockNr Position des Blocks in flash_t.blockArray
*/
typedef struct ListElem {
	struct ListElem* prev;
	struct ListElem* next;
	uint32_t blockNr;
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
	double AVG;
	uint32_t blockCounter;
	Block_t* blockArray;
} List_t;

/*	Datenstruktur f�r den FTL
*	mappingTable Tabelle in der das Mapping gespeichert wird
*  blockArray Array mit Block Datenstruktur zur Verwaltung der Bl�cke-> Siehe Block_t
*  invalidCounter Z�hlt die invaliden Segmente im gesammten FTL
*  activeBlockPosition Aktuelle Schreibposition
*  freeBlocks Anzahl der freien Blocks die zum Schreibzugriff zur verf�gung stehen
*	TODO Kommentare zu List_t erg�nzen
*/
typedef struct flash_struct
{
	uint32_t mappingTable[MAPPING_TABLE_SIZE];//[BLOCK_COUNT * PAGES_PER_BLOCK * (PAGE_DATASIZE / LOGICAL_BLOCK_DATASIZE)]; // �bersetzungstabelle
	Block_t blockArray[BLOCK_COUNT]; // Block Verwaltungsstruktur
	uint32_t invalidCounter;
	uint32_t freeBlocks;
	uint32_t actWriteBlock;
	List_t* hotPool;
	List_t* coldPool;
	List_t* neutralPool;
	List_t* writePool;
	double AVG;// globaler AVG
} flash_t;

#endif  /* __FTL_STRUCT__ */ 