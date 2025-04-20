#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <string.h>
#include <stdlib.h>

typedef struct Frame {
    int currpage; //the corresponding page in the file
    bool dirty;
    int fixCount;
    bool refbit; //true=1 false=0 for clock
    struct Frame *next;
    struct Frame *prev;
    char data[PAGE_SIZE];
   
} Frame;

typedef struct statlist{
    struct statlist *next;
    Frame *fpt; //Frame pt
}statlist;

typedef struct Buffer{ //use as a class
    int numRead; //for readIO
    void *stratData; //sizeof(void)=8 siszeof(int)=4;
    int numFrames; // number of frames in the frame list
    int numWrite; //for writeIO
    //int pinnNum; //pinned frames
    Frame *head;
    statlist *stathead; //statistics functions have to follow true sequence -.-|
    Frame *pointer; //special purposes;init as bfhead;clock used
    Frame *tail;
}Buffer;


/********************************************** Custom Functions***********************************************/
Frame *alreadyPinned(BM_BufferPool *const bm, const PageNumber pageNum)
/* Verifies if the given pageNum is already pinned.
   If found, increments the pin count and returns the Frame pointer.
   Returns NULL if not found. */
{
    Buffer *bufferMgr = bm->mgmtData;   // Renamed for better clarity
    Frame *currentFrame = bufferMgr->head; // Start from the head of the buffer

    do {
        if (currentFrame->currpage == pageNum) {
            currentFrame->fixCount++;  // Increment pin count if page is pinned
            return currentFrame;
        }
        currentFrame = currentFrame->next; // Move to the next frame
    } while (currentFrame != bufferMgr->head); // Loop back to head if not found

    return NULL;  // Return NULL if the page is not pinned
}

int pinThispage(BM_BufferPool *const bm, Frame *frame, PageNumber pageNum)
/* Pins the specified pageNum to the given frame. 
   Handles reading and writing data as necessary. */
{
    Buffer *bufferMgr = bm->mgmtData;
    SM_FileHandle fileHandle;
    RC resultCode;

    // Open the page file
    resultCode = openPageFile(bm->pageFile, &fileHandle);
    if (resultCode != RC_OK) return resultCode;

    // Ensure the pageNum is within file capacity
    resultCode = ensureCapacity(pageNum, &fileHandle);
    if (resultCode != RC_OK) return resultCode;

    // If the frame is dirty, write its content to disk
    if (frame->dirty) {
        resultCode = writeBlock(frame->currpage, &fileHandle, frame->data);
        if (resultCode != RC_OK) return resultCode;

        frame->dirty = false;        // Reset the dirty flag
        bufferMgr->numWrite++;       // Increment write count
    }

    // Read the new page into the frame
    resultCode = readBlock(pageNum, &fileHandle, frame->data);
    if (resultCode != RC_OK) return resultCode;

    bufferMgr->numRead++;            // Increment read count
    frame->currpage = pageNum;       // Update frame with the new page number
    frame->fixCount++;               // Increment the fix count

    closePageFile(&fileHandle);      // Close the page file
    return RC_OK;
}


/* Pinning Functions*/
RC pinFIFO(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum, bool isFromLRU)
/* Pins the first available frame using the FIFO strategy and moves the frame to the tail of the queue.
   If called by LRU, no need to check if the page is already pinned. */
{
    if (!isFromLRU && alreadyPinned(bm, pageNum)) {
        return RC_OK;  // Page is already pinned
    }

    Buffer *bufferMgr = bm->mgmtData;
    Frame *currentFrame = bufferMgr->head;
    bool frameFound = false;

    // Find the first available frame with fix count 0
    do {
        if (currentFrame->fixCount == 0) {
            frameFound = true;
            break;
        }
        currentFrame = currentFrame->next;
    } while (currentFrame != bufferMgr->head);

    if (!frameFound) {
        return RC_IM_NO_MORE_ENTRIES;  // No available frame
    }

    // Pin the page to the frame
    RC resultCode = pinThispage(bm, currentFrame, pageNum);
    if (resultCode != RC_OK) return resultCode;

    page->pageNum = pageNum;
    page->data = currentFrame->data;

    // Adjust the frame list to move the used frame to the tail
    if (currentFrame == bufferMgr->head) {
        bufferMgr->head = currentFrame->next;
    }
    currentFrame->prev->next = currentFrame->next;
    currentFrame->next->prev = currentFrame->prev;

    currentFrame->prev = bufferMgr->tail;
    bufferMgr->tail->next = currentFrame;
    bufferMgr->tail = currentFrame;

    currentFrame->next = bufferMgr->head;
    bufferMgr->head->prev = currentFrame;

    return RC_OK;
}

