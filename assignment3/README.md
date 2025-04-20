## CS 525- ASSIGNMENT 3

## Team Members

| #   | Name                         | CWID      | Email                       | Contribution |
| --- | ---------------------------- | --------- | --------------------------- | ------------ |
| 1   | Jafar Alzoubi                | A20501723 | jalzoubi@hawk.iit.edu       | 33.33%       |
| 2   | Dhyan Vasudeva Gowda         | A20592874 | dgowda2@hawk.iit.edu        | 33.33%       |
| 3   | Ganesh Prasad Chandra Shekar | A20557831 | gchandrashekar@hawk.iit.edu | 33.33%       |

This assignment aims to implement a Record Manager. The Record Manager enables efficient handling of fixed-schema tables, supporting operations such as inserting, deleting, updating, and scanning records. Each table is stored in a dedicated page file, with records organized and managed through a buffer manager designed in a previous assignment.

## CORE FEATURES:

    •	Table and Record Management: Creation, deletion, opening, and closing of tables with fixed schemas.
    •	Record Operations: Insertion, retrieval, update, and deletion of records by unique Record IDs.
    •	Scanning with Conditions: Querying records that match specific conditions, with efficient scan initialization and progression.
    •	Schema and Attribute Handling: Creating schemas, managing attributes, and retrieving and setting attribute values.

## RUNNING THE PROJECT:

    1.	Navigate to the Project Directory
        Open a terminal and navigate to the assign3 folder.
    2.	Clean Up Old Files
        Run "make clean" to remove any previous compiled files.
    3.	Compile the Project
        Run "make" to compile all necessary files, including test_assign3_1.c.
    4.	Execute the Test Script
        Use "make run" to execute the test_assign3_1.c program, which verifies the functionality of the Record Manager.

## SOLUTION DETAILS:

1. Table and Record Manager Functions

These functions handle the setup, management, and cleanup of tables within the Record Manager. They enable users to create tables with specified schemas, open and close tables for interaction, and retrieve metadata about tables.
############################################
AUTHOR: Jafar Alzoubi
############################################
• initRecordManager:

Initializes the Record Manager environment, preparing it to manage tables and records. This function sets up necessary components and links the Record Manager to the Storage Manager, which handles low-level data storage.
############################################
AUTHOR: Jafar Alzoubi
############################################
• shutdownRecordManager:

Cleans up the resources used by the Record Manager. This is often the final step in the application lifecycle to ensure that any allocated memory or resources are properly released to avoid memory leaks.
############################################
AUTHOR: Jafar Alzoubi
############################################
• createTable:

Creates a new table with a specified schema, allocating a page file for data storage. This function assigns an initial layout, including metadata for managing records within the table. It verifies that the schema attributes align with the provided structure before creating the table.
############################################
AUTHOR: Jafar Alzoubi
############################################
• openTable and closeTable:
• openTable: Prepares a table for access, allowing records to be inserted, updated, deleted, or scanned. It reads table metadata, making the table active in memory.
• closeTable:
Finalizes table interactions by writing any unsaved changes back to storage. This function is essential for persisting data and avoiding data loss from unsaved modifications.
############################################
AUTHOR: Jafar Alzoubi
############################################
• deleteTable:
Deletes the table from both memory and storage, freeing all resources and deallocating any pages associated with it.
############################################
AUTHOR: Jafar Alzoubi
############################################
• getNumTuples:
Returns the number of records currently stored in the table, enabling users to monitor table size for performance or data integrity purposes.

2. Record Functions

Record functions manage individual records within tables, enabling the insertion, retrieval, modification, and deletion of records.

    ############################################
    AUTHOR: Ganesh Prasad Chandra Shekar
    ############################################
    •	insertRecord:

    Adds a new record to the table, assigning it to an available slot. If the slot is not free, it will attempt to find the next available position. It then updates the Record ID (page and slot numbers) to point to the newly inserted record.

    ############################################
    AUTHOR: Ganesh Prasad Chandra Shekar
    ############################################
    • deleteRecord:

    Marks a specified record slot as free, removing the record from active storage. This action does not move or overwrite data but rather clears the slot, making it available for future records.

    ############################################
    AUTHOR: Ganesh Prasad Chandra Shekar
    ############################################
    • updateRecord:
    Modifies a record’s data at a given Record ID, which is especially useful for updating individual attribute values. This function directly updates the existing slot, ensuring changes are in-place without shifting record positions.

    ############################################
    AUTHOR: Ganesh Prasad Chandra Shekar
    ############################################
    • getRecord:
    Fetches a record from the table using its Record ID. This function locates the exact position of the record within its assigned page and slot, allowing the caller to read the data directly.

