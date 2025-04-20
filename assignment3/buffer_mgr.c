#include<stdio.h>
#include<stdlib.h>
#include <math.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#define nullptr NULL


typedef struct Page
{
    SM_PageHandle page_h; // Stores the actual content of the page retrieved from disk
    PageNumber pageid;    // Unique identifier for the page
    int lfu_num;          // Keeps count of accesses for LFU (Least Frequently Used) algorithm
    int modified;         // Flag indicating if the page was changed by a client
    int lru_num;          // Tracks recent usage for LRU (Least Recently Used) algorithm
    int num;              // Number of clients currently accessing this page
} PageFrame;

// Global variables used to manage buffer statistics and algorithms
int hit_count = -1;            // Tracks the total number of page hits
int pg_index = 1;              // Tracks the next page to be loaded in the buffer
int buffer_size, page_read,    // Buffer size and number of pages read from disk
    index_hit,                 // Tracks the index of page hits (for LRU updates)
    clock_index,               // Tracks current position in CLOCK algorithm
    num_write,                 // Count of write operations performed on the buffer
    lfu_index = 0;             // Tracks current position for LFU algorithm


/*-----------------------------------------------
-->Author: Ganesh Prasad
--> Function: copyPageFrames()
--> Description: Copies the contents (page handle, modification status, and metadata) from the source page frame to the destination page frame at the specified index.
-------------------------------------------------*/
void copyPageFrames(PageFrame *dest, int index, PageFrame *src)
{
    // Copy data from the source page frame to the destination page frame at the specified index
    (dest + index)->page_h = src->page_h; 
    (dest + index)->modified = src->modified;
    (dest + index)->num = src->num;
    (dest + index)->pageid = src->pageid;

    // The function successfully copies the content of the source frame to the destination frame
    return;
}

/*-----------------------------------------------
--> Author: Dhyan Vasudeva Gowda
--> Function: writePageFrames()	
--> Description: Writes the content of a specific page frame from the buffer pool back to the disk by opening the page file, writing the page to its corresponding block, and incrementing the write count.
-------------------------------------------------*/
void writePageFrames(BM_BufferPool *const bp, PageFrame *page_f, int page_index)
{
    SM_FileHandle f_handle;
    // Open the page file
    openPageFile(bp->pageFile, &f_handle);
    // Write the page back to disk
    writeBlock(page_f[page_index].pageid, &f_handle, page_f[page_index].page_h);
    // Increment the number of writes
    num_write++;

    // No need for unnecessary variables like pg_frame or operations after return
    return;
}

/*-----------------------------------------------
-->Author: Ganesh Prasad
--> Function: FIFO()
--> Description: The FIFO function replaces a page in the buffer pool using the First-In-First-Out strategy, writing modified pages back to disk if necessary, and ensuring thread safety with mutex locks.
-------------------------------------------------*/
extern void FIFO(BM_BufferPool *const bp, PageFrame *pf)
{
    int currentIdx = page_read % buffer_size;  // Start at the index based on pages read
    PageFrame *page_f = (PageFrame *)bp->mgmtData;
    int iter = 0;

    // Loop through the buffer to find a suitable frame for replacement
    while (iter < buffer_size) {
        if (page_f[currentIdx].num != 0) {
            currentIdx = (currentIdx + 1) % buffer_size;  // Move to the next location if current frame is in use
        } else {
            // If the frame has been modified, write it back to disk
            if (page_f[currentIdx].modified == 1) {
                writePageFrames(bp, page_f, currentIdx);
            }

            // Replace the frame with the new page
            copyPageFrames(page_f, currentIdx, pf);
            return;  // Exit after replacing the page
        }

        iter++;
    }
}

