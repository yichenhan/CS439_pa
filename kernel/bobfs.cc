#include "bobfs.h"
#include "libk.h"
#include "heap.h"
#include "debug.h"

uint8_t zero_1024[BobFS::BLOCK_SIZE];

bool streq(const char* a, const char* b) {
	int i = 0;
    while (true) {
        char x = a[i];
        char y = b[i];
        if (x != y) return false;
        if (x == 0) return true;
        i++;
    }
}

bool Node::isDirectory(void) {
    return this->getType() == DIR_TYPE;
}

bool Node::isFile(void) {
    return this->getType() == FILE_TYPE;
}

///////////
//getters//
///////////
uint16_t Node::getType(void) {
    uint16_t type = 0;
    this->fs->device->readAll(3*1024 + 16 * inumber, &type, 2);
    return type;
}
uint16_t Node::getLinks(void) {
    uint16_t nlinks = 0;
    this->fs->device->readAll(3*1024 + 16 * inumber + 2, &nlinks, 2);
    return nlinks;
}
uint32_t Node::getSize(void) {
    uint32_t size = 0;
    this->fs->device->readAll(3*1024 + 16 * inumber + 4, &size, 4);
    return size;
}
uint32_t Node::getDirectPtr(void) {
    uint32_t dirptr = 0;
    this->fs->device->readAll(3*1024 + 16 * inumber + 8, &dirptr, 4);
    return dirptr;
}
uint32_t Node::getIndirectPtr(void) {
    uint32_t idirptr = 0;
    this->fs->device->readAll(3*1024 + 16 * inumber + 12, &idirptr, 4);
    return idirptr;
}

///////////
//setters//
///////////
void Node::setType(uint16_t type) {
    this->fs->device->writeAll(3*1024 + 16 * inumber, &type, 2);
}
void Node::setLinks(uint16_t nlinks) {
    this->fs->device->writeAll(3*1024 + 16 * inumber + 2, &nlinks, 2);
}
void Node::setSize(uint32_t size) {
    this->fs->device->writeAll(3*1024 + 16 * inumber + 4, &size, 4);
}
void Node::setDirectPtr(uint32_t dirptr) {
    this->fs->device->writeAll(3*1024 + 16 * inumber + 8, &dirptr, 4);
}
void Node::setIndirectPtr(uint32_t idirptr) {
    this->fs->device->writeAll(3*1024 + 16 * inumber + 12, &idirptr, 4);
}
Node* Node::dumpdfs(const char* name) {
	uint32_t type = getType();
    switch (type) {
    case DIR_TYPE:
        {
            uint32_t sz = getSize();
            uint32_t offset = 0;

            while (offset < sz) {
                uint32_t ichild = 0;
                readAll(offset,&ichild,4);
                offset += 4;
                uint32_t len = 0;
                readAll(offset,&len,4);
                offset += 4;
                char* ptr = (char*) malloc(len+1);
                readAll(offset,ptr,len);
                offset += len;
                ptr[len] = 0;              
                
                Node* child = Node::get(this->fs,ichild);
				const char* check = (const char*)ptr;
				if(streq(check, name)){
					return child;
				}
                child->dumpdfs(ptr);
                free(ptr);
            }
        }
        break;
    case FILE_TYPE:
        break;
	}
	return nullptr;
}

void Node::dump(const char* name) {
	uint32_t type = getType();
    switch (type) {
    case DIR_TYPE:
        Debug::printf("*** 0 directory:%s(%d)\n",name,getLinks());
        {
            uint32_t sz = getSize();
            uint32_t offset = 0;
            while (offset < sz) {
                uint32_t ichild = 0;
                readAll(offset,&ichild,4);
                offset += 4;
                uint32_t len = 0;
                readAll(offset,&len,4);
                offset += 4;
                char* ptr = (char*) malloc(len+1);
                readAll(offset,ptr,len);
                offset += len;
                ptr[len] = 0;              
                
                Node* child = Node::get(fs,ichild);
				child->dump(ptr);
                free(ptr);
            }
        }
        break;
    case FILE_TYPE:
        Debug::printf("*** 0 file:%s(%d,%d)\n",name,getLinks(),getSize());
        break;
    default:
        Debug::panic("unknown i-node type %d\n",type);
    }
}



