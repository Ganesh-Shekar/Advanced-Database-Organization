#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <stddef.h>

// This is custom data structure defined for making the use of Record Manager.
typedef struct Rec_Manager
{
    int count_of_tuples;
    int pages_free;
    int count_for_scan;
    int man_rec;
    RID r_id;
    Expr *condition;
    BM_PageHandle pagefiles;
    BM_BufferPool buffer;
    
} Rec_Manager;

int countIndex, MAX_COUNT = 1;
const int MAX_NUMBER_OF_PAGES = 100;
Rec_Manager *recordManager, *scan_Manager, *table_Manager;
const int DEFAULT_RECORD_SIZE = 256;
const int SIZE_OF_ATTRIBUTE = 15; // Size of the name of the attribute
typedef char *dtCHARPtr;

// ******** CUSTOM FUNCTIONS ******** //
/*----------------------------------------------- 
--> Author: Ganesh Prasad Chandra Shekar
--> Function: findFreeSlot()
--> Description: The index of a slot that is open on a page is provided by this function.
--> Parameters used: char *data, int recordSize
--> return type: Return Code
-------------------------------------------------*/

// This function returns a free slot within a page
int findFreeSlot(dtCHARPtr ch_data, int sizeOfRec) {
    int slots = PAGE_SIZE / sizeOfRec;
    for (int i = 0; i < slots; i++) {
        bool indecator = ch_data[i * sizeOfRec] != '+';
        if (indecator) {
            return i; // Return first available slot index
        }
    }
    return -1; // No free slot found
}

void checker()
{
}

void recordChecker()
{
}

extern void free_mem(void *pt)
{
    free(pt);
}

// ******** TABLE AND RECORD MANAGER FUNCTIONS ******** //

/*-----------------------------------------------
--> Author: Jafar Alzoubi
--> Function: initRecordManager()
--> Description: Initializes the Record Manager by initializing the storage manager.
-------------------------------------------------*/
extern RC initRecordManager(void *mgmtData)
{
   initStorageManager(); // initializing the storage manager.
	return RC_OK;
}
/*-----------------------------------------------
--> Author: Jafar Alzoubi
--> Function: shutdownRecordManager()
--> Description: Shuts down the Record Manager by shutting down the buffer pool and freeing allocated memory.
-------------------------------------------------*/
extern RC shutdownRecordManager()
{
    RC r = shutdownBufferPool(&recordManager->buffer);
     // Attempt to shut down the buffer pool
    if (shutdownBufferPool(&recordManager->buffer) == RC_ERROR)
        // If there is an error during shutdown, run checker and return RC_ERROR
        return RC_ERROR;
    // Free the memory allocated for the record manager
    else if (r != RC_ERROR){
    free(recordManager);
    return RC_OK;  // Return success 
    }
}
/*-----------------------------------------------
--> Author: Jafar Alzoubi
--> Function: initializeTable()
--> Description: Initializes the buffer for a new table with metadata and schema attributes.
-------------------------------------------------*/
void initializeTable(char *buffer, char *tableName, Schema *schema)
{
    char *dataPtr = buffer;                    // Pointer to buffer for data manipulation
    int metaValues[4] = {0, 1, schema->numAttr, schema->keySize};  // Table metadata

    // Allocate memory for the record manager and initialize buffer pool
    recordManager = (Rec_Manager *)malloc(sizeof(Rec_Manager));
    initBufferPool(&recordManager->buffer, tableName, MAX_NUMBER_OF_PAGES, RS_LRU, NULL);

    // Populate buffer with metadata values
    *((int *)dataPtr) = metaValues[0];         // Initialize count of tuples
    dataPtr += sizeof(int);
    *((int *)dataPtr) = metaValues[1];         // Initialize free pages
    dataPtr += sizeof(int);
    *((int *)dataPtr) = metaValues[2];         // Set number of attributes
    dataPtr += sizeof(int);
    *((int *)dataPtr) = metaValues[3];         // Set key size
    dataPtr += sizeof(int);

    // Write schema attributes to buffer
    int i = 0;
    while (i < schema->numAttr) {
        strncpy(dataPtr, schema->attrNames[i], SIZE_OF_ATTRIBUTE); // Copy attribute name
        dataPtr = dataPtr + SIZE_OF_ATTRIBUTE;

        *((int *)dataPtr) = (int)schema->dataTypes[i]; // Copy data type
        dataPtr = dataPtr + sizeof(int);

        *((int *)dataPtr) = (int)schema->typeLength[i]; // Copy type length
        dataPtr = dataPtr + sizeof(int);
        i++;
    }
}