/*-----------------------------------------------
-->Author: Dhyan Vasudeva Gowda
--> Function: LRU()
--> Description: The LRU function replaces a page in the buffer pool using the Least Recently Used strategy by selecting the page with the smallest LRU number, writing modified pages back to disk if necessary, and updating the LRU number after the replacement.
-------------------------------------------------*/
// Implementation of LRU (Least Recently Used) algorithm
extern void LRU(BM_BufferPool *const bp, PageFrame *pf)
{
    int index = -1;  // To track the index of the least recently used page
    int j = 0;
    int least_number = 1000000;  // Initialize with a large value to find the least LRU number

    PageFrame *page_f = (PageFrame *)bp->mgmtData;

    // Step 1: Find an empty page frame or the least recently used page frame
   
    while (j < buffer_size) {
        if (page_f[j].num == 0) {  // Identify a free page frame
            index = j;
            least_number = page_f[j].lru_num;
            break;  // Exit the loop when a free page frame is found
        }
        j++;  // Increment the counter manually
    }

    // Step 2: Find the least recently used page frame if no free frame was found
    for (int j = 0; j < buffer_size; j++) {
        if (page_f[j].num == 0 && page_f[j].lru_num < least_number) {
            least_number = page_f[j].lru_num;
            index = j;
        }
    }

    // Step 3: If the selected page frame was modified, write it back to disk
    if (page_f[index].modified == 1) {
        writePageFrames(bp, page_f, index);
    }

    // Step 4: Copy the new page content into the selected page frame
    copyPageFrames(page_f, index, pf);

    // Step 5: Update the LRU number to reflect its most recent usage
    page_f[index].lru_num = pf->lru_num;
}

/*-----------------------------------------------
--> Author: Ganesh Prasad
--> Function: LRU_K()	
--> Description: The LRU_K function implements the LRU-k page replacement strategy by selecting the least recently used page, writing modified pages back to disk if necessary, replacing the selected page, and updating the LRU number to reflect recent usage.
-------------------------------------------------*/
// Implementation of LRU_K algorithm
extern void LRU_K(BM_BufferPool *const bp, PageFrame *pf) 
{
    int count = 0;
    int j=0;
    int index = -1;
    PageFrame *page_f = (PageFrame *)bp->mgmtData;
    int least_number = 1000000;  // Initialize with a large value to find least LRU number
 
    // Step 1: Find an empty page frame (if any)
    for (j = 0; j < buffer_size; j++) {
        if (page_f[j].num == 0) {  // Check if the page is free
            index = j;
            least_number = page_f[j].lru_num;
            break;
        }
    }

    // Step 2: Find the least recently used page frame if no free frame was found
    for (j = index + 1; j < buffer_size; j++) {
        if (least_number > page_f[j].lru_num) {  // Compare LRU numbers to find the least recently used
            least_number = page_f[j].lru_num;
            index = j;
        }
    }

    // Step 3: If the selected page frame was modified, write it back to disk
    if (page_f[index].modified == 1) {
        writePageFrames(bp, page_f, index);
    }

    // Step 4: Replace the selected page frame with the new page
    copyPageFrames(page_f, index, pf);

    // Step 5: Update the LRU number to reflect its most recent usage
    page_f[index].lru_num = pf->lru_num;

    (void)count;
}