RC pinLRU(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
/* Pins the page using the LRU strategy. 
   If the page is already pinned, it moves the frame to the tail of the list. 
   Otherwise, it behaves like FIFO. */
{
    Frame *pinnedFrame = alreadyPinned(bm, pageNum);
    if (pinnedFrame) {
        // Adjust frame priority by moving the pinned frame to the tail
        Buffer *bufferMgr = bm->mgmtData;
        if (pinnedFrame == bufferMgr->head) {
            bufferMgr->head = pinnedFrame->next;
        }
        pinnedFrame->prev->next = pinnedFrame->next;
        pinnedFrame->next->prev = pinnedFrame->prev;

        pinnedFrame->prev = bufferMgr->tail;
        bufferMgr->tail->next = pinnedFrame;
        bufferMgr->tail = pinnedFrame;

        pinnedFrame->next = bufferMgr->head;
        bufferMgr->head->prev = pinnedFrame;

        page->pageNum = pageNum;
        page->data = pinnedFrame->data;
    } else {
        // If not pinned, fallback to FIFO pinning
        return pinFIFO(bm, page, pageNum, true);
    }
    return RC_OK;
}

RC pinCLOCK(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
/* Implements the CLOCK page replacement strategy.
   Uses a pointer to scan frames without reordering the queue. */
{
    if (alreadyPinned(bm, pageNum)) {
        return RC_OK;  // Page is already pinned
    }

    Buffer *bufferMgr = bm->mgmtData;
    Frame *currentFrame = bufferMgr->pointer->next;
    bool frameFound = false;

    // Scan frames using CLOCK algorithm
    while (currentFrame != bufferMgr->pointer) {
        if (currentFrame->fixCount == 0) {
            if (!currentFrame->refbit) {  // If refbit is 0
                frameFound = true;
                break;
            }
            currentFrame->refbit = false;  // Reset refbit during the scan
        }
        currentFrame = currentFrame->next;
    }

    if (!frameFound) {
        return RC_IM_NO_MORE_ENTRIES;  // No available frame
    }

    // Pin the page to the selected frame
    RC resultCode = pinThispage(bm, currentFrame, pageNum);
    if (resultCode != RC_OK) {
        return resultCode;
    }

    // Update the CLOCK pointer
    bufferMgr->pointer = currentFrame;

    // Update page details
    page->pageNum = pageNum;
    page->data = currentFrame->data;

    return RC_OK;
}

RC pinLRUK (BM_BufferPool *const bm, BM_PageHandle *const page,
            const PageNumber pageNum)
{
    
    return RC_OK;
}

/************************************Assignment Functions**************************************/

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy,
                  void *stratData)
//initialization: create page frames using circular list; init bm;
{
    //error check
    if (numPages<=0) //input check
        return RC_WRITE_FAILED;
    //init bf:bookkeeping data
    Buffer *bf = malloc(sizeof(Buffer));
    
    if (bf==NULL) return RC_WRITE_FAILED;
    bf->numFrames = numPages;
    bf->stratData = stratData;
    bf->numRead = 0;
    bf->numWrite = 0;
    //create list
    int i;
    Frame *phead = malloc(sizeof(Frame));
    statlist *shead = malloc(sizeof(statlist));
    if (phead==NULL) return RC_WRITE_FAILED;
    phead->currpage=NO_PAGE;
    phead->refbit=false;
    phead->dirty=false;
    phead->fixCount=0;
    memset(phead->data,'\0',PAGE_SIZE);
    shead->fpt = phead;
    
    bf->head = phead;
    bf->stathead = shead;
    
    for (i=1; i<numPages; i++) { //i start from 1
        Frame *pnew = malloc(sizeof(Frame));
        statlist *snew = malloc(sizeof(statlist));
        if (pnew==NULL) return RC_WRITE_FAILED;
        pnew->currpage=NO_PAGE;
        pnew->dirty=false;
        pnew->refbit=false;
        pnew->fixCount=0;
        memset(pnew->data,'\0',PAGE_SIZE);
        
        snew->fpt = pnew;
        shead->next = snew;
        shead = snew;
        
        phead->next=pnew;
        pnew->prev=phead;
        phead=pnew;
    }
    shead->next = NULL;
    bf->tail = phead;
    bf->pointer = bf->head;
    
    //circular list for clock
    bf->tail->next = bf->head;
    bf->head->prev = bf->tail;
    
    //init bm
    bm->numPages = numPages;
    bm->pageFile = (char *)pageFileName;
    bm->strategy = strategy;
    bm->mgmtData = bf;
    
    return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm)
/* Shuts down the buffer pool, writing dirty pages back to disk and releasing allocated resources. */
{
    // Flush all dirty pages to disk
    RC resultCode = forceFlushPool(bm);
    if (resultCode != RC_OK) {
        return resultCode;
    }

    // Free allocated resources
    Buffer *bufferMgr = bm->mgmtData;
    Frame *currentFrame = bufferMgr->head;

    // Deallocate all frames in the circular list
    while (currentFrame != bufferMgr->tail) {
        Frame *nextFrame = currentFrame->next;
        free(currentFrame);
        currentFrame = nextFrame;
    }
    free(bufferMgr->tail);  // Free the last frame
    free(bufferMgr);        // Free the buffer manager

    // Reset the buffer pool's metadata
    bm->numPages = 0;
    bm->pageFile = NULL;
    bm->mgmtData = NULL;

    return RC_OK;
}

RC forceFlushPool(BM_BufferPool *const bm)
/* Writes all dirty pages with a fix count of 0 back to disk, ensuring data consistency. */
{
    Buffer *bufferMgr = bm->mgmtData;
    SM_FileHandle fileHandle;

    // Open the page file
    RC resultCode = openPageFile(bm->pageFile, &fileHandle);
    if (resultCode != RC_OK) {
        return resultCode;
    }

    Frame *currentFrame = bufferMgr->head;

    // Iterate over all frames and write dirty pages to disk
    do {
        if (currentFrame->dirty) {
            resultCode = writeBlock(currentFrame->currpage, &fileHandle, currentFrame->data);
            if (resultCode != RC_OK) {
                closePageFile(&fileHandle);
                return resultCode;
            }
            currentFrame->dirty = false;  // Mark page as clean
            bufferMgr->numWrite++;       // Increment write count
        }
        currentFrame = currentFrame->next;
    } while (currentFrame != bufferMgr->head);

    // Close the page file
    closePageFile(&fileHandle);
    return RC_OK;
}

// Buffer Manager Interface Access Pages
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
/* Marks the frame corresponding to the given page as dirty, indicating that it has been modified. */
{
    Buffer *bufferMgr = bm->mgmtData;
    Frame *currentFrame = bufferMgr->head;

    // Traverse the frame list to find the frame associated with the page
    do {
        if (currentFrame->currpage == page->pageNum) {
            currentFrame->dirty = true;  // Mark the frame as dirty
            return RC_OK;
        }
        currentFrame = currentFrame->next;
    } while (currentFrame != bufferMgr->head);

    // Page not found
    return RC_READ_NON_EXISTING_PAGE;
}

RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
/* Unpins the page from the buffer pool, reducing its fix count. 
   Resets the reference bit if the fix count becomes zero. */
{
    Buffer *bufferMgr = bm->mgmtData;
    Frame *currentFrame = bufferMgr->head;

    // Traverse the frame list to find the frame associated with the page
    do {
        if (currentFrame->currpage == page->pageNum) {
            if (currentFrame->fixCount > 0) {
                currentFrame->fixCount--;  // Decrement the fix count
                if (currentFrame->fixCount == 0) {
                    currentFrame->refbit = false;  // Reset the reference bit
                }
                return RC_OK;
            } else {
                return RC_READ_NON_EXISTING_PAGE;  // Page is already unpinned
            }
        }
        currentFrame = currentFrame->next;
    } while (currentFrame != bufferMgr->head);

    // Page not found in the buffer pool
    return RC_READ_NON_EXISTING_PAGE;
}

RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
/* Writes the given page from the buffer pool to disk, ensuring the page is saved. */
{
    Buffer *bufferMgr = bm->mgmtData;
    SM_FileHandle fileHandle;

    // Open the page file
    RC resultCode = openPageFile(bm->pageFile, &fileHandle);
    if (resultCode != RC_OK) {
        return RC_FILE_NOT_FOUND;
    }

    // Write the page data to disk
    resultCode = writeBlock(page->pageNum, &fileHandle, page->data);
    if (resultCode != RC_OK) {
        closePageFile(&fileHandle);  // Ensure the file is closed on error
        return RC_WRITE_FAILED;
    }

    // Increment the write count in the buffer manager
    bufferMgr->numWrite++;

    // Close the page file
    closePageFile(&fileHandle);
    return RC_OK;
}

RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
/* Pins the specified page in the buffer pool using the appropriate replacement strategy. */
{
    // Validate the page number
    if (pageNum < 0) {
        return RC_IM_KEY_NOT_FOUND;
    }

    // Determine the replacement strategy and pin the page accordingly
    switch (bm->strategy) {
        case RS_FIFO:
            return pinFIFO(bm, page, pageNum, false);
        case RS_LRU:
            return pinLRU(bm, page, pageNum);
        case RS_CLOCK:
            return pinCLOCK(bm, page, pageNum);
        case RS_LRU_K:
            return pinLRUK(bm, page, pageNum);
        default:
            return RC_IM_KEY_NOT_FOUND;  // Unknown replacement strategy
    }
}

PageNumber *getFrameContents (BM_BufferPool *const bm)
{
    // Allocate memory to store the current page numbers for all frames
    PageNumber *frameContents = calloc(bm->numPages, sizeof(int));

    // Access buffer metadata and the head of the statistics list
    Buffer *bufferInfo = bm->mgmtData;
    statlist *currentStatNode = bufferInfo->stathead;

    // Iterate through the list to populate the frame contents
    for (int pageIndex = 0; pageIndex < bm->numPages; pageIndex++)
    {
        frameContents[pageIndex] = currentStatNode->fpt->currpage;
        currentStatNode = currentStatNode->next;
    }

    // Return the array with the frame contents
    return frameContents;
}

bool *getDirtyFlags (BM_BufferPool *const bm)
{
    // Allocate memory for storing dirty flags for all pages
    bool *dirtyFlagArray = calloc(bm->numPages, sizeof(bool));

    // Retrieve buffer metadata and the statistics list
    Buffer *bufferInfo = bm->mgmtData;
    statlist *currentStatNode = bufferInfo->stathead;

    // Iterate over the pages to populate the dirty flags
    for (int pageIndex = 0; pageIndex < bm->numPages; pageIndex++)
    {
        if (currentStatNode->fpt->dirty)
        {
            dirtyFlagArray[pageIndex] = true;
        }
        currentStatNode = currentStatNode->next;
    }

    // Return the array containing the dirty flags
    return dirtyFlagArray;
}

int *getFixCounts (BM_BufferPool *const bm)
{
    // Allocate memory to store fix counts for all pages
    PageNumber *fixCountArray = calloc(bm->numPages, sizeof(int));

    // Retrieve the buffer and statistics list
    Buffer *bufferMetadata = bm->mgmtData;
    statlist *currentStat = bufferMetadata->stathead;

    // Iterate through pages to extract fix counts
    for (int pageIndex = 0; pageIndex < bm->numPages; pageIndex++)
    {
        fixCountArray[pageIndex] = currentStat->fpt->fixCount;
        currentStat = currentStat->next;
    }

    // Return the populated fix count array
    return fixCountArray;
}

int fetchReadIOCount (BM_BufferPool *const bufferPool)
{
    // Access the buffer metadata
    Buffer *bufferManager = bufferPool->mgmtData;

    // Retrieve and return the read I/O count
    return bufferManager->numRead;
}

int fetchWriteIOCount (BM_BufferPool *const bufferPool)
{
    // Extract the buffer manager metadata
    Buffer *bufferManagerData = bufferPool->mgmtData;

    // Return the number of write operations
    return bufferManagerData->numWrite;
}