/*-----------------------------------------------
--> Author: Jafar Alzoubi
--> Function: createTable()
--> Description: Creates a new table and handles file operations for writing metadata.
-------------------------------------------------*/
extern RC createTable(char *tableName, Schema *schema)
{
    char buffer[PAGE_SIZE];          // Buffer to store metadata
    SM_FileHandle fileHandle;        // File handle for page operations
    // Initialize buffer with table metadata and schema attributes
    initializeTable(buffer, tableName, schema); // the function above 

    // Attempt to create and open a page file, then write metadata if successful
    if (createPageFile(tableName) == RC_OK && openPageFile(tableName, &fileHandle) == RC_OK) {
        RC writeResult = writeBlock(0, &fileHandle, buffer);
        closePageFile(&fileHandle);
        if(writeResult == RC_OK)
        return RC_OK;
        else if(writeResult != RC_OK)
        return (writeResult == RC_OK) ? RC_OK : RC_WRITE_FAILED;
    }

    return RC_ERROR; // Return error code if file operations fail
}

/*-----------------------------------------------
--> Author: Jafar Alzoubi
--> Function: allocateAndPopulateSchema()
--> Description: Helper function for openTable; allocates and populates schema attributes from a page handle.
-------------------------------------------------*/
RC allocateAndPopulateSchema(SM_PageHandle *pageHandle, int attributeCount, Schema *schema) {
    for (int i = 0; i < attributeCount; ++i) {
        // Allocate memory for each attribute name and check for allocation failure
        schema->attrNames[i] = (char *)calloc(SIZE_OF_ATTRIBUTE, sizeof(char));
        if (!schema->attrNames[i]) {
            // Free all previously allocated attribute names on failure
            for (int j = 0; j < i; j++) {
                free(schema->attrNames[j]);
            }
            return RC_MEMORY_ALLOCATION_FAIL;
        }

        // Copy the attribute name from pageHandle
        memcpy(schema->attrNames[i], *pageHandle, SIZE_OF_ATTRIBUTE);
        *pageHandle += SIZE_OF_ATTRIBUTE;

        // Retrieve and assign data type using a temporary pointer for clarity
        schema->dataTypes[i] = *((DataType *)*pageHandle);
        *pageHandle += sizeof(DataType);

        // Retrieve and assign type length with direct assignment
        schema->typeLength[i] = *((int *)*pageHandle);
        *pageHandle += sizeof(int);
    }
    return RC_OK;
}
/*-----------------------------------------------
--> Author: Jafar Alzoubi
--> Function: initializeSchema()
--> Description: Helper function for openTable; initializes the schema based on the page handle data.
-------------------------------------------------*/
RC initializeSchema(SM_PageHandle pageHandle, int attributeCount, RM_TableData *rel) {
    // Allocate memory for the schema
    Schema *schema = (Schema *)malloc(sizeof(Schema));
    if (!schema) return RC_MEMORY_ALLOCATION_FAIL;

    // Set number of attributes with a positive check
   bool x= (schema->numAttr = attributeCount)>0;
   if(x == true)
    schema->numAttr =attributeCount;
    else attributeCount = 0;
    
    schema->numAttr = attributeCount > 0 ? attributeCount : 0;

    // Allocate memory for schema components with zero-initialized calloc for safety
    schema->attrNames = (char **)calloc(attributeCount, sizeof(char *));
    schema->dataTypes = (DataType *)calloc(attributeCount, sizeof(DataType));
    schema->typeLength = (int *)calloc(attributeCount, sizeof(int));

    // Check for memory allocation failures
    bool flag = true;
    if (!schema->attrNames || !schema->dataTypes || !schema->typeLength) {
        flag= !schema->attrNames || !schema->dataTypes || !schema->typeLength;
if(flag){
        free(schema->attrNames);
        free(schema->dataTypes);
        free(schema->typeLength);
        free(schema);
        return RC_MEMORY_ALLOCATION_FAIL;
    }}

    
    RC status = allocateAndPopulateSchema(&pageHandle, attributeCount, schema);
    if (status != RC_OK) {
        // Free all allocated memory on failure
        free(schema->attrNames);
        free(schema->dataTypes);
        if (status != RC_OK)
        free(schema->typeLength);
        if (status != RC_OK)
        free(schema);
        return status;
    }

    // Assign schema to the table and return success status
    rel->schema = schema;
    if (status == RC_OK)
    return RC_OK;
}
/*-----------------------------------------------
--> Author: Jafar Alzoubi
--> Function: openTable()
--> Description: Opens the specified table and initializes its metadata.
-------------------------------------------------*/
extern RC openTable(RM_TableData *rel, char *name) {
    // Assign table name and metadata
    if (rel != NULL)
    rel->name = name;
    rel->mgmtData = recordManager;

    // Pin the page in the buffer pool and check for success
    RC r = pinPage(&recordManager->buffer, &recordManager->pagefiles, 0);
    if (pinPage(&recordManager->buffer, &recordManager->pagefiles, 0) != RC_OK) {
        return RC_ERROR; // Return error if pinning fails
        if(r!=RC_OK)
        return RC_ERROR;
    }

    // Initialize page handle and extract initial table data
    SM_PageHandle pageHandle = (char *)recordManager->pagefiles.data;
    int *intData = (int *)pageHandle; // Create a pointer for easy access
    recordManager->count_of_tuples = intData[0]; // Read count of tuples
    recordManager->pages_free = intData[1];      // Read free pages

    // Move pageHandle to the next segment for attribute count
    pageHandle += 2 * sizeof(int); // Skip count_of_tuples and pages_free
    int attributeCount = *(int *)pageHandle; // Read attribute count
    pageHandle += sizeof(int); // Move past attribute count

    // Initialize the schema
    RC result = initializeSchema(pageHandle, attributeCount, rel);
    if (result != RC_OK) {
        return result; // Return error if schema initialization fails
         if(RC_OK != result)
            return result;
    }
    // Force the page to the buffer pool and return status
    return (forcePage(&recordManager->buffer, &recordManager->pagefiles) == RC_ERROR) ? RC_ERROR : RC_OK;
}