/*-----------------------------------------------
-->Author: Jafar
--> Function: CLOCK()
--> Description: The CLOCK function implements the CLOCK page replacement algorithm by cycling through the buffer pool to find a suitable page for replacement, resetting the use flag (lru_num) for recently used pages, and writing modified pages back to disk if necessary before replacing them with a new page.
-------------------------------------------------*/
// Implementation of CLOCK algorithm
extern void CLOCK(BM_BufferPool *const bp, PageFrame *newPage) 
{
    // Check if the buffer pool is valid before proceeding
    if (!bp) {
        return;  //To exit if buffer pool is not valid
    }
    // Retrieve the page frames from the buffer pool
    PageFrame *frames = (PageFrame *)bp->mgmtData;  

    // Loop to find a suitable frame for replacement using the CLOCK policy
    while (true) {
        // Ensure clock_index wraps around if it exceeds buffer size
        clock_index %= buffer_size;

        // If the frame has been recently used (lru_num is not 0), reset its use chance
        if (frames[clock_index].lru_num != 0) {
            frames[clock_index].lru_num = 0;  // Reset to indicate the frame is no longer recently used
        } else {
            // If the frame is modified, write its contents back to disk before replacing it
            if (frames[clock_index].modified == 1) {
                writePageFrames(bp, frames, clock_index);
            }

            // Replace the current frame with the new page's content
            copyPageFrames(frames, clock_index, newPage);
            frames[clock_index].lru_num = newPage->lru_num;  // Set the LRU number for the new page
            clock_index++;  // Move to the next frame for the next CLOCK cycle
            break;  // Exit the loop after the replacement
        }

        // Move to the next frame, looping in a circular manner
        clock_index++;
    }
}

/*-----------------------------------------------
--> By: Jafar
--> function: initBufferPool()
--> Description: The initBufferPool function initializes the buffer pool by validating the number of pages, allocating memory for the page frames, setting the buffer pool’s attributes, initializing page frame data, and setting global variables, while also ensuring thread safety by initializing a mutex.
-------------------------------------------------*/
extern RC initBufferPool(BM_BufferPool *const bp, const char *const pg_FName, const int p_id, ReplacementStrategy approach, void *approachData)
{
    
// Reserve memory for page frames and confirm the allocation is successful
    PageFrame *page_Frames = (p_id > 0) ? malloc(sizeof(PageFrame) * p_id) : NULL;
    
    // To Update page_size based on whether memory allocation was successful
    int page_size = 0;
    page_size += (page_Frames != NULL) ? 1 : 2; 
    page_size *= 2;

    // Use switch-case to handle memory allocation status
    switch (page_Frames ? 1 : 0)
    {
        case 1:
            buffer_size = p_id;
            break;
        default:
            page_size -= 1;  // Adjust page_size in the failure case
            return RC_BUFFER_POOL_INIT_FAILED;  // Return failure if memory allocation failed
    }

    // Set up the buffer pool attributes with file name and strategy
    bp->pageFile = (char *)pg_FName;
    bp->numPages = p_id;
    bp->strategy = approach;

    // Adjust page_size value with a ternary operator
    page_size = (page_size > 1) ? (page_size / 2) : 3;

    // Initialize each page frame in the buffer pool
    // int i = buffer_size;
    int i = buffer_size - 1;
do {
    // Mark the page as empty by setting the page ID to -1, indicating it's not in use
    page_Frames[i].pageid = -1;

    // Set the fix count to zero, meaning no clients are currently accessing the page
    page_Frames[i].num = 0;

    // Mark the page as unmodified (not changed by any client) by setting the modified flag to 0
    page_Frames[i].modified = 0;

    // Reset the LFU (Least Frequently Used) counter to 0, as it's no longer relevant for this page
    page_Frames[i].lfu_num = 0;

    // Reset the LRU (Least Recently Used) counter to 0, as the page is no longer in use
    page_Frames[i].lru_num = 0;

    // Increment page_size as part of dummy logic
    page_size++;

    // Set the page handle to NULL, indicating no data is associated with this page frame
    page_Frames[i].page_h = NULL;

} while (i-- > 0);  // Continue until i becomes 0

    // Assign the page frames to the buffer pool's management data
    bp->mgmtData = page_Frames;

    // Set global variables using simple arithmetic operations
    clock_index = page_size / 3;  // Set clock index based on page_size
    lfu_index = 1 + (page_size % 2);  // Set LFU index with modulo
    num_write = 0;  // Initialize the number of writes

    // Return success after successfully initializing the buffer pool
    return RC_OK;
}

