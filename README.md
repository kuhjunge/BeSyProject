#Getting Started - Implementation eines FTL's#

##Schritt 1 - Öffnen des Projektes##

Nachdem die *.zip Datei entpackt wurde befindet sich im Ordner “FTL Implementation” das Programm. 
Um das Programm in Visual Studio 2010 zu öffnen, starten Sie als erstes Visual Studio 2010.
Unter Datei - Öffnen - Projekt öffnen wählen Sie im Ordner “FTL Implementation” die Datei “flash.sln”.

##Schritt 2 - Konfiguration der Parameter##

Anschließend gehen Sie auf den Projektmappen-Explorer von Visual Studio und öffnen unter Headerdateien die Datei ftl_structs.h.
Alle Parameter mit dem unser FTL arbeitet wurden in der ftl_structs.h parametrisiert.
Folgende Parameter sind dort hinterlegt:

* die Logische Blockgröße des Betriebssystemes unter LOGICAL_BLOCK_DATASIZE (Defaut: 16)
* die verwendeten Spare Blocks unter SPARE_BLOCKS (Default: 5)
* das Theta nach [TC11] unter THETA (Default: 10)
* das Delta nach [TC11] unter DELTA (Default: 5)
* die Größe des save state unter SAVE_STATE_SIZE (Default: 6 *512)
* der Debug Ausgabe Level unter DEBUG_LEVEL (Default 4)
	* Level 1 -  3 aktivieren Debug ausgaben (1 = alles, 3 = nur das Wichtige)
	* ab Level 4 werden nur noch Fehlermeldung angezeigt
	* ab Level 5 wird keine Ausgabe mehr vom FTL auf der Konsole gemacht

##Schritt 3 - Starten eines Tests##

Im Projektmappen-Explorer findet man unter den Quelldateien die “main.c”. Dort sind bereits einige Testfälle konfiguriert. Diese kann man nach belieben ein und auskommentieren. Anschließend kann man testen ob das Projekt ordnungsmäßig funktioniert.

##Schritt 4 - Nutzen des FTL in einem eigenen Projekt##

###Lesezugriff###
Findet über die Funktion uint8_t readBlock (flash_t *flashDevice, uint32_t index, uint8_t *data) statt.
Liest einen Datenblock an der angegebene Indexposition vom Flashspeicher, der mit der in flashDevice übergebenen Datenstruktur verwaltet wird. flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung dieses Flash-Datenträgers dient. index ist die Nummer des zu lesenden Blocks. data ist ein Pointer auf einen ausreichend großen Datenbereich, in den die zu lesenden Daten kopiert werden. Der Rückgabewert ist als Boolescher Wert zu interpretieren.

###Schreibzugriff###
Findet über die Funktion uint8_t writeBlock (flash_t *flashDevice, uint32_t index, uint8_t *data) statt.
Schreibt einen Datenblock an der angegebene Indexposition auf den Flashspeicher, der mit der in flashDevice übergebenen Datenstruktur verwaltet wird. flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung dieses Flash-Datenträgers dient. index ist die Nummer des zu schreibenden Blocks auf dem Flashdevice data ist ein Pointer auf den Quelldatenblock. Der Rückgabewert ist als Boolescher Wert zu interpretieren.

###Mount###
Findet üer die Funktion flash_t *mount (flashMem_t *flashHardware) statt.
Mounten des Flash-Datenträgers. Dabei sollen die Datenstrukturen des FTL aus dem im Flash abgelegten Zustand erzeugt werden. Nach dem Mounten ist der Datenträger bereit zur Verwendung. flashHardware zeigt auf die Datenstruktur vom Typ flashMem_t, die den FlashDatenträger modelliert. Der Rückgabewert ist ein Pointer auf eine von Ihnen definierte Datenstruktur mit Typnamen flash_t, die bei weiteren Funktionen dieses Interfaces als Parameter übergeben.

###Unmount###
Findet über die Funktion  flash_t *unmount (flash_t *flashDevice) statt.
Unmounten des Flash-Datenträgers. Dabei sollen die Datenstrukturen des FTL in der Repräsentation als Byte-Array auf dem Flash abgelegt werden. Nach dem Unmounten ist eine weitere Verwendung des Datenträgers erst nach erneutem Mounten möglich. flashDevice zeigt auf die Datenstruktur vom Typ flash_t, die zur Verwaltung dieses Flash-Datenträgers dient. Der Rückgabewert ist ein Pointer auf eine Datenstruktur vom Typ flash_t. Im Erfolgsfall wird NULL zurückgegeben, bei einem Fehler kann die Information des Fehlerfalls in der Datenstruktur abgelegt sein. 

>by Chris Deter () & Simon Krause