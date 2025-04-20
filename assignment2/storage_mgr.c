#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage_mgr.h"
#include "dberror.h"

#define nullptr NULL

FILE *filePointer;
RC ret_value;

int block_num = 0;
int total_offset = 1;

/*-----------------------------------------------
 --> Author: Jafar Alzoubi
 --> Function Name: setEmptyMemory
 --> Description: This function will set the memory to NULL.
 --> Parameters used is Memory
-----------------------------------------------*/

void setEmptyMemory(char *memory) {
    for (size_t i = 0; i < PAGE_SIZE; i++) {
        memory[i] = '\0';
    }
    block_num += PAGE_SIZE; // Increment block_num by PAGE_SIZE instead of in a loop
}

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: initStorageManager
DESCRIPTION: Initializes the storage manager. This function must be called before any other file operations. It sets up necessary resources for managing page files
-----*/

void initStorageManager(void) {
    filePointer = nullptr;
    block_num = 2; // Start directly at 2 since we know we increment it once initially
    ret_value = -1;

    printf("\nDefining the Storage Manager function");
    printf("\nStorage Manager Defined and successfully initialized");
}

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: createPageFile
DESCRIPTION: Creates a new page file with the specified name, initializing it with a single empty page. If a file with the same name exists, it will be overwritten. Returns `RC_OK` on success or an appropriate error code otherwise
-----*/

extern RC createPageFile(char *fileName) {
    filePointer = fopen(fileName, "w+");

    if (filePointer != nullptr) {
        SM_PageHandle new_page = (SM_PageHandle)malloc(PAGE_SIZE);
        if (new_page == nullptr) {
            fclose(filePointer);
            return RC_MEMORY_ALLOCATION_FAIL;
        }

        setEmptyMemory(new_page);
        if (fwrite(new_page, sizeof(char), PAGE_SIZE, filePointer) < PAGE_SIZE) {
            printf("\nZero page cannot be appended to file");
            free(new_page);
            fclose(filePointer);
            return RC_WRITE_FAILED;
        }

        printf("\nZero page has been appended to file pointed by filePointer");
        free(new_page);
        fclose(filePointer);
        return RC_OK;
    } else {
        return RC_FILE_NOT_FOUND;
    }
}

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: openPageFile
DESCRIPTION: Opens an existing page file, reading its metadata (total pages, current page position) into the file handle (`SM_FileHandle`). Returns an error code if the file cannot be opened or found

-----*/

extern RC openPageFile(char* fileName, SM_FileHandle *fHandle) {
    filePointer = fopen(fileName, "r+");
    if (!filePointer) {
        return RC_FILE_NOT_FOUND;
    }
    
    fHandle->fileName = fileName;
    fHandle->curPagePos = 0;
    fseek(filePointer, 0, SEEK_END);
    fHandle->totalNumPages = ftell(filePointer) / PAGE_SIZE;
    fclose(filePointer);
    return RC_OK;
}

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: closePageFile
DESCRIPTION:  Closes the opened page file and frees any resources associated with it. Returns `RC_OK` on success or an error code if the file is not open.
-----*/

extern RC closePageFile(SM_FileHandle *fHandle) {
    if (!fHandle) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    printf("Inside closePageFile\n"); 
    filePointer = fopen(fHandle->fileName, "r");
    if (filePointer) {
        fclose(filePointer);
        return RC_OK;
    } else {
        return RC_FILE_NOT_FOUND;
    }
}


/*------
AUTHOR: Jafar Alzoubi
FUNCTION: destroyPageFile
DESCRIPTION:   Deletes the specified page file from disk. If the file cannot be found or deleted, an error code is returned.
-----*/

extern RC destroyPageFile(char *fileName) {
    filePointer = fopen(fileName, "r");
    if (!filePointer) {
        return RC_FILE_NOT_FOUND;
    }
    
    fclose(filePointer);
    if (remove(fileName) == 0) {
        return RC_OK;
    } else {
        return RC_FILE_NOT_FOUND;
    }
}

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: readBlock
DESCRIPTION: Reads a specific page from the page file and stores its content in memory (`memPage`). Returns `RC_OK` if successful or an error if the page number is invalid
-----*/