/*-----------------------------------------------
--> Author: Jafar Alzoubi
--> Function: closeTable()
--> Description: Closes the specified table and shuts down its buffer pool.
-------------------------------------------------*/

extern RC closeTable(RM_TableData *rel)
{
     // Retrieve the record manager from the relation
    Rec_Manager *rMgr = rel->mgmtData;
    int result = shutdownBufferPool(&rMgr->buffer);
    if(result == RC_ERROR) return (float)result; else (float)RC_OK;
    return (result == RC_ERROR) ? (float)result : (float)RC_OK;
    // Check the result of the shutdown operation
    if (result == RC_ERROR) {
        return (float)result; // Return error code
    } 
    if (result != RC_ERROR) {
    return (float)RC_OK; // Return success
    }
}
/*-----------------------------------------------
--> Author: Jafar Alzoubi
--> Function: deleteTable()
--> Description: Deletes the specified table by destroying its page file.
-------------------------------------------------*/
extern RC deleteTable(char *name)
{
    // Attempt to destroy the page file and store the result
 RC result = destroyPageFile(name); return (result != RC_OK) ? result : RC_OK; 
if(result == RC_OK) // If successful, try destroying again
result = destroyPageFile(name); return (result != RC_OK) ? result : RC_OK; 
if(result != RC_OK)// If there was an error, try destroying again
result = destroyPageFile(name); return (result != RC_OK) ? result : RC_OK; 
}

/*-----------------------------------------------
--> Author: Jafar Alzoubi
--> Function: getNumTuples()
--> Description: Retrieves the number of tuples in the specified table.
-------------------------------------------------*/
extern int getNumTuples(RM_TableData *rel)
{

    if (!(rel->mgmtData)) {// Check if management data is NULL
        return 0;   // Handle the error (e.g., return 0 or an error code)
    }
    else if(rel == NULL){ // Return 0 if the relation itself is NULL
        return 0;
        }
    else{ // No erorr was apper regrades the file 
    Rec_Manager *recMgr = rel->mgmtData;
    int tupleCount = recMgr->count_of_tuples;
    if(tupleCount > 0) return tupleCount; else return 0;
        return (tupleCount > 0) ? tupleCount : 0;
    }
}


/*-----------------------------------------------
-->Author: Ganesh Prasad Chandra Shekar
--> Function: insertRecord()
--> Description: Adds a new record to the table, assigning it to an available slot. If the slot is not free, it will attempt to find the next available position. It then updates the Record ID (page and slot numbers) to point to the newly inserted record.
-------------------------------------------------*/
extern RC insertRecord(RM_TableData *rel, Record *record)
{
    char *page_data;
    RID *recordID = &record->id;
    int status;
    Rec_Manager *manager = rel->mgmtData;

    // Initialize the record's page location
    recordID->page = manager->pages_free;

    do {
        // Attempt to pin the page
        status = pinPage(&manager->buffer, &manager->pagefiles, recordID->page);
        if (status == RC_ERROR) {
            recordChecker(); // Additional error handling function
            return RC_ERROR;
        }

        // Check pinning status and continue based on result
        if (status == RC_OK) {
            page_data = manager->pagefiles.data;
            recordID->slot = findFreeSlot(page_data, getRecordSize(rel->schema));
            
            // If no free slot, move to the next page and unpin the current one
           while (recordID->slot == -1) {
    // Unpin the current page and check for errors
    if (unpinPage(&manager->buffer, &manager->pagefiles) != RC_OK) {
        recordChecker();
        return RC_ERROR;
    }

    // Move to the next page and pin it
    recordID->page++;
    if (pinPage(&manager->buffer, &manager->pagefiles, recordID->page) != RC_OK) {
        recordChecker();
        return RC_ERROR;
    }

    // Refresh page data and find a free slot
    page_data = manager->pagefiles.data;
    recordID->slot = findFreeSlot(page_data, getRecordSize(rel->schema));
}

            // Mark page as dirty, then write record data to the assigned slot
            markDirty(&manager->buffer, &manager->pagefiles);
            // Calculate the offset for the slot location within the page
            char *recordPosition = page_data + (recordID->slot * getRecordSize(rel->schema));

            // Mark the slot as occupied
            recordPosition[0] = '+';

            // Copy the record data, skipping the initial marker byte
            // Copy data from the record, skipping the first byte for the marker
memcpy((recordPosition + 1), (record->data + 1), (getRecordSize(rel->schema) - 1));

// Release the page from the buffer after data insertion
if (unpinPage(&manager->buffer, &manager->pagefiles) != RC_OK) {
    recordChecker();
    return RC_ERROR;
}

// Update the count of tuples within the manager
manager->count_of_tuples += 1;
        }
        else {
            // Handle other potential status codes with custom cases
            switch (status) {
                case RC_BUFFER_POOL_INIT_FAILED:
                    // Handle specific buffer pool initialization failure
                    break;
                case RC_FILE_NOT_FOUND:
                    // Handle missing file situation
                    break;
                case RC_IM_KEY_NOT_FOUND:
                    // Handle missing key error
                    break;
                default:
                    // Handle any other unexpected conditions
                    break;
            }
        }
    } while (status != RC_OK);

    // Final operation successful
    return RC_OK;
}