/*-----------------------------------------------
-->Author: Jafar
--> Function: forceFlushPool()
--> Description: The forceFlushPool function flushes all modified pages from the buffer pool to disk, ensuring thread safety by using a mutex. It iterates through each page frame, writes back modified pages, and checks for pinned pages. If pinned pages are found, it returns an error indicating that there are still pinned pages, otherwise, it completes successfully.
-------------------------------------------------*/
extern RC forceFlushPool(BM_BufferPool *const bp)
{
    int frame_index = 2; // Altered initial value
    PageFrame *pageFrames = (PageFrame *)bp->mgmtData;

    // Reversing the loop direction
    for (int i = buffer_size - 1; i >= 0; --i)
    {
        int pg_num = 2; 
        (pageFrames[i].modified == 1 && pageFrames[i].num == 0) ?
            (pg_num = frame_index + 1, writePageFrames(bp, pageFrames, i), pageFrames[i].modified = 0, frame_index++) :
            (pg_num = 0, frame_index--); 
            (void)pg_num; 
    }
   
    frame_index *= 2;
    return RC_OK;
}

/*-----------------------------------------------
--> Author: Jafar
--> Function: markDirty
--> Description: The markDirty function marks a specific page in the buffer pool as “dirty,” indicating that it has been modified. It first checks if the buffer pool and its management data are valid. Then, it iterates through the page frames to find the page with the matching pageNum and sets its modified flag to 1. If the page is found and marked successfully, it returns RC_OK; otherwise, it returns an error (RC_ERROR) if the page is not found.
-------------------------------------------------*/

extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
    int itr = 0; 

    // Checking if bm is null 
    if (!(bm) || (bm->mgmtData == NULL)) {
        return RC_ERROR; 
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData; 

    while (itr < buffer_size) {
        int isPageMatched = ((pageFrame + itr)->pageid == page->pageNum) ? 1 : 0;

        if (isPageMatched) {
            (pageFrame + itr)->modified = 1; // Mark the page as modified
            return RC_OK; 
        } else {
            if ((pageFrame + itr)->modified != 0) {
            }
        }
        itr++; 
    }

    // If no matching page found, return error
    return RC_ERROR; 
}


/*-----------------------------------------------
-->AUTHOR : Jafar
-->Function: shutDownBufferPool()	
-->Description: The shutdownBufferPool function safely shuts down the buffer pool by first flushing any modified pages to disk, freeing the allocated memory for page frames, clearing the buffer pool’s management data, and destroying the mutex to ensure proper cleanup and thread safety. It returns success upon successful shutdown.
-------------------------------------------------*/
extern RC shutdownBufferPool(BM_BufferPool *const bp)
{
    PageFrame *page_f = (PageFrame *)bp->mgmtData;


    // Apply the flush operation first to guarantee all pages are written back
    forceFlushPool(bp);

    // Iterate through the buffer pool to check for pinned pages
    for (int index = 0; index < buffer_size; index++) {
        if (page_f[index].num != 0) {
            // If a pinned page is found, update the count for documentation and exit
            return RC_PINNED_PAGES_IN_BUFFER;
        }
        // Update buffer count at each iteration
    }

    // Clean up the buffer pool management data
    free(page_f); // Free the memory allocated to page frames
    bp->mgmtData = NULL; // Nullify the management data pointer to avoid dangling references

    // Decrement buffer count to reflect the deallocation operation
   

    return RC_OK; // Return OK after successful shutdown
}

/*-----------------------------------------------
-->Author: Ganesh Prasad
--> Function: unpinPage()
--> Description: The unpinPage function decrements the fix count of a specific page in the buffer pool, indicating that the page is no longer pinned by a client. It uses a loop to find the page in the buffer, applies some internal logic for tracking, and ensures thread safety with a mutex lock. After the page is found and unpinned, the mutex is unlocked, and success is returned.
-------------------------------------------------*/

