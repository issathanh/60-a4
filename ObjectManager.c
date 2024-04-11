#include "ObjectManager.h"

// You can add other header files if needed

// tracks the next reference (ID) to use, we start at 1 so we can use 0 as the NULL reference
static Ref nextRef = 1;

// A Memblock holds the relevent information associated with an allocated block of memory by our memory manager
typedef struct MEMBLOCK MemBlock;


// information needed to track our objects in memory
struct MEMBLOCK
{
  int numBytes;    // how big is this object?
  int startAddr;   // where the object starts
  Ref ref;         // the reference used to identify the object
  int count;       // the number of references to this object
  MemBlock *next;  // pointer to next block.  Blocks stored in a linked list.
};


// The blocks are stored in a linked list where the start of the list is pointed to by memBlockStart.
static MemBlock *memBlockStart; // start of linked list of blocks allocated
static MemBlock *memBlockEnd;   // end of linked list of blocks allocated
static int numBlocks;            // number of blocks allocated

// our buffers.  This is where we allocate memory from.  One of these is always the current buffer.  The other is used for swapping
//during compaction stage.

static unsigned char buffer1[MEMORY_SIZE];
static unsigned char buffer2[MEMORY_SIZE];

// points to the current buffer containing our data
static unsigned char *currBuffer = buffer1;

// points to the location of the next available memory location
static int freeIndex = 0;

// performs required setup
void initPool()
{
  memBlockStart = NULL;
  memBlockEnd = NULL;
  numBlocks = 0;
  currBuffer = buffer1; // Start with buffer1 as the active buffer
  freeIndex = 0; // Reset free index to the start of the buffer
}

// performs required clean up
void destroyPool()
{
   MemBlock *current = memBlockStart;
  while (current != NULL) {
    MemBlock *next = current->next;
    free(current); // Assuming dynamic allocation, adjust as necessary
    current = next;
  }
  memBlockStart = memBlockEnd = NULL;
  numBlocks = 0;
  freeIndex = 0; // Reset for safety
}

// Adds the given object into our buffer. It will fire the garbage collector as required.
// We always assume that an insert always creates a new object...
// On success it returns the reference number for the block allocated.
// On failure it returns NULL_REF (0)
Ref insertObject(const int size) {
  if (MEMORY_SIZE - freeIndex < size) {
    compact();
    if (MEMORY_SIZE - freeIndex < size) // Check again after compaction
      return NULL_REF;
  }

  // Allocate the object
  Ref ref = nextRef++;
  MemBlock *block = malloc(sizeof(MemBlock)); // Adjust based on your allocation strategy
  block->numBytes = size;
  block->startAddr = freeIndex;
  block->ref = ref;
  block->count = 1; // Initial reference count
  block->next = NULL;

  // Insert into the linked list
  if (memBlockEnd != NULL) {
    memBlockEnd->next = block;
  } else {
    memBlockStart = block;
  }
  memBlockEnd = block;
  numBlocks++;

  // Update free index
  freeIndex += size;

  return ref;
}


// returns a pointer to the object being requested
void *retrieveObject(const Ref ref) {
  MemBlock *current = memBlockStart;
  while (current != NULL) {
    if (current->ref == ref) {
      return (void *)&currBuffer[current->startAddr];
    }
    current = current->next;
  }
  return NULL;
}

// update our index to indicate that we have another reference to the given object
void addReference(const Ref ref) {
  MemBlock *current = memBlockStart;
  while (current != NULL) {
    if (current->ref == ref) {
      current->count++;
      return;
    }
    current = current->next;
  }
}

// update our index to indicate that a reference is gone
void dropReference( const Ref ref )
{  
  //write your code here
}

// performs our garbage collection
void compact()
{
  // Assume we're using buffer1 and buffer2, with buffer1 being the current buffer.
    // First, reset the freeIndex for the new buffer and select it as the current buffer.
    unsigned char *newBuffer = (currBuffer == buffer1) ? buffer2 : buffer1;
    int newFreeIndex = 0;

    MemBlock *current = memBlockStart;
    MemBlock *prev = NULL;

    while (current != NULL) {
        if (current->count == 0) { // If the object is not referenced, skip it.
            // Remove the current block from the list
            MemBlock *toDelete = current;
            if (prev != NULL) {
                prev->next = current->next;
            } else {
                memBlockStart = current->next;
            }
            if (current == memBlockEnd) {
                memBlockEnd = prev;
            }
            current = current->next;

            free(toDelete); // Adjust based on your allocation strategy
            numBlocks--;
        } else {
            // Move the object to the new buffer
            memcpy(newBuffer + newFreeIndex, currBuffer + current->startAddr, current->numBytes);
            current->startAddr = newFreeIndex; // Update the start address in the new buffer
            newFreeIndex += current->numBytes;

            prev = current;
            current = current->next;
        }
    }

    // Switch the current buffer to the new buffer and reset the freeIndex
    currBuffer = newBuffer;
    freeIndex = newFreeIndex;
}

void dumpPool()
{
  printf("Current Memory Usage:\n");
    MemBlock *current = memBlockStart;
    while (current != NULL) {
        printf("Block Ref: %lu, Size: %d, Start Addr: %d, Ref Count: %d\n",
               current->ref, current->numBytes, current->startAddr, current->count);
        current = current->next;
    }
    printf("Total Blocks: %d, Free Index: %d\n", numBlocks, freeIndex);
}

//you may add additional function if needed