3. Scan Functions

Scan functions allow the Record Manager to search for records based on conditions, providing a way to filter and retrieve subsets of data. Scanning is essential for query processing and analytical operations.

    ############################################
    AUTHOR: Ganesh Prasad Chandra Shekar
    ############################################
    •	startScan:
    Initializes a new scan on a table, setting up the RM_ScanHandle structure to keep track of scanning state and any specified conditions. Users define a filter expression to find records that match specific criteria.

    ############################################
    AUTHOR: Ganesh Prasad Chandra Shekar
    ############################################
    • next:
    Retrieves the next record that satisfies the scan’s condition, returning it through the provided Record structure. It advances through the table, page by page, slot by slot, until a matching record is found.

    ############################################
    AUTHOR: Ganesh Prasad Chandra Shekar
    ############################################
    • closeScan:
    Terminates the current scan, releasing any resources that were allocated for the scanning process. This function is essential for freeing memory associated with scan structures and ending the scan lifecycle properly.

4. Schema Functions

These functions manage table schemas, which define the structure of tables, including attribute names, data types, and sizes.

    ############################################
    AUTHOR: Dhyan Vasudeva Gowda
    ############################################
    •	getRecordSize:
    Calculates the size (in bytes) of a single record based on the schema. It multiplies each attribute size by its type’s byte requirements, summing them to get the total record size.

    ############################################
    AUTHOR: Dhyan Vasudeva Gowda
    ############################################
    • createSchema:
    Sets up a new schema by allocating memory for its attributes and initializing attribute data such as types and sizes. This function returns a schema structure that describes the layout for records.

    ############################################
    AUTHOR: Dhyan Vasudeva Gowda
    ############################################
    • freeSchema:
    Releases memory used by a schema, ensuring that no unused memory lingers. This is particularly important in applications that create multiple schemas or tables dynamically.

5. Attribute Functions

Attribute functions deal with retrieving and setting values within individual records, managing the attributes according to their data types.

    ############################################
    AUTHOR: Dhyan Vasudeva Gowda
    ############################################
    •   createRecord:
    Allocates memory for a new record according to the schema and initializes its attributes to default values. This function prepares a blank record structure, ready to be populated with data.

    ############################################
    AUTHOR: Dhyan Vasudeva Gowda
    ############################################
    •   attrOffset:
    This function is used to store the outcome of the'result' parameter that is passed to it. It also finds and assigns the byte offset from the initial position to the designated attribute within the record.

    ############################################
    AUTHOR: Dhyan Vasudeva Gowda
    ############################################
    • freeRecord:
    Deallocates memory occupied by a record. This function is necessary for cleaning up records that are no longer needed, especially in scenarios where records are created frequently.

    ############################################
    AUTHOR: Dhyan Vasudeva Gowda
    ############################################
    • getAttr and setAttr:
    getAttr: Retrieves an attribute value from a specified record. It locates the attribute within the record based on its position and type in the schema.
    setAttr: Assigns a new value to a specified attribute within a record. This function directly updates the value in the record’s data, ensuring data type consistency.

## IMPLEMENTATION NOTES:

This Record Manager relies on fixed-length data types and layouts, simplifying space management and record access. Key aspects include:

    •	Page Layout: Each page is organized with slots representing fixed-size records. Unused slots can be marked as free for reuse.
    •	Record ID Management: Records are uniquely identified by a combination of page and slot numbers.
    •	Free Space Management: Pages with available space are tracked, allowing efficient insertion of new records into vacant slots.

## FOLDER STRUCTURE:

The main files in this assignment include:

    •	record_mgr.h and record_mgr.c: Define and implement the Record Manager.
    •	tables.h: Defines data structures for schemas, tables, and records.
    •	expr.h: Manages conditions and expressions for scanning records.
    •	test_assign3_1.c: Test cases to verify the Record Manager’s functionality.

## TESTING:

Two primary test files are provided:

    •	test_assign3_1.c: Tests core Record Manager functions.
    •	test_expr.c: Tests condition expressions and evaluation.

To run tests, use the commands listed in the “Running the Project” section.