//////////
// Node //
//////////
void copyInode(uint16_t* dataArray, Node* inode){
	//size 8	
	uint16_t type = dataArray[0];
	uint16_t nlinks = dataArray[1];
	uint32_t size = dataArray[3]<<16 | dataArray[2];	
	
	uint32_t dirptr = dataArray[5]<<16 | dataArray[4];	
	uint32_t idirptr = dataArray[7]<<16 | dataArray[6];	

	inode->type = type;
	inode->nlinks = nlinks;
	inode->size = size;
	inode->dirptr = dirptr;
	inode->idirptr = idirptr;
}

uint16_t* getInodeData(Ide* device, uint32_t inumber){
	uint16_t* idata = new uint16_t[8];
	device->readAll((3*1024) + (16*inumber), idata, 16);
	//of size 8 array
	return idata;
}

void copyBlock(BobFS* fs, uint8_t* data, uint8_t arg){
	if(arg == 1){
		for(int i = 0; i < 1024; i++){
			fs->superBlock[i] = data[i];
		}
	}
	else if(arg == 2){
		for(int i = 0; i < 1024; i++){
			fs->dataBitMap[i] = data[i];
		}
	}
	else if(arg == 3){
		for(int i = 0; i < 1024; i++){
			fs->inodeBitMap[i] = data[i];
		}
	}
}

Node::Node(BobFS* fs, uint32_t inumber) {
	this->fs = fs;
	this->inumber = inumber;
	
	//Dont need this
	uint16_t* idata = new uint16_t[8];
	idata = getInodeData(fs->device, inumber);
	copyInode(idata, this);
}

bool getCheck(uint8_t row, int index){
	return !((row>>(index)) & 0x1);
}

uint32_t Node::getFreeINumber(){
	uint32_t inumber = 0;
	for(int i = 0; i < 1024; i++){
		uint8_t row = fs->inodeBitMap[i];
		for(int j = 0; j < 8; j++){
			if(getCheck(row, j)){
				fs->inodeBitMap[i] = row | (1<<j);
				fs->device->writeAll(2048, fs->inodeBitMap, 1024);
				return inumber;
			}
			else inumber++;
		}
	}	
	Debug::panic("No More!");
	return 1000690001;
}
uint32_t Node::getFreeDataBlock(){
	uint32_t datanumber  = 0;
	for(int i = 0; i < 1024; i++){
		uint8_t row = fs->dataBitMap[i];
		for(int j = 0; j < 8; j++){
			if(getCheck(row, j)){
				fs->dataBitMap[i] = row | (1<<j);
				fs->device->writeAll(1024, fs->dataBitMap, 1024);
				return datanumber;
			}
			else datanumber++;
		}
	}	
	Debug::panic("No More!");
	return 99999999;
}

Node* Node::findNode(const char* name) {
    return dumpdfs(name);
}

uint8_t* Node::getDataBlockFromPtr(uint32_t datablockptr){
	uint32_t offset = (datablockptr)*1024;
	uint8_t* databuffer = new uint8_t[1024];
	fs->device->readAll(offset, databuffer, 1024);
	return databuffer;
}

uint32_t Node::makeDataBlock(){
	//returns Data block
	uint32_t dNumber = getFreeDataBlock();	
	//zero out the data block
	uint32_t offset = (dNumber+131) * 1024;
	fs->device->writeAll(offset, &zero_1024, 1024);
	return dNumber+131;
	
}