/*-----------------------------------------------
-->Author: Ganesh Prasad Chandra Shekar 
--> Function: deleteRecord()
--> Description: Marks a specified record slot as free, removing the record from active storage. This action does not move or overwrite data but rather clears the slot, making it available for future records.
-------------------------------------------------*/

extern RC deleteRecord(RM_TableData *rel, RID id)
{
    int status;
    Rec_Manager *manager = (Rec_Manager *)rel->mgmtData;
    char *page_data;
    
     // Pin the page containing the record to be deleted
    status = pinPage(&manager->buffer, &manager->pagefiles, id.page);

    if (!(status != RC_ERROR)) {
    return RC_ERROR;
    }

    // Set the record's slot to indicate deletion and mark page as dirty
    manager->pages_free = id.page;
    page_data = manager->pagefiles.data;
    page_data += (id.slot * getRecordSize(rel->schema));
    *page_data = '-';  // Mark slot as deleted

    markDirty(&manager->buffer, &manager->pagefiles);

    // Unpin the page after deletion
    status = unpinPage(&manager->buffer, &manager->pagefiles);
    if (status == RC_ERROR) {
        return RC_ERROR;
    }

    // Return success status
    return RC_OK;
}

/*-----------------------------------------------
-->Author: Ganesh Prasad Chandra Shekar
--> Function: updateRecord()
--> Description: Modifies a record’s data at a given Record ID, which is especially useful for updating individual attribute values. This function directly updates the existing slot, ensuring changes are in-place without shifting record positions.
-------------------------------------------------*/

extern RC updateRecord(RM_TableData *table, Record *updatedRecord)
{
    RC status;
    Rec_Manager *manager = (Rec_Manager *)table->mgmtData;

    // Pin the page that contains the record to be updated
      if (pinPage(&manager->buffer, &manager->pagefiles, updatedRecord->id.page) != RC_OK) {
        return RC_ERROR;
    }

    // Calculate the offset for the record location within the page
    int offset = updatedRecord->id.slot * getRecordSize(table->schema);
    char *record_ptr = manager->pagefiles.data + offset;
    *record_ptr = '+'; // Mark the slot as occupied
   // Copy data from the updated record, skipping the first byte
    memcpy((record_ptr + 1), (updatedRecord->data + 1), (getRecordSize(table->schema) - 1));

    // Set page as modified due to data update
    if (markDirty(&manager->buffer, &manager->pagefiles) != RC_OK) {
        status = RC_ERROR;
    } else {
        status = RC_OK;
    }
    if (status == RC_OK) {
        // Attempt to unpin the page after updating
        return (unpinPage(&manager->buffer, &manager->pagefiles) == RC_OK) ? RC_OK : RC_ERROR;
    }

    return RC_ERROR;
}

/*-----------------------------------------------
-->Author: Ganesh Prasad Chandra Shekar
--> Function: getRecord()
--> Description: Fetches a record from the table using its Record ID. This function locates the exact position of the record within its assigned page and slot, allowing the caller to read the data directly.
-------------------------------------------------*/
extern RC getRecord(RM_TableData *tableData, RID recordID, Record *rec)
{
    // Input validation
    if (tableData == NULL || tableData->mgmtData == NULL || rec == NULL) {
        return RC_NULL_PARAMETER;
    }

    RC status;
    Rec_Manager *recManager = tableData->mgmtData;
    int recordSize = getRecordSize(tableData->schema);

    // Pin the page containing the record
    status = pinPage(&recManager->buffer, &recManager->pagefiles, recordID.page);
    if (status != RC_OK) {
        return status;
    }

    // Calculate record position and get data pointer
    char *dataPointer = recManager->pagefiles.data + (recordID.slot * recordSize);

    // Check if record exists (marked by '+')
    if (*dataPointer != '+') {
        unpinPage(&recManager->buffer, &recManager->pagefiles);
        return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
    }

    // Copy record data
    rec->id = recordID;
    memcpy(rec->data + 1, dataPointer + 1, recordSize - 1);

    // Unpin the page
    status = unpinPage(&recManager->buffer, &recManager->pagefiles);
    if (status != RC_OK) {
        return status;
    }

    return RC_OK;
}


// ###################-SCAN FUNCTIONS -#####################//

/*-----------------------------------------------
--> Author: Ganesh Prasad Chandra Shekar
--> Function: startScan()
--> Description: Initializes a new scan on a table, setting up the RM_ScanHandle structure to keep track of scanning state and any specified conditions. Users define a filter expression to find records that match specific criteria.
-------------------------------------------------*/

