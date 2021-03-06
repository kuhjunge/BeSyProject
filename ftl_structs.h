#ifndef __FTL_STRUCT__
#define __FTL_STRUCT__

// Konstanten
// Alle vom FTL benutzen Parameter k�nnen im folgenden Abschnitt angepasst werden
//----------------------------------
// Allocator Konstanten
// Logische Blockgr��e des OS
#define LOGICAL_BLOCK_DATASIZE 16

// Cleaner Konstanten
// Anzahl der Reserve Blocks, die f�r Kopiervorg�nge gebraucht werden 
#define SPARE_BLOCKS 5

// Wear-Leveler ([TC11]- Algorithmus) Konstanten
// Definiert die Gr��e des neutralen Pools	
#define THETA 10				
// Definiert den Bereich f�r BlockNeutralisationen
#define DELTA 5	

// Mount Konstanten
// definiert die Gr��e des save_state
#define SAVE_STATE_SIZE 6 * 512

// Debug Konstanten 1 - 3 Debug, 4 Fehler, 5 -nichts ausgeben- 
#define DEBUG_LEVEL 5


// Datenstrukturen
// -----------------------------------

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

/* Datenstruktur f�r die Blockverwaltung
 * writePos gibt an, wo das n�chste (logisch) beschreibbare Segement liegt (steht auf dem letzten Segement, wenn der Block voll ist)
 * deleteCounter h�lt fest wie oft der Block gel�scht wurde
 * invalidCounter h�lt fest wie viele Segmente in diesem Block invalid markiert sind
 * status h�lt die Status des Blockes fest -> [BlockStatus_t]
 */
typedef struct Block_struct
{
	uint16_t writePos;
	uint16_t deleteCounter;
	uint16_t invalidCounter;
	BlockStatus_t status;

} Block_t;

/*	Datenstruktur f�r ein Listenelement
 *	prev Pointer auf vorheriges Element
 *	next Pointer auf n�chstes Element
 *	blockNr Position des Blocks in flash_t.blockArray
 */
typedef struct ListElem {
	struct ListElem *prev;
	struct ListElem *next;
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
	ListElem_t *first;
	ListElem_t *last;
	double AVG;
	uint16_t blockCounter;
	Block_t *blockArray;
} List_t;

/* Datenstruktur f�r den FTL
 * mappingTable Tabelle in der das Mapping gespeichert wird
 * mappingTableRev Umgekehrte Mapping Tabelle (f�r schnelleren Zugriff)
 * blockArray Array mit Block Datenstruktur zur Verwaltung der Bl�cke-> Siehe Block_t
 * invalidCounter Z�hlt die invaliden Segmente im gesammten FTL
 * freeBlocks Anzahl der freien Blocks die zum Schreibzugriff zur verf�gung stehen
 * activeBlock Aktueller Block der bearbeitet wird
 * hotPool Hot Pool f�r Wear Leveling
 * coldPool Cold Pool f�r Wear Leveling
 * neutralPool neutraler Pool f�r das Wear Leveling
 * badBlockCounter Z�hlt die Anzahl der Badblocks im System
 * AVG ist der globale Mittelwert des Wear Levelings
 */
typedef struct flash_struct
{	
	uint32_t *mappingTable; // �bersetzungstabelle
	uint32_t *mappingTableRev;
	Block_t *blockArray; // Block Verwaltungsstruktur
	uint32_t invalidCounter;	
	uint16_t freeBlocks;
	uint16_t actWriteBlock;
	List_t *hotPool;
	List_t *coldPool;
	List_t *neutralPool;
	uint16_t badBlockCounter;
	double AVG;// globaler AVG
} flash_t;

#endif  /*__FTL_STRUCT__ */ 