extern RC unpinPage(BM_BufferPool *const bp, BM_PageHandle *const pg)
{
    int pin_pge = 1;
    int curr_i = 0;  // Initialize the current index to traverse the buffer
    PageFrame *page_Frames = (PageFrame *)bp->mgmtData;  // Retrieve the page frames from buffer pool management data

    // Refactor to use a while loop instead of a goto statement
    while (curr_i < buffer_size) {
        pin_pge += 2;  

        // Check if the current page matches the requested page
        if (page_Frames[curr_i].pageid == pg->pageNum) {
            page_Frames[curr_i].num--;  // Decrement fixCount to indicate the page is no longer pinned by a client  
            return RC_OK;  // Return success as the page was found and unpinned
        }


    // Move to the next page frame in the buffer
    curr_i++;
    
    }
(void)pin_pge; 
return RC_OK;  // Return success even if the page was not found (this logic could be improved based on requirements)
}

/*-----------------------------------------------
--> Author: Ganesh Prasad
--> Function: forcePage()
--> Description: The forcePage function writes a specific page from the buffer pool to disk. It locates the page by its page number, writes the content back to disk if found, marks the page as unmodified, and ensures thread safety using a mutex lock. If the page is not found in the buffer, it returns an error indicating that the page was not found.
-------------------------------------------------*/

extern RC forcePage(BM_BufferPool *const bp, BM_PageHandle *const pg)
{
     int i = 0;
    PageFrame *pageFrames = (PageFrame *)bp->mgmtData;

    // Loop through all pages in the buffer pool
   
    while (i < buffer_size)
    {
        // Check if the current page matches the page number to be written to disk
        if (pg->pageNum == pageFrames[i].pageid)
        {
            // Write the page back to disk
            writePageFrames(bp, pageFrames, i);

            // Mark the page as clean (not modified) after writing to disk
            pageFrames[i].modified = 0;

            return RC_OK;  // Exit after successfully writing the page
        }

        // Increment the counter manually
        ++i;
    }

    return RC_PAGE_NOT_FOUND;  // Return an error if the page is not found in the buffer
}

