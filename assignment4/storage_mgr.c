#include "storage_mgr.h"
#include <stdlib.h>
#include <stdio.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>


// This Part Written By Jafar Alzoubi

FILE *pagePtr; // Global declaration

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: initStorageManager
DESCRIPTION: Initializes the storage manager. This function must be called before any other file operations. It sets up necessary resources for managing page files
-----*/

extern void initStorageManager(void) {
    pagePtr = NULL; // Initialize global variable
   // printf("This is The First Draft!");
}

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: createPageFile
DESCRIPTION: Creates a new page file with the specified name, initializing it with a single empty page. If a file with the same name exists, it will be overwritten. Returns `RC_OK` on success or an appropriate error code otherwise
-----*/

extern RC createPageFile(char *fileName) {
    //Checking on the file 
    // Attempt to open the file in write mode. If it can't be opened, return an error code.
    FILE *pagePtr = fopen(fileName, "w");
    if (!pagePtr) { // if the file not exist the answer will be 0 then !0 = True
        return RC_FILE_NOT_FOUND;  // File not found or could not be created.
    }

    // Allocate memory for a page and initialize it to zero.
    SM_PageHandle newPage = calloc(1, PAGE_SIZE);// we can use maloc here tho
    if (!newPage) { // same as !pagePtr
        fclose(pagePtr);  // Close the file before returning.
        return RC_MEMORY_ALLOCATION_FAIL; 
    }

    // Write the zero-initialized page to the file.
    if (fwrite(newPage, PAGE_SIZE, 1, pagePtr) != 1) {
        free(newPage);  
        fclose(pagePtr); // Close the file before returning.
        return RC_WRITE_FAILED;  // Writing to the file failed.
    }

    // Clean up resources: free the allocated memory and close the file.
    free(newPage);  // Free the sources to avoid memory leak
    fclose(pagePtr);  

    // Return success 
    return RC_OK;
}

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: openPageFile
DESCRIPTION: Opens an existing page file, reading its metadata (total pages, current page position) into the file handle (`SM_FileHandle`). Returns an error code if the file cannot be opened or found

-----*/

extern RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    // Open the file in read mode
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        return RC_FILE_NOT_FOUND; // Return error if file can't be opened
    }

    // Initialize file handle structure
    fHandle->fileName = fileName;   // Store file name
    fHandle->curPagePos = 0;        // Start at page 0
    fHandle->mgmtInfo = file;       // Save file pointer

    // Move to the end of the file to calculate size
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return RC_ERROR;            // Return error if seek fails
    }

    // Get file size and calculate number of pages
    long fileSize = ftell(file);
    if (fileSize < 0) {
        fclose(file);
        return RC_ERROR;            // Return error if file size can't be determined
    }
    fHandle->totalNumPages = (int)(fileSize / PAGE_SIZE); // Calculate total pages

    // Reset to the start of the file
    fseek(file, 0, SEEK_SET);

    return RC_OK;                   // File opened successfully
}

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: closePageFile
DESCRIPTION:  Closes the opened page file and frees any resources associated with it. Returns `RC_OK` on success or an error code if the file is not open.
-----*/

extern RC closePageFile(SM_FileHandle *fHandle) {
    // Ensure the file handle and file pointer are valid
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Close the file and check for errors
    if (fclose(fHandle->mgmtInfo) != 0) {
        return RC_ERROR;
    }

    // Nullify the file pointer to indicate the file is closed
    fHandle->mgmtInfo = NULL;

    return RC_OK;
}

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: destroyPageFile
DESCRIPTION:   Deletes the specified page file from disk. If the file cannot be found or deleted, an error code is returned.
-----*/

extern RC destroyPageFile(char *fileName) {
    // Attempt to open the file in read mode to check if it exists
    FILE *file = fopen(fileName, "r");
    
    if (file == NULL) {
        // File does not exist or cannot be opened, return error code
        return RC_FILE_NOT_FOUND;
    }
    
    // Close the file stream before attempting to delete it
    fclose(file);
    
    // Attempt to remove the file from the filesystem
    if (remove(fileName) != 0) {
        // If remove fails, return an error code
        return RC_ERROR;
    }
    
    // Return success 
    return RC_OK;
}