RC readBlock(int pgNum, SM_FileHandle *fileHandle, SM_PageHandle memPage) {
    if (pgNum < 0 || pgNum >= fileHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    filePointer = fopen(fileHandle->fileName, "r");
    if (!filePointer) {
        return RC_FILE_NOT_FOUND;
    }

    fseek(filePointer, pgNum * PAGE_SIZE, SEEK_SET);
    size_t bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, filePointer);
    fclose(filePointer);

    if (bytesRead < PAGE_SIZE) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    fileHandle->curPagePos = pgNum;
    return RC_OK;
}

/*------
AUTHOR: Ganesh Prasad Chandra Shekar
FUNCTION: getBlockPos
DESCRIPTION: Returns the current position (page number) in the opened file, indicating where the file pointer is located.
-----*/

extern RC getBlockPos(SM_FileHandle *fHandle) {
    if (!fHandle) {
        return RC_FILE_HANDLE_NOT_INIT;  // More specific error for uninitialized file handle
    }
    return fHandle->curPagePos;  // Directly return the current position
}

/*------
AUTHOR: Ganesh Prasad Chandra Shekar
FUNCTION: readFirstBlock
DESCRIPTION: Reads the first page of the file into memory (`memPage`). Returns `RC_OK` if successful or an error if the file is not open.
-----*/

extern RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (!fHandle || !memPage) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    FILE *filePointer = fopen(fHandle->fileName, "r");
    if (!filePointer) {
        return RC_FILE_NOT_FOUND;
    }

    if (fseek(filePointer, 0, SEEK_SET) != 0) {
        fclose(filePointer);
        return RC_READ_NON_EXISTING_PAGE;  // More specific error when fseek fails
    }

    size_t bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, filePointer);
    fclose(filePointer);

    if (bytesRead < PAGE_SIZE) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    fHandle->curPagePos = 0;  // Set current page position to first
    return RC_OK;
}

/*------
AUTHOR: Ganesh Prasad Chandra Shekar
FUNCTION: readPreviousBlock
DESCRIPTION: Reads the block before the current page position into memory. If the current page is the first one, an error is returned.
-----*/

extern RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (!fHandle || !memPage) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    if (fHandle->curPagePos == 0) {
        return RC_READ_NON_EXISTING_PAGE;  // Cannot read a block before the first
    }

    return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

/*------
AUTHOR: Ganesh Prasad Chandra Shekar
FUNCTION: readCurrentBlock
DESCRIPTION: Reads the page at the current file pointer position into memory. Returns `RC_OK` on success or an error if the file pointer is invalid.
-----*/

extern RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (!fHandle || !memPage) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

/*------
AUTHOR: Ganesh Prasad Chandra Shekar
FUNCTION: readNextBlock
DESCRIPTION: Reads the next page relative to the current page position into memory. If the current page is the last one, an error is returned.
-----*/

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (!fHandle || !memPage) {
        return RC_FILE_HANDLE_NOT_INIT; // Check for null pointers to avoid dereferencing null.
    }
    
    if (fHandle->curPagePos >= fHandle->totalNumPages - 1) {
        return RC_READ_NON_EXISTING_PAGE; // Check if the current page is the last one.
    }

    return readBlock(fHandle->curPagePos + 1, fHandle, memPage); // Read the next block.
}


/*------
AUTHOR: Dhyan V Gowda
FUNCTION: readLastBlock
DESCRIPTION: Reads the last page of the file into memory. Returns `RC_OK` on success or an error code otherwise.
-----*/

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Validate the input pointers first to ensure they are not null
    if (!fHandle || !memPage) {
        return RC_FILE_HANDLE_NOT_INIT; // Return initialization error if file handle or memory page is null
    }

    // Ensure there is at least one page to read
    if (fHandle->totalNumPages <= 0) {
        return RC_READ_NON_EXISTING_PAGE; // Return error if there are no pages to read
    }

    // Calculate the page number of the last page
    int lastPageNum = fHandle->totalNumPages - 1;

    // Use the existing function to read the last page
    return readBlock(lastPageNum, fHandle, memPage);
}