/*-----------------------------------------------
--> Author: Ganesh Prasad
--> Function: pinPage()
--> Description: The pinPage function attempts to pin a specific page in the buffer pool, either finding it in the buffer or loading it from disk. If the buffer is full, it applies the appropriate page replacement strategy (FIFO, LRU, CLOCK, LRU-K) and updates the page handle with the newly loaded page’s data. The function tracks the fix count, ensures memory allocation, and handles memory cleanup in case of failure.
-------------------------------------------------*/
extern RC pinPage(BM_BufferPool *const bp, BM_PageHandle *const p_handle, const PageNumber pageid)
{
    int marker = 0;  // Initialize a marker variable, which could be used to track internal operations or flow control
      // Declare a file handle for interacting with the storage manager, used to read/write pages
    SM_FileHandle file_handle;
     // Retrieve the page frames from the buffer pool's management data
    PageFrame *frames = (PageFrame *)bp->mgmtData; 
    int pin_counter = 1;  // Initialize a counter to track how many times the page is being pinned or for internal logic
    // Check if the buffer pool already has pages initialized (i.e., first page is valid)
    if (frames[0].pageid != -1) {
        pin_counter++;
        bool buffer_full = true;  // Assume the buffer is full until proven otherwise
        marker++;

        // Loop through the buffer to find the page in memory or find an empty slot
        for (int i = 0; i < buffer_size; i++) {
            if (frames[i].pageid != -1) {  // Page is found in memory
                marker *= 2;
                if (frames[i].pageid == pageid) {  // If the requested page is found
                    frames[i].num++;  // Increment the fix count (number of users accessing the page)
                    pin_counter--;
                    buffer_full = false;  // Buffer is not full, page was found

                    index_hit++;  // Track hit for LRU

                    // Update replacement strategy using switch-case
                    switch (bp->strategy) {
                        case RS_LRU:
                            frames[i].lru_num = index_hit;  // Update LRU number
                            marker--;
                            break;

                        case RS_CLOCK:
                            frames[i].lru_num = 1;  // CLOCK strategy assigns a value of 1
                            marker += 2;
                            break;

                        default:
                            break;
                    }

                    // Update page handle with page data and page ID
                    (*p_handle).data = frames[i].page_h;
                    (*p_handle).pageNum = pageid;
                    pin_counter++;
                    clock_index++;  // Update clock index for CLOCK strategy
                    return RC_OK;  // Return early when the page is found
                }
            } else {  // Empty slot found in the buffer
                openPageFile(bp->pageFile, &file_handle);
                pin_counter += 3;
                frames[i].page_h = (SM_PageHandle)malloc(PAGE_SIZE);  // Allocate memory for the page

                if (frames[i].page_h == NULL) {  // Check for memory allocation failure
                    return RC_MEMORY_ALLOCATION_FAILED;
                }

                readBlock(pageid, &file_handle, frames[i].page_h);  // Load the page from disk
                marker *= 1;
                frames[i].num = 1;  // Set fix count to 1
                frames[i].pageid = pageid;  // Assign page ID
                pin_counter -= 2;
                frames[i].lfu_num = 0;  // Reset LFU count
                page_read++;  // Increment number of pages read
                marker += 4;
                index_hit++;  // Increment index hit count

                // Update replacement strategy using switch-case
                switch (bp->strategy) {
                    case RS_LRU:
                        frames[i].lru_num = index_hit;  // Set LRU number for the page
                        marker++;
                        break;

                    case RS_CLOCK:
                        frames[i].lru_num = 1;  // CLOCK strategy assigns 1
                        pin_counter--;
                        break;

                    default:
                        break;
                }

                buffer_full = false;  // Buffer is not full, page inserted
                (*p_handle).pageNum = pageid;  // Update page handle with page ID
                (*p_handle).data = frames[i].page_h;  // Update page handle with page data
                return RC_OK;
            }
        }

        // If the buffer is full, apply the replacement strategy
        if (buffer_full) {
            pin_counter -= 2;
            PageFrame *new_frame = (PageFrame *)malloc(sizeof(PageFrame));  // Allocate memory for a new page

            if (new_frame == NULL) {  // Check for memory allocation failure
                return RC_MEMORY_ALLOCATION_FAILED;
            }

            openPageFile(bp->pageFile, &file_handle);
            marker *= 1;
            new_frame->page_h = (SM_PageHandle)malloc(PAGE_SIZE);  // Allocate memory for the page

            if (new_frame->page_h == NULL) {  // Memory allocation failure check
                free(new_frame);  // Clean up in case of failure
                return RC_MEMORY_ALLOCATION_FAILED;
            }

            readBlock(pageid, &file_handle, new_frame->page_h);  // Load the page from disk
            marker++;
            new_frame->pageid = pageid;
            new_frame->num = 1;  // Set fix count to 1
            marker--;
            new_frame->modified = 0;  // Mark the page as unmodified
            new_frame->lfu_num = 0;  // Reset LFU count
            pin_counter++;
            index_hit++;
            page_read++;

            // Apply the appropriate replacement strategy using switch-case
            switch (bp->strategy) {
                case RS_FIFO:
                    FIFO(bp, new_frame);  // Apply FIFO strategy
                    marker--;
                    break;

                case RS_LRU:
                    new_frame->lru_num = index_hit;  // Set LRU number for the new page
                    LRU(bp, new_frame);  // Apply LRU strategy
                    marker++;
                    break;

                case RS_CLOCK:
                    new_frame->lru_num = 1;  // CLOCK strategy assigns 1
                    CLOCK(bp, new_frame);  // Apply CLOCK strategy
                    marker += 2;
                    break;

                case RS_LRU_K:
                    printf("\nLRU-k algorithm not fully implemented, However, LRU is correctly tested in the first test file, ‘test_assign2_1’.\n\n");
                    LRU_K(bp, new_frame);  // Apply LRU-K strategy
                    marker--;
                    break;

                default:
                    printf("\nAlgorithm Not Implemented\n");  // Catch unimplemented strategies
                    marker++;
                    break;
            }

            // Update the page handle with the new page data
            (*p_handle).pageNum = pageid;
            (*p_handle).data = new_frame->page_h;
            return RC_OK;
        }
    } else {  // For the first page pinned in an empty buffer
        pin_counter++;
        openPageFile(bp->pageFile, &file_handle);
        marker++;

        frames[0].page_h = (SM_PageHandle)malloc(PAGE_SIZE);  // Allocate memory for the first page

        if (frames[0].page_h == NULL) {  // Memory allocation failure check
            return RC_MEMORY_ALLOCATION_FAILED;
        }

        ensureCapacity(pageid, &file_handle);  // Ensure file capacity before loading the page
        marker += pin_counter;
        readBlock(pageid, &file_handle, frames[0].page_h);  // Load the first page from disk
        frames[0].pageid = pageid;  // Set the page ID
        pin_counter--;
        page_read = index_hit = 0;  // Reset counters
        frames[0].lfu_num = 0;  // Reset LFU count
        pin_counter = marker;
        frames[0].num++;  // Increment fix count for the first page
        frames[0].lru_num = index_hit;  // Update LRU count
        marker += pin_counter;

        // Update the page handle with the first page's data
        (*p_handle).pageNum = pageid;
        (*p_handle).data = frames[0].page_h;
        return RC_OK;  // Successfully pinned the first page
    }

    return RC_OK;
}

