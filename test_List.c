//Testklasse für List.h
#include "ftl.h"


int main(int argc, char *argv[]) {
	flash_t* ssd;
	flashMem_t flMe;
	List_t* p1;
	List_t* p2;
	int i, tmp;
	
	FL_resetFlash(); 	
	ssd = (flash_t*)malloc(sizeof(flash_t));
	for (i = 0; i < 20; i++){			
		ssd->blockArray[i].invalidCounter = 0;
		ssd->blockArray[i].deleteCounter = 0;
		ssd->blockArray[i].writePos = 0;
		ssd->blockArray[i].status = ready;
	}
	p1 = initList(ssd->blockArray);
	p2 = initList(ssd->blockArray);

	for(i = 0; i < 20; i++){
		ssd->blockArray[i].deleteCounter = i;//rand() % 50;
	}

	for(i = 0; i < 20; i++){
		if(	addBlock(p1, i) == FALSE){
			printf("fehler\n");
		}
		else{
			printf("i=%i\n",i);
		}
	}
	printList(p1);

	for(i = 0; i < 20; i++){
		addBlock(p2, getFirstBlock(p1));
	}
	printList(p2);

	for(i = 0; i < 20; i++){
		addBlock(p1, getLastBlock(p2));
	}
	printList(p1);

	for(i = 0; i < 20; i++){
		delBlock(p1, i);
		addBlock(p2, i);
	}
	printList(p2);

	for(i = 0; i < 20; i++){
		tmp = getFirstBlock(p2);
		delBlock(p2, tmp);
		addBlock(p2, tmp);
	}
	printList(p2);

	for(i = 0; i < 20; i++){
		tmp = getLastBlock(p2);
		delBlock(p2, tmp);
		addBlock(p2, tmp);
	}
	printList(p2);

	for(i = 0; i < 20; i++){
		tmp = getLastBlock(p2);		
		addBlock(p1, tmp);
	}
	printList(p1);

	for(i = 0; i < 1000; i++){
		tmp = rand() % 20;
		if( delBlock(p1, tmp) )
			addBlock(p1, tmp);
	}	
	printList(p1);

	scanf_s(&i);
}