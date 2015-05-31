//Testklasse für List.h
#include "ftl.h"
/*
int main(int argc, char *argv[]) {
	flash_t* ssd;
	flashMem_t flMe;
	List_t* p1;
	List_t* p2;
	int i, tmp;
	
	FL_resetFlash(); 
	ssd = mount(&flMe);
	p1 = initList(ssd->blockArray);
	p2 = initList(ssd->blockArray);

	for(i = 0; i < 20; i++){
		ssd->blockArray[i].deleteCounter = rand() % 50;
	}

	for(i = 0; i < 20; i++){
		addBlock(p1, i);		
	}
	printList(p1);

	for(i = 0; i < 20; i++){
		addBlock(p2, getFirstElement(p1)->blockNr);
	}
	printList(p2);

	for(i = 0; i < 20; i++){
		addBlock(p1, getLastElement(p2)->blockNr);
	}
	printList(p1);

	for(i = 0; i < 20; i++){
		delBlock(p1, i);
		addBlock(p2, i);
	}
	printList(p2);

	for(i = 0; i < 20; i++){
		tmp = getFirstElement(p2)->blockNr;
		delBlock(p2, tmp);
		addBlock(p2, tmp);
	}
	printList(p2);

	for(i = 0; i < 20; i++){
		tmp = getLastElement(p2)->blockNr;
		delBlock(p2, tmp);
		addBlock(p2, tmp);
	}
	printList(p2);

	scanf_s(&i);
}*/