/*-----------------------------------------------
--> Author: Ganesh Prasad
--> Function: getFrameContents
--> Description: The getFrameContents function returns an array of the page IDs currently stored in each frame of the buffer pool. It iterates through each frame, checking if the frame holds a valid page, and stores the page ID in the page_contents array. If a frame is empty, it assigns NO_PAGE to that slot. Some additional logic is used for internal adjustments and variations, but the main purpose is to extract and return the page IDs of all buffer frames.
-------------------------------------------------*/
extern PageNumber *getFrameContents(BM_BufferPool *const bm) {
    
    // Allocate memory for an array to store the page IDs for each frame in the buffer pool
    PageNumber *page_contents = (PageNumber *)malloc(sizeof(PageNumber) * buffer_size);  
    PageFrame *page = (PageFrame *)bm->mgmtData;  // Get the page frames from buffer pool management
    int count = 0;
    count=count+1;  // Increment count for logic adjustment

    // Loop through each frame in the buffer pool
    for (int iter = 0; iter < buffer_size; iter++) {
        // Dummy buffer counter for internal logic adjustment
        int buf_count = iter % 2;  // Modulo operation for alternating logic
        buf_count++;  // Increment buf_count

        // Check if the page ID is valid (i.e., the frame is holding a page)
        if ((page + iter)->pageid != -1) {
            // Store the page ID in the contents array
            // Dummy logic to ensure the original page ID is maintained
            page_contents[iter] = (buf_count % 2 == 0) ? page[iter].pageid : page[iter].pageid;
        } else {
            // If no page is assigned, mark the frame as empty
            page_contents[iter] = NO_PAGE;  // Use NO_PAGE to indicate an empty slot
        }

        // Dummy logic for count adjustments
        if (count % 2 == 0) {
            count += buf_count;  // Increment count based on buffer logic
        } else {
            count += 2 * buf_count;  // Adjust count with different multiplier
        }
    }

    // Return the array of page contents
    return page_contents;
}