/*------
AUTHOR: Dhyan V Gowda
FUNCTION: writeBlock
DESCRIPTION:  Writes the content of `memPage` to the specified page number in the file. Returns `RC_OK` if successful or an error code if the page number is invalid.
-----*/
extern RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle mrPg) {
    // Validate the page number and file handle before proceeding
    if (pageNum < 0 || pageNum > fHandle->totalNumPages || fHandle == NULL) {
        return RC_WRITE_FAILED;
    }

    int writeOffset = PAGE_SIZE * pageNum; // Calculate the offset for the file write operation
    FILE *filePointer = fopen(fHandle->fileName, "r+"); // Open the file in read/update mode

    if (!filePointer) { // Check if file opening failed
        return RC_FILE_NOT_FOUND;
    }

    if (pageNum != 0) {
        appendEmptyBlock(fHandle); // Append empty block if not the first page (conditional use questionable, consider logic)
    }
    
    // Move to the correct position in the file and write the page data
    fseek(filePointer, writeOffset, SEEK_SET);
    if (fwrite(mrPg, sizeof(char), PAGE_SIZE, filePointer) < PAGE_SIZE) { // Write and check success in one line
        fclose(filePointer);
        return RC_WRITE_FAILED; // Return failure if not all data was written
    }

    fHandle->curPagePos = ftell(filePointer); // Update the current page position in the file handle
    fclose(filePointer); // Close the file after writing
    return RC_OK; // Return success after successful write operation
}

/*------
AUTHOR: Dhyan V Gowda
FUNCTION: writeCurrentBlock
DESCRIPTION: Writes the content of `memPage` to the current page position in the file. Returns `RC_OK` on success or an error if the position is invalid.
-----*/

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if file handle or memory page pointer is null
    if (!fHandle || !memPage) {
        return RC_FILE_HANDLE_NOT_INIT; // Return error if any pointer is NULL
    }

    // Check if the current page position is valid
    if (fHandle->curPagePos < 0 || fHandle->curPagePos >= fHandle->totalNumPages) {
        return RC_WRITE_FAILED; // Return error if current page position is out of valid range
    }

    // Delegate to writeBlock function using the current page position
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

/*------
AUTHOR: Dhyan V Gowda
FUNCTION: appendEmptyBlock
DESCRIPTION: Adds a new, empty page to the end of the file. The new page is initialized to zero and increases the total number of pages in the file by one. Returns `RC_OK` on success.
-----*/

extern RC appendEmptyBlock(SM_FileHandle *fHandle) {
    if (fHandle == NULL) {
        return RC_FILE_HANDLE_NOT_INIT; // Properly handle null file handle
    }

    // Allocate and initialize an empty block of memory
    SM_PageHandle emptyBlock = (SM_PageHandle) calloc(PAGE_SIZE, sizeof(char));
    if (emptyBlock == NULL) {
        return RC_MEMORY_ALLOCATION_FAIL; // Handle memory allocation failure
    }

    // Open the file in append mode to ensure data is added to the end
    FILE *filePointer = fopen(fHandle->fileName, "ab");
    if (filePointer == NULL) {
        free(emptyBlock); // Ensure memory is freed on file opening failure
        return RC_FILE_NOT_FOUND; // Return file not found error
    }

    // Write the empty page to the file
    size_t writeResult = fwrite(emptyBlock, sizeof(char), PAGE_SIZE, filePointer);
    fclose(filePointer); // Close the file immediately after writing
    free(emptyBlock); // Free the allocated memory block

    if (writeResult == PAGE_SIZE) {
        fHandle->totalNumPages++; // Only increment total pages if write was successful
        return RC_OK;
    } else {
        return RC_WRITE_FAILED; // Return write failed error if not all data was written
    }
}

/*------
AUTHOR: Dhyan V Gowda
FUNCTION: ensureCapacity
DESCRIPTION: Ensures that the file has at least `numberOfPages`. If the file has fewer pages, it appends empty pages until the specified number is reached. Returns `RC_OK` on success.
-----*/

extern RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    if (fHandle == NULL) {
        return RC_FILE_HANDLE_NOT_INIT; // Return error if file handle is not initialized
    }

    int currentPageCount = fHandle->totalNumPages; // Get the current number of pages in the file
    if (currentPageCount >= numberOfPages) {
        return RC_OK; // No need to append if the current count meets or exceeds the required capacity
    }

    int pagesToAdd = numberOfPages - currentPageCount; // Calculate the number of pages to add
    for (int i = 0; i < pagesToAdd; i++) {
        RC result = appendEmptyBlock(fHandle);
        if (result != RC_OK) {
            return result; // Return error immediately if appending a page fails
        }
    }

    return RC_OK; // Return success after successfully appending all required pages
}