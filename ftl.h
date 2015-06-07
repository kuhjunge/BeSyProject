#ifndef __FTL__
#define __FTL__

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "flashhardware.h"
#include "list.h"
#include "types.h"
#include "ftl_structs.h"

// PUBLIC Funktionen
////////////////////////////////////////////////////////////////////
/*
  *Mounten des Flash-Datentr�gers. erzeugt die Datenstrukturen des FTL aus dem im
  *Flash abgelegten Zustand. Nach dem Mounten ist der Datentr�ger bereit
  *zur Verwendung.
  *flashHardware zeigt auf die Datenstruktur vom Typ flashMem_t, die den FlashDatentr�ger
  *modelliert.
  *Der R�ckgabewert ist ein Pointer auf eine von Ihnen definierte Datenstruktur mit
  *Typnamen flash_t, die bei weiteren Funktionen dieses Interfaces als Parameter
  *�bergeben wird, um den Flash-Speicher auszuw�hlen (analog der Datenstruktur FILE f�r
  *Dateien).
 */
flash_t *mount(flashMem_t *flashHardware);

/*
  *Unmounten des Flash-Datentr�gers. Dabei werden die Datenstrukturen des FTL in der
  *Repr�sentation als Byte-Array auf dem Flash abgelegt. Nach dem Unmounten ist
  *eine weitere Verwendung des Datentr�gers erst nach erneutem Mounten m�glich.
  *flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
  *dieses Flash-Datentr�gers dient.
  *Der R�ckgabewert ist ein Pointer auf eine Datenstruktur vom Typ flash_t. Im
  * Erfolgsfall wird NULL zur�ckgegeben(ToDo: ,bei einem Fehler kann die Information des
  *Fehlerfalls in der Datenstruktur abgelegt sein).
 */
flash_t *unmount(flash_t *flashDevice);

/*
  *Liest einen Datenblock an der angegebene Indexposition vom Flashspeicher, der mit der
  *in flashDevice �bergebenen Datenstruktur verwaltet wird.
  *flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
  *dieses Flash-Datentr�gers dient.
  *index ist die Nummer des zu lesenden Blocks.
  *data ist ein Pointer auf einen ausreichend gro�en Datenbereich, in den die zu lesenden
  *Daten kopiert werden.
  *Der R�ckgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t readBlock(flash_t *flashDevice, uint32_t index, uint8_t *data);

/*
  *Schreibt einen Datenblock an der angegebene Indexposition auf den Flashspeicher, der
  *mit der in flashDevice �bergebenen Datenstruktur verwaltet wird.
  *flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung
  *dieses Flash-Datentr�gers dient.
  *index ist die Nummer des zu schreibenden Blocks auf dem Flashdevice
  *data ist ein Pointer auf den Quelldatenblock.
  *Der R�ckgabewert ist als Boolescher Wert zu interpretieren.
 */
uint8_t writeBlock(flash_t *flashDevice,uint32_t index, uint8_t *data);

// PUBLIC DEBUG Funktionen
////////////////////////////////////////////////////////////////////
/*
  *Debug Funktion
  *gibt die Struktur der flash_t Datenstruktur auf der Konsole aus
 */
void printerr(flash_t *flashDevice);

#endif  / *__FTL__ */ 

