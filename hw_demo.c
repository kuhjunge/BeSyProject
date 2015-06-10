/* Einfacher Test und Demo zur Verwendung der Simulation eines Flash-Speichers */
#include "flashhardware.h"

uint8_t myState[2*STATEBLOCKSIZE], *statePtr; 
uint8_t myData[16], *dataPtr, myReadData[16];
uint16_t count; 

int main_alt(int argc, char *argv[]) {
	uint16_t i,size; 
	uint8_t ok=FALSE; 
	// Flash Initialisieren 
	FL_resetFlash(); 
	// dummy-Daten generieren
	for (i=0; i<16; i++)
		myData[i]=(uint8_t)(i%10+65); 
	// Block löschen
	FL_deleteBlock(1); 
	// Page darin schreiben 
	count=FL_writeData(1,0,6,4,&myData);
	// Page schreiben ohne Löschen 
	count=FL_writeData(1,0,0,10,&myData);
	// was lesen
	count=FL_readData(1,0,2,6,&myReadData);
	// Page lesen, falsche Parameter
	count=FL_readData(1,1,12,6,&myReadData);
	// Page -Spare schreiben
	count=FL_writeSpare(2,1,2,8,&myData);
	// Page-spare lesen
	count=FL_readSpare(2,1,0,8,&myData);		
	// Dummy-Zustand generieren 
	for (i=0; i<2*STATEBLOCKSIZE; i++)
		myState[i]=(uint8_t)(i%96+48); 
	// zustand schreiben
	FL_saveState(2,myState);
	// Zustand löschen 
	for (i=0; i<2*STATEBLOCKSIZE; i++)
		myState[i]=(uint8_t)(0x00); 
	// Zustand auslesen 
	size=FL_getStateSize(); 
	statePtr=FL_restoreState(myState); 
	// nun Blöcke kaputtlöschen
	// Blöcke löschen
	ok=FL_deleteBlock(0); 
	ok=FL_deleteBlock(2); 
	ok=FL_deleteBlock(0); 
	ok=FL_deleteBlock(1); 
	ok=FL_deleteBlock(2); 
	ok=FL_deleteBlock(0); 
	ok=FL_deleteBlock(1); 
	ok=FL_deleteBlock(2); 
		// nun gehen sie kaputt
	ok=FL_deleteBlock(0); 
	ok=FL_deleteBlock(1); 
	ok=FL_deleteBlock(2); 
	ok=FL_deleteBlock(0); 
	ok=FL_deleteBlock(1); 
	ok=FL_deleteBlock(2); 
	ok=FL_deleteBlock(0); 
	ok=FL_deleteBlock(1); 
	ok=FL_deleteBlock(2); 

	return 1;
}

