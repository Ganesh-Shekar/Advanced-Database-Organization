## Group#26, CS525 ADO Assignment - 1 - Storage Manager

Team Members and Contribution (Names are commented on the source code) :

1. NAME: Ganesh Prasad Chandra Shekar
   EMAIL: gchandrashekar@hawk.iit.edu
   CWID: A20557831
   CONTRIBUTION: 33.33 %

2. NAME: Dhyan Vasudeva Gowda
   email: dgowda2@hawk.iit.edu
   CWID: A20592874
   CONTRIBUTION: 33.33 %

3. NAME: Jafar Alzoubi
   EMAIL: jalzoubi@hawk.iit.edu
   CWID: A20501723
   CONTRIBUTION: 33.33 %

### Storage Manager:

This project implements a storage manager with functionalities to create, manipulate, read, and write blocks from a page file on disk. It provides a basic foundation for handling file operations in a database system, enabling file management at a low level.

### Folder structure: 

/Project_Root
├── storage_mgr.c # Source file implementing the storage manager functions
├── storage_mgr.h # Header file containing function prototypes and data structures
├── dberror.h # Error handling for the project
├── test_helper.h # Defines error codes and macros used in the test cases
├── test_assign1_1.c # Test file which contains test cases for storage manager functions
├── Readme.md # Briefly describes our programming flow
└── Makefile # File to compile and link the project

### Functions and Descriptions:

### File Manipulation Functions

1. **`initStorageManager()`**
   AUTHOR: JAFAR

   Initializes the storage manager. This function must be called before any other file operations. It sets up necessary resources for managing page files.

2. **`createPageFile(char *fileName)`**  
   AUTHOR: JAFAR

   Creates a new page file with the specified name, initializing it with a single empty page. If a file with the same name exists, it will be overwritten. Returns `RC_OK` on success or an appropriate error code otherwise.

3. **`openPageFile(char *fileName, SM_FileHandle *fHandle)`**  
   AUTHOR: JAFAR

   Opens an existing page file, reading its metadata (total pages, current page position) into the file handle (`SM_FileHandle`). Returns an error code if the file cannot be opened or found.

4. **`closePageFile(SM_FileHandle *fHandle)`**  
   AUTHOR: JAFAR

   Closes the opened page file and frees any resources associated with it. Returns `RC_OK` on success or an error code if the file is not open.

5. **`destroyPageFile(char *fileName)`**  
   AUTHOR: JAFAR

   Deletes the specified page file from disk. If the file cannot be found or deleted, an error code is returned.

### Block Reading Functions

6. **`readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)`**
   AUTHOR: JAFAR

   Reads a specific page from the page file and stores its content in memory (`memPage`). Returns `RC_OK` if successful or an error if the page number is invalid.

7. **`getBlockPos(SM_FileHandle *fHandle)`**  
   AUTHOR: Ganesh Prasad

   Returns the current position (page number) in the opened file, indicating where the file pointer is located.

8. **`readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)`**  
   AUTHOR: Ganesh Prasad

   Reads the first page of the file into memory (`memPage`). Returns `RC_OK` if successful or an error if the file is not open.

9. **`readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)`**
   AUTHOR: Ganesh Prasad

   Reads the block before the current page position into memory. If the current page is the first one, an error is returned.

10. **`readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)`**  
    AUTHOR: Ganesh Prasad

Reads the page at the current file pointer position into memory. Returns `RC_OK` on success or an error if the file pointer is invalid.

11. **`readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)`**
    AUTHOR: Ganesh Prasad

Reads the next page relative to the current page position into memory. If the current page is the last one, an error is returned.

12. **`readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)`**
    AUTHOR: Dhyan V Gowda

    Reads the last page of the file into memory. Returns `RC_OK` on success or an error code otherwise.

### Block Writing Functions

13. **`writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)`**  
    AUTHOR: Dhyan V Gowda

    Writes the content of `memPage` to the specified page number in the file. Returns `RC_OK` if successful or an error code if the page number is invalid.

14. **`writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)`**
    AUTHOR: Dhyan V Gowda

    Writes the content of `memPage` to the current page position in the file. Returns `RC_OK` on success or an error if the position is invalid.

15. **`appendEmptyBlock(SM_FileHandle *fHandle)`**  
    AUTHOR: Dhyan V Gowda

    Adds a new, empty page to the end of the file. The new page is initialized to zero and increases the total number of pages in the file by one. Returns `RC_OK` on success.

16. **`ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)`**  
    AUTHOR: Dhyan V Gowda

    Ensures that the file has at least `numberOfPages`. If the file has fewer pages, it appends empty pages until the specified number is reached. Returns `RC_OK` on success.

### Steps For Execution:

1. Go to Project root through Terminal.
2. Type ls to list the files and check that you are in the correct directory.
3. Type "make clean" to delete old compiled files.
4. Type "make" to compile all project files including "test_assign1_1.c" file.
5. Type "./test_assign1" to run "test_assign1_1.c" file.
   In order to check for data leaks type the below command:
6. "valgrind --leak-check=full --track-origins=yes ./test_assign1"