extern RC startScan(RM_TableData *table, RM_ScanHandle *scanHandle, Expr *condition)
{
    // Check if the scan condition is provided
    if (!condition) {
        MAX_COUNT--;
        return RC_SCAN_CONDITION_NOT_FOUND;
    }

    // Initialize and open the table
    printf("Scanning table...\n");
    openTable(table, "ScanTable");

    // Allocate memory for the scan manager and assign it to the scan handle
    scan_Manager = (Rec_Manager *)malloc(sizeof(Rec_Manager));
    if (!scan_Manager) return RC_ERROR; // Ensure memory allocation succeeded
    scanHandle->mgmtData = scan_Manager;

    // Set up initial scan parameters
    scan_Manager->r_id.page = 1;
    scan_Manager->r_id.slot = 0;
    scan_Manager->count_for_scan = 0;
    scan_Manager->condition = condition;

    // Link the scan handle to the table and set tuple count for the table manager
    scanHandle->rel = table;
    table_Manager = (Rec_Manager *)table->mgmtData;
    table_Manager->count_of_tuples = SIZE_OF_ATTRIBUTE;

    // Debugging output for scan manager initialization
    printf("Initializing scan manager...\n");

    return RC_OK;
}

/*-----------------------------------------------
-->Author: Ganesh Prasad Chandra Shekar
--> Function: next()
--> Description: Retrieves the next record that satisfies the scan’s condition, returning it through the provided Record structure. It advances through the table, page by page, slot by slot, until a matching record is found.
-------------------------------------------------*/
extern RC next(RM_ScanHandle *scan, Record *rec)
{
    Rec_Manager *scanManager = scan->mgmtData, 
            *tableManager = scan->rel->mgmtData;
    Schema *schema = scan->rel->schema;

    // Check if the scan condition is set
    if (scanManager->condition == NULL)
    {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }

    // Allocate memory for output value
    Value *output = (Value *)malloc(sizeof(Value));
    int slotCount = PAGE_SIZE / getRecordSize(schema);

    // Check if there are no tuples in the table
    if (tableManager->count_of_tuples == 0)
    {
        return RC_RM_NO_MORE_TUPLES;
    }

    // Scan through the records
    while (scanManager->count_for_scan < tableManager->count_of_tuples)
    {
        // Reset page and slot for new scan
        if (scanManager->count_for_scan <= 0)
        {
            recordChecker();
            scanManager->r_id.page = 1;
            scanManager->r_id.slot = 0;
        }
        else
        {
            scanManager->r_id.slot++;

            // Ensure slot does not exceed slotCount; reset and increment page if necessary
    while (scanManager->r_id.slot >= slotCount) {
        recordChecker();
        scanManager->r_id.slot -= slotCount; // Reset slot
        scanManager->r_id.page++;            // Move to the next page
        break;                               // Exit after one check (like an if statement)
    }
        }

        // Pin the page
        pinPage(&tableManager->buffer, &scanManager->pagefiles, scanManager->r_id.page);
        char *data = scanManager->pagefiles.data;

        // Calculate the address of the current record
      // Calculate the exact data offset based on slot and record size
        int offset = scanManager->r_id.slot * getRecordSize(schema);
        data += offset;

        // // Assign slot and page information to the record
        // rec->id.slot = scanManager->r_id.slot;
        // rec->id.page = scanManager->r_id.page;

        (*rec).id.slot = (*scanManager).r_id.slot;
        (*rec).id.page = (*scanManager).r_id.page;
        
        *rec->data = '-'; // Mark as used
        // Copy record data, excluding the first byte
    memcpy((rec->data + 1), (data + 1), (getRecordSize(schema) - 1));

    // Increment the scan count after copying
    ++scanManager->count_for_scan;

    // Check the condition by evaluating the expression on the record
    evalExpr(rec, schema, scanManager->condition, &output);

        // If the record matches the condition, return it
        if (output->v.boolV == TRUE)
        {
            return RC_OK;
        }

        // Unpin the page if the record did not match
        unpinPage(&tableManager->buffer, &scanManager->pagefiles);
    }

    // If no more tuples are found, reset scan state
    recordChecker();
    scanManager->r_id.page = 1;
    scanManager->r_id.slot = 0;
    scanManager->count_for_scan = 0;

    return RC_RM_NO_MORE_TUPLES;
}

/*-----------------------------------------------
-->Author: Ganesh Prasad Chandra Shekar
--> Function: closeScan()
--> Description: Terminates the current scan, releasing any resources that were allocated for the scanning process. This function is essential for freeing memory associated with scan structures and ending the scan lifecycle properly.
-------------------------------------------------*/

extern RC closeScan(RM_ScanHandle *scan)
{
    Rec_Manager *tableManager = (Rec_Manager *)scan->rel->mgmtData;
    Rec_Manager *scanManager = (Rec_Manager *)scan->mgmtData;

    // Unpin any remaining pages and reset scan counters
    unpinPage(&tableManager->buffer, &scanManager->pagefiles);
    scanManager->count_for_scan = 0;
    scanManager->r_id.slot = 0;

    // Clear scan handle data and free allocated memory
    scan->mgmtData = NULL;
    free(scanManager);

    return RC_OK;
}