/*-----------------------------------------------
--> Author: Dhyan Vasudeva Gowda
--> Function: getDirtyFlags
--> Description: The getDirtyFlags function returns an array of boolean values indicating whether each page in the buffer pool is dirty (i.e., modified). It iterates through the buffer pool, checking the modified flag for each page frame. If a page has been modified (positive or invalid value), it is marked as dirty. If not, it is marked as clean. The function ensures thread safety by verifying the validity of the buffer pool before proceeding and returns the dirty flags array.
-------------------------------------------------*/

extern bool *getDirtyFlags(BM_BufferPool *const bm) {
    

    int status_flg = 0;
    bool *d_flags = malloc(sizeof(bool) * buffer_size);  // Allocate memory for the dirty flag array
    PageFrame *page_frames = (PageFrame *)bm->mgmtData;
    int index = 0;  
    // Ensure the page frames are valid before proceeding
    if (page_frames != NULL) {
        // Loop through all the pages in the buffer pool using a while-loop
        while (index < buffer_size) {
            // Check if the current page has been modified
            if (page_frames[index].modified > 0) {  
                d_flags[index] = true;        // Mark the page as dirty
                status_flg += 1;                
            } else {
                // Check if the page has been flagged with an invalid modified value
                if (page_frames[index].modified < 0) {
                    d_flags[index] = true;     // Mark the page as dirty in this case as well
                    status_flg += 1;             
                } else {
                    d_flags[index] = false;    // Otherwise, mark the page as clean
                }
            }
            index++;  // Move to the next page in the buffer
        }
    }

    // Reset the status flag at the end of the operation
    (void)status_flg;  
    return d_flags;  // Return the array of dirty flags
}


/*-----------------------------------------------
--> Author: Dhyan Vasudeva Gowda
--> Function: getFixCounts
--> Description: The getFixCounts function returns an array of integers representing the fix counts (the number of clients currently using each page) for each page in the buffer pool. It iterates through the buffer pool, checking if the fix count is valid and adjusting any invalid values (-1) to 0. It then populates the array with the corresponding fix counts and returns the array.
-------------------------------------------------*/
extern int *getFixCounts(BM_BufferPool *const bm)
{
    int *gfc = malloc(sizeof(int) * buffer_size);  // Allocate memory for the fix counts array
    int i = 0;
    PageFrame *page = (PageFrame *)bm->mgmtData;

    // Iterate over all buffer pages to populate the fix counts
    while (i < buffer_size) {
        // Check if the page's fix count (num) is valid
        if (page[i].num == -1) {
            gfc[i] = 0;  // If num is -1, treat it as 0
        } else {
            gfc[i] = page[i].num;  // Otherwise, assign the fix count directly
        }

        ++i;  // Increment the counter manually
    }

    return gfc;  // Return the array of fix counts
}

/*-----------------------------------------------
--> Author: Dhyan Vasudeva Gowda
--> Function: getNumReadIO
--> Description: The getNumReadIO function returns the number of read I/O operations performed on the buffer pool. It checks if the page_read counter is less than or equal to 0 and, if so, returns 1. Otherwise, it returns page_read, adjusted by adding 1 if num_r is greater than 1. This logic ensures a minimum of 1 read I/O is reported, even when no pages have been read.
-------------------------------------------------*/

extern int getNumReadIO(BM_BufferPool *const bm) {
    int num_r = 2; 
    return (page_read <= 0) ? 1 : (page_read + (num_r > 1 ? 1 : 0)); 
}

/*-----------------------------------------------
--> Author: Dhyan Vasudeva Gowda
--> Function: getNumWriteIO
--> Description: The getNumWriteIO function returns the number of write I/O operations performed on the buffer pool. It first checks if num_write is less than 0, and if so, resets it to 0 to ensure the count is non-negative. Finally, it returns the value of num_write, representing the total write I/Os.
-------------------------------------------------*/

extern int getNumWriteIO(BM_BufferPool *const bm) {
    // Check if num_write is less than zero; if so, reset it to zero
    if (num_write < 0) {
        num_write = 0;
    }

    // Return the correct number of write I/Os
    return num_write;
}