/*------
AUTHOR: Jafar Alzoubi
FUNCTION: readBlock
DESCRIPTION: Reads a specific page from the page file and stores its content in memory (`memPage`). Returns `RC_OK` if successful or an error if the page number is invalid
-----*/

// Function to read a block from the page file
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Validate the file handle and page number
    if (fHandle == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Open the file in read mode
    FILE *filePtr = fopen(fHandle->fileName, "rb");
    if (filePtr == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // Calculate the byte offset for the page
    long offset = (long)pageNum * PAGE_SIZE;

    // Move the file pointer to the correct position
    if (fseek(filePtr, offset, SEEK_SET) != 0) {
        fclose(filePtr);
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Read the page into memory
    size_t bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, filePtr);
    if (bytesRead < PAGE_SIZE) {
        fclose(filePtr);
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Close the file
    fclose(filePtr);

    // Update the current page position
    fHandle->curPagePos = pageNum;

    // Return success code
    return RC_OK;
}

/*------
AUTHOR: Ganesh Prasad Chandra Shekar
FUNCTION: getBlockPos
DESCRIPTION: Returns the current position (page number) in the opened file, indicating where the file pointer is located.
-----*/


// Function to retrieve the current block position in the file
RC getBlockPos(SM_FileHandle *fHandle) {
    // Check for a valid file handle
    if (fHandle == NULL || fHandle->fileName == NULL) {
        return -1; // Error: invalid file handle or file name
    }

    // Ensure the current page position is within a valid range
    if (fHandle->curPagePos < 0 || fHandle->curPagePos >= fHandle->totalNumPages) {
        return -1; // Error: current position is out of bounds
    }

    // Return the current page position if valid
    return fHandle->curPagePos;
}

/*------
AUTHOR: Ganesh Prasad Chandra Shekar
FUNCTION: readFirstBlock
DESCRIPTION: Reads the first page of the file into memory (`memPage`). Returns `RC_OK` if successful or an error if the file is not open.
-----*/

extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
        if (fHandle == NULL || memPage == NULL) {
        return RC_FILE_HANDLE_NOT_INIT; // Assuming RC_FILE_HANDLE_NOT_INIT is a defined error code
    }
    // Read the first block (page number 0)
    return readBlock(0, fHandle, memPage);
}

/*------
AUTHOR: Ganesh Prasad Chandra Shekar
FUNCTION: readPreviousBlock
DESCRIPTION: Reads the block before the current page position into memory. If the current page is the first one, an error is returned.
-----*/

// Function to read the previous block from the file
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Ensure the file handle is valid and that it's not already at the first block
    if (fHandle == NULL || fHandle->curPagePos <= 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Calculate the page number of the previous block
    int prevPageNum = fHandle->curPagePos - 1;

    // Open the file in binary read mode using the management information
    FILE *filePtr = (FILE *)fHandle->mgmtInfo;
    if (filePtr == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Move the file pointer to the start of the previous block
    if (fseek(filePtr, prevPageNum * PAGE_SIZE, SEEK_SET) != 0) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Read the previous block into memPage
    size_t bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, filePtr);
    if (bytesRead < PAGE_SIZE) {
        return RC_FILE_NOT_FOUND;
    }

    // Update the current page position to the previous block
    fHandle->curPagePos = prevPageNum;

    // Return success code
    return RC_OK;
}

/*------
AUTHOR: Ganesh Prasad Chandra Shekar
FUNCTION: readCurrentBlock
DESCRIPTION: Reads the page at the current file pointer position into memory. Returns `RC_OK` on success or an error if the file pointer is invalid.
-----*/


// Function to read the current block of the file
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Ensure the file handle is valid
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Check if the current page position is within a valid range
    if (fHandle->curPagePos < 0 || fHandle->curPagePos >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Move the file pointer to the current page position
    FILE *filePtr = (FILE *)fHandle->mgmtInfo;
    long offset = fHandle->curPagePos * PAGE_SIZE;
    if (fseek(filePtr, offset, SEEK_SET) != 0) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Attempt to read the current block into memPage
    size_t bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, filePtr);
    if (bytesRead < PAGE_SIZE) {
        if (feof(filePtr)) {
            return RC_FILE_NOT_FOUND;
        } else {
            return RC_FILE_NOT_FOUND;
        }
    }

    // Return success code if read is successful
    return RC_OK;
}