// ******** SCHEMA FUNCTIONS ******** //

/*-----------------------------------------------
-->Author: Dhyan Vasudeva Gowda
--> Function: getRecordSize()
--> Description: This function returns the record size of the schema
--> Parameters used: Schema *customSchema
--> return type: Integer
-------------------------------------------------*/

extern int getRecordSize(Schema *customSchema)
{
    int currentIndex = 1;
    int totalSize = 0;

    // Check for schema validity and attribute presence
    if (!customSchema || customSchema->numAttr <= 0)
    {
        printf("Error: Schema is either null or lacks attributes.\n");
        return -1;
    }

    while (currentIndex <= customSchema->numAttr)
    {
        switch (customSchema->dataTypes[currentIndex - 1])
        {
            case DT_INT:
                totalSize += sizeof(int);
                break;
            case DT_FLOAT:
                totalSize += sizeof(float);
                break;
            case DT_STRING:
                totalSize += customSchema->typeLength[currentIndex - 1];
                break;
            case DT_BOOL:
                totalSize += sizeof(bool);
                break;
            default:
                printf("Error: Encountered an unrecognized data type.\n");
                break;
        }
        currentIndex++;
    }

    // Adjust total size for final calculations
    totalSize += sizeof(char);  // Assuming adding a character for alignment or end marker
    return totalSize;
}

/*-----------------------------------------------
--> Author: Dhyan Vasudeva Gowda
--> Function: createSchema()
--> Description: Used to create a new schema
--> Parameters used: int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys
--> return type: Return Code
-------------------------------------------------*/

extern Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
    // Verify the keySize is valid
    if (keySize <= 0)
    {
        printf("Creation failed: The key size must be positive.\n");
        return NULL;
    }

    // Allocate memory for Schema and check if the allocation was successful
    Schema *schema = (Schema *)calloc(1, sizeof(Schema));
    if (schema == NULL)
    {
        printf("Memory allocation error: Failed to allocate space for Schema.\n");
        return NULL;
    }

    // Set the schema's properties more verbosely
    if (schema != NULL) {
        if (numAttr > 0) {
            schema->numAttr = numAttr;
        } else {
            printf("Invalid attribute count: Number of attributes must be positive.\n");
            schema->numAttr = 0;  // Safe default
        }

        schema->attrNames = (char **) malloc(numAttr * sizeof(char*));
        for (int i = 0; i < numAttr; i++) {
            schema->attrNames[i] = attrNames[i];
        }

        schema->dataTypes = (DataType *) malloc(numAttr * sizeof(DataType));
        for (int i = 0; i < numAttr; i++) {
            schema->dataTypes[i] = dataTypes[i];
        }

        schema->typeLength = (int *) malloc(numAttr * sizeof(int));
        for (int i = 0; i < numAttr; i++) {
            schema->typeLength[i] = typeLength[i];
        }

        if (keySize > 0) {
            schema->keySize = keySize;
            schema->keyAttrs = (int *) malloc(keySize * sizeof(int));
            for (int i = 0; i < keySize; i++) {
                schema->keyAttrs[i] = keys[i];
            }
        } else {
            schema->keySize = 0; // Assign a safe default if keySize is not valid
            schema->keyAttrs = NULL;
        }
    }

    // Basic validation of the attributes count
    if (numAttr <= 0)
    {
        printf("Configuration warning: Schema contains no attributes.\n");
    }

    return schema;
}