int32_t Node::writeHelp(uint32_t offset, const void* buffer, uint32_t n) {
	uint32_t start = offset % 1024;
	uint32_t blockn = offset/1024;
	uint32_t nbytes = n;
	
	if(1024-start > n) nbytes = n;
	else nbytes = 1024-start;

	int32_t ret = 0;
	if(blockn == 0){
		if(this->dirptr == 0){
			this->dirptr = makeDataBlock();
			Debug::printf("writeHelp dirptr:::::: %d\n", this->dirptr);
		}
		ret = fs->device->writeAll((dirptr)*1024 + start, buffer, nbytes);	

	}
	else{
		if(this->idirptr == 0){
			//there is no indirect block
			this->idirptr = makeDataBlock();
		}
		//does the data block exist
		uint32_t* dBlock = (uint32_t*)getDataBlockFromPtr(idirptr);
		if(dBlock[blockn-1] == 0){
			//allocate a data block
			dBlock[blockn-1] = makeDataBlock();
		}
		ret = fs->device->writeAll((dBlock[blockn-1])*1024 + start, buffer, nbytes);
		fs->device->writeAll(idirptr*1024, dBlock, 1024);
	}
	
	if(offset + ret > size) this->size += ret;
	
	this->writeBackInode();

	return ret;
}


int32_t Node::write(uint32_t offset, const void* buffer, uint32_t n) {
	return writeHelp(offset, buffer, n);
}

int32_t Node::writeAll(uint32_t offset, const void* buffer_, uint32_t n) {
	if(size < offset) size = offset;
	
	uint32_t red = 0;
	uint32_t currentWrite = -1;
	while(n > 0 && currentWrite > 0){
		//check how many bytes it has read
		currentWrite = writeHelp(offset, buffer_, n);
		red += currentWrite;
		buffer_ = &((uint8_t*)buffer_)[currentWrite];
		offset += currentWrite;
		n-=currentWrite;
	}
	return red;
}

int32_t Node::readHelp(uint32_t offset, void* buffer, uint32_t n) {
	uint32_t start = offset % 1024;
	uint32_t blockn = offset/1024;
	uint32_t nbytes = n;
	if((1024-start) > n) nbytes = n;
	else nbytes = 1024-start;

			
	if(blockn < 1){
		return fs->device->readAll(dirptr*1024 + start, buffer, nbytes);
	}
	else{
		uint32_t datablock = 0;
		blockn-=1;
		fs->device->readAll(idirptr*1024 + blockn*4, &datablock, 4);
		return fs->device->readAll(datablock*1024 + start, buffer, nbytes);
	}
}

int32_t Node::read(uint32_t offset, void* buffer, uint32_t n) {
	return readHelp(offset, buffer, n);
}

int32_t Node::readAll(uint32_t offset, void* buffer_, uint32_t n) {
	if(getSize() == 0) return 0;
	
	if(offset+n > getSize()){
	   	n = getSize()-offset;
	}
	uint32_t red = 0;
	uint32_t currentread = -1;
	while(n > 0 && currentread > 0){
		//check how many bytes it has read
		currentread = Node::readHelp(offset, buffer_, n);
		red += currentread;
		buffer_ = &((uint8_t*)buffer_)[currentread];
		offset += currentread;
		n-=currentread;
	}
	return red;
}

uint32_t len(const char* string){
    uint32_t index = 0;
    while(string[index] != 0) index++;
    return index;
}	


void Node::entry(const char* name, uint32_t inumber){
	uint32_t length = len(name);
	uint32_t* ent = new uint32_t[2];
	ent[0] = inumber;
	ent[1] = length;
	uint32_t curSize = getSize();	
	this->writeAll(curSize, ent, 8);
	this->writeAll(curSize+8, name, length);
}

void Node::linkNode(const char* name, Node* node) {
	if(this->isFile()) return;
	node->setLinks(node->getLinks() + 1);//   ->nlinks++;	
	//Debug::printf("sizeee %d\n" , size);
	this->entry(name, node->inumber);	
	//node->writeBackInode();
	this->writeBackInode();
	node->writeBackInode();
}

