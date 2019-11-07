#ifndef _BOBFS_H_
#define _BOBFS_H_

#include "ide.h"

class BobFS;

/* In-memory representation of an i-node */
class Node {
public:
	BobFS* fs;
	uint32_t inumber;

	static constexpr uint32_t SIZE = 16;
    static constexpr uint16_t DIR_TYPE = 1;
    static constexpr uint16_t FILE_TYPE = 2;

		
	uint16_t type;
	uint16_t nlinks;
	uint32_t size;
	
	uint32_t dirptr;
	uint32_t idirptr;

    /* Create a Node object representing the given i-node
       in the given file system */
    Node(BobFS* fs, uint32_t inumber);

    uint16_t getType(void);
    uint16_t getLinks(void);
    uint32_t getSize(void);
	uint32_t getDirectPtr(void);
	uint32_t getIndirectPtr(void);


    void setType(uint16_t);
    void setLinks(uint16_t);
    void setSize(uint32_t);
	void setDirectPtr(uint32_t);
	void setIndirectPtr(uint32_t);

    /* read up to "n" bytes and store them in the given "buffer"
       starting at the given "offset" in the file

       returns: number of bytes actually read
                x = 0      ==> end of file
                x < 0      ==> error
                0 > x <= n ==> number of bytes actually read
     */ 
    int32_t readHelp(uint32_t offset, void* buffer, uint32_t n);
    int32_t read(uint32_t offset, void* buffer, uint32_t n);
	void writeBackInode();


    /* like read but promises to read as many bytes as it can */
    int32_t readAll(uint32_t offset, void* buffer, uint32_t n);

    /* writes up to "n" bytes from the given "buffer"
       starting at the given "offset" in the file

       returns: number of bytes actually written
                x = 0      ==> end of file
                x < 0      ==> error
                0 > x <= n ==> number of bytes actually written
     */ 
	uint8_t* getDataBlockFromPtr(uint32_t datablockptr);
    int32_t writeHelp(uint32_t offset, const void* buffer, uint32_t n);
    int32_t write(uint32_t offset, const void* buffer, uint32_t n);
	uint32_t getFreeINumber();
	uint32_t getFreeDataBlock();
	uint32_t makeDataBlock();
	void writeBackBitMap(uint32_t);
	void entry(const char*, uint32_t);

    /* like write but promises to write as many bytes as it can */
    int32_t writeAll(uint32_t offset, const void* buffer, uint32_t n);

    /* If the current node is a directory, create an entry
       with the given information and return a pointer to the
       Node representing the new entry

       returns null if the current Node is not a directory

     */
    Node* newNode(const char* name, uint32_t type);

    /* calls newNode to create a file */
    Node* newFile(const char* name);

    /* calls newNode to create a directory */
    Node* newDirectory(const char* name);

    /* if the current node is a directory, returns a pointer
       the entry with the given name */
    Node* findNode(const char* name);

    bool isFile(void);
    bool isDirectory(void);

    /* Creates a new link to the given node in this directory */
    /* does nothing of the current node is not a directory */
    void linkNode(const char* name, Node* file);

    void dump(const char* name);
    Node* dumpdfs(const char* name);

    static Node* get(BobFS* fs, uint32_t index) {
        Node* n = new Node(fs,index);
        return n;
    }
};


/* In-memory representation of a BobFS file system */
class BobFS {
public:

    static constexpr uint32_t BLOCK_SIZE = 1024;
	Ide* device;
	Node* rt;

	uint8_t superBlock[BLOCK_SIZE];
	uint8_t dataBitMap[BLOCK_SIZE];
	uint8_t inodeBitMap[BLOCK_SIZE];
	

	BobFS(Ide* device);
    virtual ~BobFS();

    /* make a new BobFS file system on the given device */
    static BobFS* mkfs(Ide* device);

    /* mount an existing BobFS file system from the given device */
    static BobFS* mount(Ide* device);

    /* Return a pointer to the root node of the given file system */
    static Node* root(BobFS* fs);

};

#endif