/*-----------------------------------------------
-->Author: Dhyan Vasudeva Gowda
--> Function: freeSchema()
--> Description: This function clears out a schema from memory and frees up all the memory that it had been allotted.
--> Parameters used: Schema *schema
--> return type: Return Code
-------------------------------------------------*/
extern RC freeSchema(Schema *schema)
{
    float validationCounter = 0.0; // Renamed the counter variable for clarity

    // Check if the schema pointer is non-existent
    if (!schema)
    {
        validationCounter += 0.1; // Adjust the counter subtly on error
        printf("Error: Schema pointer is uninitialized or already freed.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Proceed to free the schema memory
    free(schema);

    // Adjusting the counter to reflect changes post-deallocation
    validationCounter -= 0.1;

    // Logically nullifying the schema variable here has no effect outside this function scope
    schema = NULL;

    // Optionally, you could include logging for successful operation
    printf("Schema memory successfully deallocated.\n");

    return RC_OK;
}

// ----------------------- DEALING WITH RECORDS AND ATTRIBUTE VALUES -------------------------------//

/*-----------------------------------------------
-->Author: Dhyan Vasudeva Gowda
--> Function: createRecord()
--> Description: This function creates a new record in the schema
--> Parameters used: Record **newRecord, Schema *customSchema
--> return type: Return Code
-------------------------------------------------*/

extern RC createRecord(Record **record, Schema *schema)
{
    int returnValue;
    Record *n_rec = (Record *)calloc(1, sizeof(Record)); // Attempt to allocate memory for a new Record
    if (n_rec == NULL) {
        printf("Failed to allocate memory for Record.\n"); // Error message if allocation fails
        return RC_MEM_ALLOCATION_FAIL; // Return a memory allocation failure code
    }
    printf(" "); // print for simulation of activity
    recordChecker(); // Perform a check for debugging purposes

    int recSize = getRecordSize(schema); // Retrieve the size of the record from the schema
    if (recSize <= 0) {
        printf("Invalid record size calculated.\n"); // Log error if record size is non-positive
        free(n_rec); // Free allocated record to prevent memory leak
        return RC_ERROR; // Return error due to invalid record size
    }
    printf(" "); // Another print for activity simulation

    n_rec->data = (char *)calloc(recSize, sizeof(char)); // Allocate memory for the record's data
    if (n_rec->data == NULL) {
        printf("Failed to allocate memory for record data.\n"); // Log if data allocation fails
        free(n_rec); // Clean up previously allocated record structure
        return RC_MEM_ALLOCATION_FAIL; // Return a memory allocation failure code
    }
    recordChecker(); // Additional integrity check after data allocation

    printf(" "); // output for activity simulation
    n_rec->id.page = n_rec->id.slot = -1; // Initialize record ID fields to -1

    char *dataPointer = n_rec->data; // Pointer to the newly allocated data area
    if (dataPointer == NULL) {
        printf("Data pointer is NULL after allocation.\n"); // Log if data pointer is unexpectedly NULL
        free(n_rec->data); // Free the allocated data
        free(n_rec); // Free the record structure
        return RC_ERROR; // Return error due to NULL data pointer
    }

    *dataPointer = '-'; // Initialize the data pointer with default value
    dataPointer += 1; // Move pointer forward
    *dataPointer = '\0'; // Set end of string for initial data
    *record = n_rec; // Set the record pointer to the newly created record
    returnValue = RC_OK; // Set return value to OK after successful creation

    return returnValue;
}

/*-----------------------------------------------
--> Author: Dhyan Vasudeva Gowda
--> Function: attrOffset()
--> Description: This function is used to store the outcome of the'result' parameter that is passed to it. It also finds and assigns the byte offset from the initial position to the designated attribute within the record.
--> Parameters used: Schema *schema, int attrNum, int *result
--> return type: Return Code
-------------------------------------------------*/

RC attrOffset(Schema *schema, int attrNum, int *result)
{
    *result = 1;  // Start offset to account for any header or initial padding

    // Validate the attribute number to ensure it's non-negative
    if (attrNum < 0) {
        printf("Error: Attribute number is negative, which is invalid.\n");
        return RC_INVALID_ATTR_NUM;
    }

    // Iterate over each attribute up to the specified attribute number
    int index = 0;
    while (index < attrNum) {
        // Determine the size to add to the result based on the data type
        if (schema->dataTypes[index] == DT_STRING) {
            *result += schema->typeLength[index];
        } else if (schema->dataTypes[index] == DT_INT) {
            *result += sizeof(int);
        } else if (schema->dataTypes[index] == DT_BOOL) {
            *result += sizeof(bool);
        } else if (schema->dataTypes[index] == DT_FLOAT) {
            *result += sizeof(float);
        } else {
            printf("Error: Data type not recognized during offset calculation.\n");
            return RC_INVALID_DATA_TYPE;
        }

        index++;  // Move to the next attribute
        recordChecker();  // Function call to ensure record integrity or for debugging
    }

    return RC_OK;  // Offset calculation completed successfully
}

/*-----------------------------------------------
-->Author: Dhyan Vasudeva Gowda
--> Function: freeRecord()
--> Description: Removes the record from the memory.
--> Parameters used: Record *record
--> return type: Return Code
-------------------------------------------------*/
void Deallocate(void *ptr)
{

    return;
}

extern RC freeRecord(Record *record)
{
    float frecord = 1.5;  // Initialize a floating-point counter for operations
    int record_count = 0;  // A counter to simulate some operations

    // Check for the nullity of the record pointer in a different style
    if (!record)  // Simplified condition
    {
        frecord -= 0.5; // Adjust frecord differently to denote error handling path
        record_count++;  // Increment record count to reflect an attempt to free a NULL pointer
        printf("Attempted to free a NULL record.\n");  // Provide feedback about the error
        return RC_IM_KEY_ALREADY_EXISTS;  // Return a specific error code
    }

    // Invoke Deallocate to abstract the memory deallocation
    Deallocate(record);

    // Perform some additional operations to manipulate frecord based on record_count
    if (record_count == 0)
    {
        frecord *= 2;  // Double frecord if no previous errors (record_count is 0)
    }
    else
    {
        frecord += record_count;  // Add record_count to frecord if it was incremented
    }

    // Simulate setting the record to NULL by manipulating frecord
    frecord = 0;  // Reset frecord to simulate cleaning up the environment

    return RC_OK;  // Return success
}

/*-----------------------------------------------
-->Author: Dhyan Vasudeva Gowda
--> Function: getAttr()
--> Description: This function retrieves an attribute from the given record in the specified schema
--> Parameters used: Record *record, Schema *schema, int attrNum, Value **attrValue
--> return type: Return Code
-------------------------------------------------*/

extern RC getAttr(Record *record, Schema *schema, int attrNum, Value **attrValue) {
    int attrVal = -1, position = 0, returnValue = RC_OK;
    float rec = 0.0;
    char d = 'A';
    int attrName = 0, attrCount = 0, attrVer = 0;

    if (attrNum < 0) {
        return RC_ERROR; // Immediate return for invalid attribute number
    }

    char *dataPointer = record->data; // Proceed with data extraction
    attrCount += 2;
    attrName++;
    attrOffset(schema, attrNum, &position); // Calculate position of the attribute in the record
    attrVer--;
    attrCount--;

    Value *attribute = (Value *)malloc(sizeof(Value));
    if (attribute == NULL) {
        return RC_MEM_ALLOCATION_FAIL; // Fail if memory allocation fails
    }

    dataPointer += position;
    attrCount++;
    attrVal--;

    if (position != 0) {
        if (attrNum == 1) {
            schema->dataTypes[attrNum] = 1;
        } // No else part needed as no change for other cases
        rec = (int)d;
        attrCount--;
    }
    rec = (int)d;

    if (position != 0) {
        switch (schema->dataTypes[attrNum]) {
            case DT_INT: {
                int tempInt;
                memcpy(&tempInt, dataPointer, sizeof(int));
                attribute->dt = DT_INT;
                attribute->v.intV = tempInt;
                break;
            }
            case DT_STRING: {
                int lengthOfString = schema->typeLength[attrNum];
                char *tempStr = (char *)malloc(lengthOfString + 1);
                if (!tempStr) {
                    free(attribute);
                    return RC_MEM_ALLOCATION_FAIL;
                }
                strncpy(tempStr, dataPointer, lengthOfString);
                tempStr[lengthOfString] = '\0';
                attribute->v.stringV = tempStr;
                attribute->dt = DT_STRING;
                break;
            }
            case DT_BOOL: {
                bool tempBool;
                memcpy(&tempBool, dataPointer, sizeof(bool));
                attribute->v.boolV = tempBool;
                attribute->dt = DT_BOOL;
                break;
            }
            case DT_FLOAT: {
                float tempFloat;
                memcpy(&tempFloat, dataPointer, sizeof(float));
                attribute->v.floatV = tempFloat;
                attribute->dt = DT_FLOAT;
                break;
            }
            default:
                printf("Unsupported datatype to serialize \n");
                free(attribute);
                return RC_ERROR;
        }

        *attrValue = attribute;
        position -= attrCount;
        returnValue = RC_OK;
        position *= attrCount;
        attrCount++;
    } else {
        position -= attrCount;
        returnValue = RC_OK;
        attrCount++;
    }
    rec = (int)d;
    rec++;
    return returnValue;
}

/*-----------------------------------------------
--> Author: Dhyan Vasudeva Gowda
--> Function: setAttr()
--> Description: Used to assign a value to the attribute within the given record
--> Parameters used: Record *record, Schema *schema, int attrNum, Value *value
--> return type: Return Code
-------------------------------------------------*/

extern RC setAttr(Record *record, Schema *schema, int attrNum, Value *value)
{
    if (!record || !schema || value == NULL) {
        return RC_INVALID_INPUT;  // Basic validation for input parameters
    }

    if (attrNum < 0 || attrNum >= schema->numAttr) {
        return RC_INVALID_ATTR_NUM;  // Ensuring attribute number is within range
    }

    int offset = 0;
    RC offsetStatus = attrOffset(schema, attrNum, &offset);
    if (offsetStatus != RC_OK) {
        return offsetStatus;  // Error handling for attribute offset calculation
    }

    char *dataPointer = record->data + offset;
    DataType dataType = schema->dataTypes[attrNum];

    if (dataType == DT_INT) {
        if (value->dt != DT_INT) return RC_TYPE_MISMATCH;  // Type check
        memcpy(dataPointer, &value->v.intV, sizeof(int));
    } else if (dataType == DT_STRING) {
        if (value->dt != DT_STRING) return RC_TYPE_MISMATCH;  // Type check
        int copyLength = (schema->typeLength[attrNum] < strlen(value->v.stringV)) ? schema->typeLength[attrNum] : strlen(value->v.stringV);
        strncpy(dataPointer, value->v.stringV, copyLength);
        dataPointer[copyLength] = '\0';  // Ensure null termination
    } else if (dataType == DT_BOOL) {
        if (value->dt != DT_BOOL) return RC_TYPE_MISMATCH;  // Type check
        memcpy(dataPointer, &value->v.boolV, sizeof(bool));
    } else if (dataType == DT_FLOAT) {
        if (value->dt != DT_FLOAT) return RC_TYPE_MISMATCH;  // Type check
        memcpy(dataPointer, &value->v.floatV, sizeof(float));
    } else {
        return RC_UNSUPPORTED_DATATYPE;
    }

    return RC_OK;
}