/*------
AUTHOR: Ganesh Prasad Chandra Shekar
FUNCTION: readNextBlock
DESCRIPTION:Reads the next page relative to the current page position into memory. If the current page is the last one, an error is returned.
-----*/

// Function to read the next block of the file
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Ensure the file handle is valid
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Calculate the next page position
    int nextPage = fHandle->curPagePos + 1;

    // Check if the next page is within a valid range
    if (nextPage >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Move the file pointer to the next page position
    FILE *filePtr = (FILE *)fHandle->mgmtInfo;
    long offset = nextPage * PAGE_SIZE;
    if (fseek(filePtr, offset, SEEK_SET) != 0) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Attempt to read the next block into memPage
    size_t bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, filePtr);
    if (bytesRead < PAGE_SIZE) {
        if (feof(filePtr)) {
            return RC_FILE_NOT_FOUND;
        } else {
            return RC_FILE_NOT_FOUND;
        }
    }

    // Update the current page position to the next page
    fHandle->curPagePos = nextPage;

    // Return success code if read is successful
    return RC_OK;
}

/*------
AUTHOR: Dhyan V Gowda
FUNCTION: readLastBlock
DESCRIPTION: Reads the last page of the file into memory. Returns `RC_OK` on success or an error code otherwise.
-----*/

// Function to read the last block of the file
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Ensure the file handle is valid and initialized
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Validate the total number of pages
    if (fHandle->totalNumPages <= 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Determine the last page number
    int lastPage = fHandle->totalNumPages - 1;

    // Attempt to read the last block using readBlock
    RC readStatus = readBlock(lastPage, fHandle, memPage);

    // Check if the read was successful
    if (readStatus != RC_OK) {
        return readStatus;
    }

    // Return success code
    return RC_OK;
}

/*------
AUTHOR: Dhyan V Gowda
FUNCTION: writeBlock
DESCRIPTION:  Writes the content of `memPage` to the specified page number in the file. Returns `RC_OK` if successful or an error code if the page number is invalid.
-----*/

// Function to write a block of data to a specific page in the file
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Validate the file handle and page number
    if (fHandle == NULL || pageNum < 0) {
        return RC_WRITE_FAILED;
    }

    // Attempt to open the file in binary read/write mode
    FILE *filePointer = fopen(fHandle->fileName, "r+b");
    if (filePointer == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // Calculate the offset where the block should be written
    long offset = (long)pageNum * PAGE_SIZE;

    // Move the file pointer to the calculated position
    if (fseek(filePointer, offset, SEEK_SET) != 0) {
        fclose(filePointer);
        return RC_WRITE_FAILED;
    }

    // Write the data block to the file
    size_t bytesWritten = fwrite(memPage, sizeof(char), PAGE_SIZE, filePointer);
    if (bytesWritten < PAGE_SIZE) {
        fclose(filePointer);
        return RC_WRITE_FAILED;
    }

    // Update the file handle's current page position
    fHandle->curPagePos = pageNum;

    // Close the file after writing
    fclose(filePointer);

    // Check if the page number was the last one; if so, update the total number of pages
    if (pageNum >= fHandle->totalNumPages) {
        fHandle->totalNumPages = pageNum + 1;
    }

    // Return success code
    return RC_OK;
}