Node* Node::newNode(const char* name, uint32_t type) {
	if(isFile()) return nullptr;
	
	uint32_t inumber = getFreeINumber();    
	Node* newNode = new Node(fs, inumber);	
	newNode->type = type;
	newNode->size = 0;
	newNode->nlinks = 0;
	newNode->dirptr = 0;
	newNode->idirptr = 0;

	newNode->setType(type);
	newNode->setLinks(0);
	newNode->setSize(0);
	newNode->setDirectPtr(0);
	newNode->setIndirectPtr(0);

	this->linkNode(name, newNode);


	newNode->writeBackInode();
	return newNode;
}

Node* Node::newFile(const char* name) {
	return newNode(name, FILE_TYPE);
}

Node* Node::newDirectory(const char* name) {
	return newNode(name, DIR_TYPE);
}


Node* getInodeFromDisk(uint32_t inumber, BobFS* fs, Ide* device){
	Node* inode = new Node(fs, inumber);
	//read the root inode from the number
	uint16_t* data = new uint16_t[8];
	data = getInodeData(device, inumber);	
	copyInode(data, inode);
	return inode;	
}	

///////////
// BobFS //
///////////
Node* BobFS::root(BobFS* fs) {
	return fs->rt;
}
BobFS::BobFS(Ide* device) {
	this->device = device;
}
//destructor
BobFS::~BobFS() {}

BobFS* BobFS::mount(Ide* device) {
	uint64_t rootnumber = 0;
	device->readAll(8, &rootnumber, 4);
	BobFS* fs = new BobFS(device);	
	Node* root = getInodeFromDisk(rootnumber, fs, device);
	
	uint8_t* superblock = new uint8_t[BLOCK_SIZE];
	uint8_t* dataBitMap = new uint8_t[BLOCK_SIZE];
	uint8_t* inodeBitMap = new uint8_t[BLOCK_SIZE];

	device->readAll(0, superblock, BLOCK_SIZE);
	device->readAll(1024, dataBitMap, BLOCK_SIZE);
	device->readAll(2048, inodeBitMap, BLOCK_SIZE);

	copyBlock(fs, superblock, 1);	
	copyBlock(fs, dataBitMap, 2);	
	copyBlock(fs, inodeBitMap, 3);	
	root->type = 1;

	fs->rt = root;	
	return fs;
}

void Node::writeBackInode(){
	uint16_t* dataArray = new uint16_t[8];
	dataArray[0] = this->type;
	dataArray[1] = this->nlinks;
	((uint32_t*)dataArray)[1] = this->size;
	((uint32_t*)dataArray)[2] = this->dirptr;
	((uint32_t*)dataArray)[3] = this->idirptr;
	fs->device->writeAll((3*1024) + 16*(this->inumber), dataArray, 16);
}

BobFS* BobFS::mkfs(Ide* device) {
	const char* magicbits = "BOBFS439";
	device->writeAll(0, magicbits, 8);
	//write root number
	uint32_t rootnu = 0;
	device->writeAll(8, &rootnu, 4);
	//clear BitMaps
	device->writeAll(1024, &zero_1024, BLOCK_SIZE);
	device->writeAll(2048, &zero_1024, BLOCK_SIZE);


	BobFS* fs = mount(device);

	//make a root
	Node* rt = new Node(fs, 0);
	rt->inumber = rt->getFreeINumber();


	rt->type = 1;
	rt->nlinks = 1;
	rt->size = 0;
	rt->dirptr = 0;
	rt->idirptr = 0;
	
	rt->setType(1);
	rt->setLinks(1);
	rt->setSize(0);
	rt->setDirectPtr(0);
	rt->setIndirectPtr(0);

	fs->rt = rt;

	return fs;
}
