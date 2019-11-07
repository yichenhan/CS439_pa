#include "sys.h"
#include "stdint.h"
#include "idt.h"
#include "debug.h"
#include "machine.h"

ssize_t write(int fd, void* buf, size_t nbyte){
	char* string = (char*) buf;
	for(size_t i = 0; i < nbyte; i++){
		Debug::printf("%c", string[i]);
	}
	return nbyte;
}

void exit(int status){
	Debug::shutdown();
}

extern "C" int sysHandler(uint32_t eax, uint32_t *frame) {
	uint32_t* esp = (uint32_t*)frame[3];
	if(eax == 0){
		exit((int)esp[1]);
		return 0;
	}
	else{
		return write((int)esp[1], (void*)esp[2], (size_t)esp[3]);
	}
}

void SYS::init(void) {
    IDT::trap(48,(uint32_t)sysHandler_,3);
}