/*------
AUTHOR: Dhyan V Gowda
FUNCTION: writeCurrentBlock
DESCRIPTION: Writes the content of `memPage` to the current page position in the file. Returns `RC_OK` on success or an error if the position is invalid.
-----*/


// Function to write data to the current block in a page file
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the file handle or memory page is NULL
    if (fHandle == NULL || memPage == NULL) {
        return RC_WRITE_FAILED; // Return an error if either is invalid
    }

    // Ensure that the current page position is within valid range
    if (fHandle->curPagePos < 0 || fHandle->curPagePos >= fHandle->totalNumPages) {
        return RC_WRITE_FAILED; // Return an error if curPagePos is out of bounds
    }

    // Attempt to write the current block using curPagePos directly
    RC status = writeBlock(fHandle->curPagePos, fHandle, memPage);

    // Check if the write operation was successful
    if (status == RC_OK) {
        // Optionally update curPagePos to point to the next block after writing
        fHandle->curPagePos += 1;
    }

    return status; // Return the status of the write operation
}

/*------
AUTHOR: Dhyan V Gowda
FUNCTION: appendEmptyBlock
DESCRIPTION: Adds a new, empty page to the end of the file. The new page is initialized to zero and increases the total number of pages in the file by one. Returns `RC_OK` on success.
-----*/

// Function to append an empty block to the file
RC appendEmptyBlock(SM_FileHandle *fHandle) {
    // Check if the file handle is valid
    if (fHandle == NULL) {
        return RC_FILE_HANDLE_NOT_INIT; // Return error if the file handle is not initialized
    }

    // Dynamically allocate memory for an empty block and initialize it to zero
    char *emptyBlock = (char *)calloc(PAGE_SIZE, sizeof(char));
    if (emptyBlock == NULL) {
        return RC_FILE_NOT_FOUND; // Return error if memory allocation fails
    }

    // Open the file in read/write mode to append the block
    FILE *file = fopen(fHandle->fileName, "r+b");
    if (file == NULL) {
        free(emptyBlock); // Free allocated memory if file opening fails
        return RC_FILE_NOT_FOUND; // Return error if the file cannot be opened
    }

    // Move the file pointer to the end of the file to append the new block
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        free(emptyBlock); // Free allocated memory if seeking fails
        return RC_WRITE_FAILED; // Return error if seeking to the end fails
    }

    // Write the empty block to the file
    size_t writtenSize = fwrite(emptyBlock, sizeof(char), PAGE_SIZE, file);
    if (writtenSize != PAGE_SIZE) {
        fclose(file);
        free(emptyBlock); // Free allocated memory if writing fails
        return RC_WRITE_FAILED; // Return error if writing fails
    }

    // Update the file handle's metadata
    fHandle->totalNumPages++;
    fHandle->curPagePos = fHandle->totalNumPages - 1;

    // Clean up: close the file and free allocated memory
    fclose(file);
    free(emptyBlock);

    return RC_OK;
}

/*------
AUTHOR: Dhyan V Gowda
FUNCTION: ensureCapacity
DESCRIPTION: Ensures that the file has at least `numberOfPages`. If the file has fewer pages, it appends empty pages until the specified number is reached. Returns `RC_OK` on success.
-----*/


RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    // Validate the file handle first
    if (fHandle == NULL) {
        return RC_FILE_HANDLE_NOT_INIT; // Return error if the file handle is not initialized
    }

    // Calculate the difference between required pages and current pages
    int pagesToAdd = numberOfPages - fHandle->totalNumPages;

    // If more pages are needed, append empty blocks
    if (pagesToAdd > 0) {
        for (int i = 0; i < pagesToAdd; i++) {
            RC status = appendEmptyBlock(fHandle); // Append an empty block

            // Check if appending the block was successful
            if (status != RC_OK) {
                return status; // Return the error if appending failed
            }
        }
    }

    // Return success code if capacity is ensured or no additional pages were needed
    return RC_OK;
}