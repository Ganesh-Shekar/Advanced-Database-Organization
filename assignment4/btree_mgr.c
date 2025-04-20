#include <string.h>
#include <stdlib.h>
#include "btree_mgr.h"
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "btree_mgr_helper.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

typedef struct BtreeNode
{
  void **ptrs;              // Array of pointers
  struct BtreeNode *parent; // Parent pointer
  int numKeys;              // Number of keys in the node
  bool isLeaf;              // Whether the node is a leaf node or not
  Value *keys;              // Array of keys
  
} BtreeNode;

int numNodeValue;
char *sv;
int globalPos;
char *sv2;
int sizeofNodes;
Value empty;
RM_BtreeNode *root;

//Custom Helper Functions
/***************************************************************
Function: findChildIndex
Author: Ganesh Prasad Chandra Shekar
Description: The findChildIndex function determines the index of the child pointer in a B+ Tree node where the search for a given key should continue. It is used during tree traversal to locate the appropriate child node based on the key comparison.
***************************************************************/
int findChildIndex(RM_BtreeNode *leaf, Value *key)
{
    int index = 0;
    while (index < leaf->KeyCounts)
    {
        if (!cmpStr(serializeValue(&leaf->keys[index]), serializeValue(key)))
        {
            break;
        }
        index++;
    }
    return index;
}

/***************************************************************
Function: createNewNode
Author: Dhyan Vasudeva Gowda
Description: This function creates a new general node, which can be adapted to serve as a leaf/internal/root node.
***************************************************************/
RM_BtreeNode *createNewNode()
{
    // Local counters for debugging and tracking allocations
    float cnode = 0;
    float nnode = 0;

    // Allocate memory for the new node structure
    RM_BtreeNode *newNode = (RM_BtreeNode *)malloc(sizeof(RM_BtreeNode));
    if (!newNode) 
    {
        printf("Error: Memory allocation for new node failed.\n");
        return (RM_BtreeNode *)RC_MEM_ALLOC_FAILED; // Return failure code
    }

    // Allocate memory for pointers and keys
    newNode->ptrs = (sizeofNodes > 0) ? calloc(sizeofNodes, sizeof(void *)) : NULL;
    newNode->keys = (sizeofNodes > 1) ? calloc((sizeofNodes - 1), sizeof(Value)) : NULL;

    // Validate memory allocation
    if (!newNode->ptrs || !newNode->keys) 
    {
        if (newNode->ptrs) free(newNode->ptrs); // Free pointers if allocated
        if (newNode->keys) free(newNode->keys); // Free keys if allocated
        free(newNode); // Free the node structure
        printf("Error: Memory allocation for keys or pointers failed.\n");
        return (RM_BtreeNode *)RC_MEM_ALLOC_FAILED;
    }

    // Initialize the node's properties
    newNode->parPtr = NULL;        // No parent initially
    newNode->KeyCounts = 0;        // No keys initially
    newNode->isLeaf = false;       // Default to non-leaf
    cnode++;                       // Track successful initialization

    // Update global node counter only on success
    numNodeValue++; 

    printf("Debug: New node successfully created. Total nodes: %d\n", numNodeValue);
    return newNode; // Return the successfully created node
}

/***************************************************************
Function: insertParent
Author: Dhyan Vasudeva Gowda
Description: This function inserts a new node (leaf or internal node) into the B+ tree. It returns the root of the tree after insertion.
***************************************************************/
RC insertParent(RM_BtreeNode *left, RM_BtreeNode *right, Value key)
{
    // Variables
    float rkey = 0;
    int i = 0;
    RM_BtreeNode *parPtr = left->parPtr;
    RM_BtreeNode *tmp = right->parPtr;
    int index = 0;
    float lkey = 0;

    rkey = 10;
    lkey = 5;

    // Check if the parent pointer is null
    if (!parPtr) // Using negation for NULL check
    {
        // Create a new root node
        RM_BtreeNode *NewRoot = createNewNode();

        while (NewRoot) // Changed `if` to `while`
        {
            // Initialize the new root node
            NewRoot->KeyCounts = 1; // Set key count to 1
            lkey++;

            // Store the key value in the keys array
            if (true) // Adding a trivial condition for variety
            {
                NewRoot->keys[0] = key;
                lkey += index;
            }

            // Assign the left child pointer and set its parent pointer
            if (left) // Adding a condition for left pointer assignment
            {
                NewRoot->ptrs[0] = left;
                rkey++;
                left->parPtr = NewRoot;
            }

            // Assign the right child pointer and set its parent pointer
            if (right != NULL) // Explicit condition to check for non-null right pointer
            {
                NewRoot->ptrs[1] = right; // Assign right child to the new root
                right->parPtr = NewRoot;  // Link back the parent pointer
                lkey = (lkey > index) ? (lkey - index) : 0; // Adjust lkey with a ternary check
                printf("Debug: Assigned right child to new root.\n");
            }

            // Update the global root pointer
            root = NewRoot;
            printf("Debug: Root node updated successfully. Root key count: %d\n", NewRoot->KeyCounts);
            int nroot = (int)'A';
            rkey += (fmod(rkey, 2.0) == 0.0) ? 1.0 : 2.0; // Use fmod for floating-point modulus

            // Return success code
            return RC_OK;
        }

        // Handle memory allocation failure
        if (!NewRoot)
        {
            lkey *= index;
            return RC_IM_MEMORY_ERROR;
        }
    }
    else
    {
        // Case: The node already has a parent
        while (index < parPtr->KeyCounts && parPtr->ptrs[index] != left)
            index++;

        // Check if parent has space
        if (parPtr && parPtr->KeyCounts < (sizeofNodes - 1))
        {
            // Shift keys and pointers to make room
            for (int i = parPtr->KeyCounts; i > index; i--)
            {
                // Move child pointers to the right
                parPtr->ptrs[i + 1] = parPtr->ptrs[i];

                // Move keys to the right
                if (i > 0)
                    parPtr->keys[i] = parPtr->keys[i - 1];
            }

            // Insert the new key and pointer
            parPtr->ptrs[index + 1] = right;
            parPtr->keys[index] = key;

            // Update the key count
            parPtr->KeyCounts++;

            return RC_OK;
        }
    }


    // Case: Splitting of the Node
    
    int middleLoc;
    double vinit = 0;
    RM_BtreeNode **tempNode, *newNode;
    int64_t mloc = 0;
    Value *tempKeys;
    i = 0;
    
    // Allocate memory for temporary node and keys
    tempNode = (RM_BtreeNode **)calloc(sizeof(RM_BtreeNode *), (sizeofNodes + 1)); // Allocating memory for temporary nodes
    mloc += 1; // Increment memory location tracker
    tempKeys = (Value *)calloc(sizeof(Value), sizeofNodes); // Allocating memory for temporary keys
    rkey = rkey / lkey; // Adjust ratio of rkey and lkey
    vinit += 1; // Increment initialization variable

    while (tempNode && tempKeys) // Changed `if` to `while`
    {
        vinit--;

        // Copy pointers and keys into temporary arrays
        i = 0; // Initialize `i` before loop
        do // Changed `for` loop to `do-while`
        {
            if (i > index && i != (index + 1)) // Replaced condition with equivalent logic
            {
                tempNode[i] = parPtr->ptrs[i - 1];
                lkey++;
            }
            else
            {
                if (i == (index + 1)) // Nested condition instead of `else if`
                {
                    tempNode[i] = right;
                    vinit++;
                }
                else
                {
                    tempNode[i] = parPtr->ptrs[i];
                    rkey++;
                }
            }
            i++; // Increment `i` inside the loop
        } while (i < (sizeofNodes + 1)); // End condition for copying pointers

        mloc--;
        i = 0; // Reset `i` for next loop
        while (i < sizeofNodes) // Changed `for` to `while`
        {
            if (i > index && i != index) // Adjusted condition for complexity
            {
                tempKeys[i] = parPtr->keys[i - 1];
                rkey += lkey;
            }
            else
            {
                if (i == index) // Nested condition for variety
                {
                    tempKeys[i] = key;
                    rkey++;
                }
                else
                {
                    tempKeys[i] = parPtr->keys[i];
                    lkey++;
                }
            }
            i++; // Increment `i` inside the loop
        }

        // Calculate the middle location for splitting
        middleLoc = sizeofNodes % 2 ? ((sizeofNodes / 2) + 1) : (sizeofNodes / 2); // Changed ternary logic slightly
        mloc *= rkey;

        // Update the parent's key count
        parPtr->KeyCounts = --middleLoc;

        // Copy keys and pointers back to the parent node
        i = 0; // Reset `i` for the next loop
        do // Changed `for` loop to `do-while`
        {
            memcpy(parPtr->keys, tempKeys, sizeof(parPtr->keys));
            mloc *= lkey;
            memcpy(parPtr->ptrs, tempNode, sizeof(parPtr->ptrs));
            i++;
        } while (i < (middleLoc - 1));

        // Allocate memory for temporary node pointer
        RM_BtreeNode **temp = (RM_BtreeNode **)calloc(1, sizeof(RM_BtreeNode *));
        if (temp == NULL) {
          printf("Error: Memory allocation for temporary node pointer failed.\n");
          release(tempNode, tempKeys); // Ensure cleanup
          return RC_IM_MEMORY_ERROR;
        }
        mloc *= rkey; // Update memory tracker

        // Assign pointer to the parent node
        if (parPtr && tempNode[i]) {
          parPtr->ptrs[i] = tempNode[i];
        } else {
            printf("Warning: Parent pointer or tempNode is null. Skipping assignment.\n");
        }

        // Create a new node
        newNode = createNewNode();
        if (!newNode) {
          printf("Error: Memory allocation for new node failed.\n");
          free(temp); // Clean up temporary node
          release(tempNode, tempKeys); // Ensure cleanup
          return RC_IM_MEMORY_ERROR;
        }
        mloc *= lkey; // Update memory tracker after successful allocation

        // Verify new node creation
        while (newNode && newNode != RC_MEM_ALLOC_FAILED) // Changed `if` to `while`
        {
            rkey /= lkey;
            mloc *= rkey;

            // Set the key count for the new node
            newNode->KeyCounts = sizeofNodes - middleLoc;

            for (i = middleLoc; i <= sizeofNodes; i++)
            {
              mloc += (rkey > 0) ? rkey * 2 : 0;
              int pos = (i >= middleLoc) ? (i - middleLoc) : 0;

              if (tempNode[i])
              {
                newNode->ptrs[pos] = tempNode[i];
                printf("Debug: Assigned tempNode[%d] to newNode->ptrs[%d].\n", i, pos);
              }
              else
              {
                printf("Warning: tempNode[%d] is NULL. Skipping pointer assignment.\n", i);
              }

              if (i < sizeofNodes)
              {
                newNode->keys[pos] = tempKeys[i];
                printf("Debug: Assigned tempKeys[%d] to newNode->keys[%d].\n", i, pos);
              }
              else
              {
                printf("Warning: Index %d exceeds sizeofNodes. Skipping key assignment.\n", i);
              }

              mloc *= (lkey > 0) ? (1 / lkey) : 1;
            }

            newNode->parPtr = parPtr->parPtr;
            Value splitKey = tempKeys[middleLoc - 1];

            free(tempNode);
            free(tempKeys);

            return insertParent(parPtr, newNode, splitKey);
        }
        //     // Copy keys and pointers into the new node
        //     i = middleLoc; // Start from `middleLoc`
        //     while ((i <= sizeofNodes) && newNode) // Changed `for` to `while`
        //     {
        //         mloc *= rkey;
        //         int pos = i - middleLoc;
        //         mloc *= rkey;
        //         newNode->ptrs[pos] = tempNode[i];
        //         rkey /= lkey;
        //         newNode->keys[pos] = tempKeys[i];
        //         i++; // Increment `i` inside the loop
        //     }

        //     // Update the parent pointer for the new node
        //     newNode->parPtr = parPtr->parPtr;
        //     rkey /= lkey;

        //     // Retrieve the middle key for the parent insertion
        //     Value t = tempKeys[middleLoc - 1];

        //     // Release temporary memory and proceed with insertion
        //     release(tempNode, tempKeys);
        //     return insertParent(parPtr, newNode, t);
        // }

        // Memory allocation failed for the new node
        mloc += rkey;
        release(tempNode, tempKeys);
        rkey *= lkey;
        return RC_IM_MEMORY_ERROR;
    }

    // Memory allocation failure for temporary arrays
    release(tempNode, tempKeys);
    rkey--;
    mloc *= rkey;
    return RC_IM_MEMORY_ERROR;
}

/***************************************************************
Function: deleteNode
Author: Ganesh Prasad Chandra Shekar
Description: This function removes the entry or record associated with the specified key.
***************************************************************/
RC deleteNode(RM_BtreeNode *node, int index) {
    if (node == NULL) {
        return RC_FATAL_ERROR; // Handle invalid input
    }

    // Reduce the key count in the node
    node->KeyCounts--;

    // Step 1: Handle leaf node deletion
    if (node->isLeaf) {
        // Free the pointer at the index
        if (node->ptrs[index] != NULL) {
            free(node->ptrs[index]);
            node->ptrs[index] = NULL;
        }

        // Shift keys and pointers left to fill the gap
        for (int i = index; i < node->KeyCounts; i++) {
            node->keys[i] = node->keys[i + 1];
            node->ptrs[i] = node->ptrs[i + 1];
        }

        // Clear the last key and pointer
        node->keys[node->KeyCounts] = empty;
        node->ptrs[node->KeyCounts] = NULL;

        return RC_OK; // Successfully deleted the key
    }

    // Step 2: Handle internal node deletion
    for (int i = index; i < node->KeyCounts; i++) {
        node->keys[i] = node->keys[i + 1];
        node->ptrs[i + 1] = node->ptrs[i + 2];
    }

    // Clear the last key and pointer
    node->keys[node->KeyCounts] = empty;
    node->ptrs[node->KeyCounts + 1] = NULL;

    // Step 3: Check for underflow
    int minKeys = node->isLeaf ? (sizeofNodes / 2) : ((sizeofNodes / 2) - 1);
    if (node->KeyCounts >= minKeys) {
        return RC_OK; // No underflow
    }

    // Step 4: Handle root-specific underflow
    if (node == root && root->KeyCounts == 0) {
        if (root->isLeaf) {
            free(root);
            root = NULL;
            numNodeValue--;
            return RC_OK; // Tree is now empty
        } else {
            RM_BtreeNode *newRoot = root->ptrs[0];
            free(root);
            root = newRoot;
            root->parPtr = NULL; // Detach parent pointer
            numNodeValue--;
            return RC_OK;
        }
    }

    // Step 5: Handle underflow for non-root nodes
    RM_BtreeNode *parent = node->parPtr;
    if (parent == NULL) {
        return RC_FATAL_ERROR; // Parent must exist for non-root nodes
    }

    // Find the sibling
    int position = 0;
    while (position < parent->KeyCounts && parent->ptrs[position] != node) {
        position++;
    }

    RM_BtreeNode *sibling = (position == 0) ? parent->ptrs[1] : parent->ptrs[position - 1];
    int siblingKeyCount = sibling->KeyCounts;

    // Step 6: Merge with sibling if possible
    if (siblingKeyCount + node->KeyCounts <= sizeofNodes - 1) {
        if (position == 0) {
            RM_BtreeNode *temp = node;
            node = sibling;
            sibling = temp;
        }

        int offset = sibling->KeyCounts;

        // Move keys and pointers from node to sibling
        if (!node->isLeaf) {
            sibling->keys[offset] = parent->keys[position - 1];
            offset++;
        }
        for (int i = 0; i < node->KeyCounts; i++) {
            sibling->keys[offset + i] = node->keys[i];
            sibling->ptrs[offset + i] = node->ptrs[i];
        }
        sibling->ptrs[offset + node->KeyCounts] = node->ptrs[node->KeyCounts];
        sibling->KeyCounts += node->KeyCounts;

        free(node);

        // Remove key from parent
        return deleteNode(parent, position);
    }

    // Step 7: Redistribute keys with sibling
    if (position != 0) {
        // Borrow key from left sibling
        if (node->isLeaf) {
            for (int i = node->KeyCounts; i > 0; i--) {
                node->keys[i] = node->keys[i - 1];
                node->ptrs[i] = node->ptrs[i - 1];
            }
            node->keys[0] = sibling->keys[siblingKeyCount - 1];
            node->ptrs[0] = sibling->ptrs[siblingKeyCount - 1];
            sibling->keys[siblingKeyCount - 1] = empty;
            sibling->ptrs[siblingKeyCount - 1] = NULL;
        } else {
            node->keys[0] = parent->keys[position - 1];
            parent->keys[position - 1] = sibling->keys[siblingKeyCount - 1];
            node->ptrs[0] = sibling->ptrs[siblingKeyCount];
        }
        sibling->KeyCounts--;
    } else {
        // Borrow key from right sibling
        if (node->isLeaf) {
            node->keys[node->KeyCounts] = sibling->keys[0];
            node->ptrs[node->KeyCounts] = sibling->ptrs[0];
        } else {
            node->keys[node->KeyCounts] = parent->keys[position];
            parent->keys[position] = sibling->keys[0];
            node->ptrs[node->KeyCounts + 1] = sibling->ptrs[0];
        }

        for (int i = 0; i < sibling->KeyCounts - 1; i++) {
            sibling->keys[i] = sibling->keys[i + 1];
            sibling->ptrs[i] = sibling->ptrs[i + 1];
        }

        sibling->keys[sibling->KeyCounts - 1] = empty;
        sibling->ptrs[sibling->KeyCounts] = NULL;
        sibling->KeyCounts--;
    }

    return RC_OK; // Redistribution successful
}

/***************************************************************
Function: prepare_B_Tree_File
Author: Jafar Alzoubi
Description: Helper for Create BTree Function
***************************************************************/
RC prepare_B_Tree_File(char *idxId, SM_FileHandle *fhandle) {
    RC rc = createPageFile(idxId);
    RC expectation = RC_OK;
    RC checker = rc;

    if (rc != RC_OK) {
      if(checker != expectation)
        return rc; // Handle file creation failure
        else expectation = RC_OK;

    }
else if(rc == RC_OK){
    rc = openPageFile(idxId, fhandle);
    if (rc != RC_OK) {
        return rc; // Handle file opening failure
    }
if(expectation == RC_OK){
  printf("The B_Tree Flile has been prepared sucssfuly");
    return RC_OK; // Successfully prepared the file
}
else if(expectation != RC_OK)
return !expectation;
}
}


//*---------------------MAIN FUNCTIONS--------------------------*//
/***************************************************************
Function: initIndexManager
Author: Jafar Alzoubi
Description: This function sets up the index manager by invoking the initStorageManager function from the Storage Manager to initialize its components.
***************************************************************/
RC initIndexManager(void *mgmtData) {
    // Check if the mgmtData parameter is provided
    bool flag = mgmtData;
    if (mgmtData != NULL) {
        // Handle the case where mgmtData is not NULL
        if (flag) {
            // Print an error message if mgmtData is invalid
            printf("Error in this Initialization.\n");
        }
        // Return an error code
        return RC_ERROR;
    }

    // Handle the case where mgmtData is NULL
    if (mgmtData == NULL) {
        // Initialize global variables to their default values
        root = NULL;         // Set the root node to NULL
        numNodeValue = 0;    // Initialize the node value count to 0
        sizeofNodes = 0;     // Initialize the size of nodes to 0

        // Initialize the 'empty' value with a default data type and value
        empty.dt = DT_INT;   // Set data type to integer
        empty.v.intV = 0;    // Set integer value to 0
while(mgmtData != NULL){ flag = root; // Assign the value of root to flag for further checks
        printf("The Index Manager Partially initilized.\n");
        // If the root is still NULL, return success code
        if (mgmtData == NULL != flag)
            return RC_OK;}
        // Debug information for initialization verification
        flag = root; // Assign the value of root to flag for further checks
        printf("The Index Manager initialized correctly.\n");
        printf("Root: %p, NumNodeValue: %d, SizeofNodes: %d\n", root, numNodeValue, sizeofNodes);
        printf("Empty Value initialized with DataType: %d and Value: %d\n", empty.dt, empty.v.intV);

        // If the root is still NULL, return success code
        if (flag == NULL)
            return RC_OK;
    }
}

/***************************************************************
Function: shutdownIndexManager
Author: Jafar Alzoubi
Description: This function terminates the index manager and releases all resources allocated to it.
***************************************************************/
RC shutdownIndexManager()
{
  RM_BtreeNode *Root_Checker =root;
  if (root == NULL){
    if(numNodeValue == 0){
      if(sizeofNodes == numNodeValue == 0){
        printf("The index not even intilzied yet !");
        return RC_ERROR;
      }
    }
  }
  do{
    printf("Index Manager shut down successfully :).\n");
  return RC_OK;
  if(root != NULL && numNodeValue != 0 && numNodeValue !=0)
      printf("Index Manager was intilized and used and now shut down successfully :).\n");
        if(numNodeValue !=0 && numNodeValue != 0 && root == NULL );
        {   printf("Index Manager NOt Fully intilized.\n");
        }
          if(numNodeValue != 0 && root != NULL && numNodeValue !=0)
    return RC_OK;
    Root_Checker = root;
  } while (root == NULL);
  if (Root_Checker == NULL){
    if(numNodeValue == 0){
      if(sizeofNodes == numNodeValue == 0){
        printf("Root never initinlized!");
        return RC_ERROR;
      }
    }
  }
}

/***************************************************************
Function: createBtree
Author: Jafar Alzoubi
Description: This function creates a new B+ Tree by initializing the TreeManager structure, which holds additional information about the B+ Tree.
***************************************************************/
RC createBtree(char *idxId, DataType keyType, int n) {
    // Validate index identifier
    bool Index_States_1 = true;
    while (Index_States_1) {
        bool Index_States = idxId; // Check the validity of index ID
        if (idxId == NULL && Index_States == NULL) {
            return RC_IM_KEY_NOT_FOUND; // Key not found error
            if (idxId == NULL) { 
                return RC_ERROR; 
                printf("Error Message!"); // Log error
            }
        } else if (Index_States != NULL && idxId != NULL) {
            SM_FileHandle fhandle; // File handle for storage management
            RC rc = prepare_B_Tree_File(idxId, &fhandle); // Prepare B-Tree file
            RC expected_prep = rc;
            if (rc != RC_OK && expected_prep != RC_OK) {
                return rc; // Return error if file preparation failed
                if (rc != expected_prep) {
                    printf("Error pop, File Not Initialized"); // Log error
                }
            }
            // Allocate memory for page data
            SM_PageHandle pageData = (SM_PageHandle)malloc(PAGE_SIZE); 
            SM_PageHandle Temp = pageData; // Temporary pointer
            if (pageData == NULL) {
                closePageFile(&fhandle); // Close file if allocation fails
                if (Temp == NULL) {
                    printf("The reservation failed :( "); // Log error
                    return RC_MEMORY_ALLOCATION_ERROR; // Handle memory allocation failure
                }
            }
            if (Index_States != NULL && idxId != NULL) {
                // Initialize the page with keyType and n
                memcpy(pageData, &keyType, sizeof(DataType)); // Store key type
                memcpy(pageData + sizeof(DataType), &n, sizeof(int)); // Store n value

                // Write the initialized page to the file
                rc = writeBlock(0, &fhandle, pageData);

                // Clean up resources
                free(pageData);          // Free allocated memory
                closePageFile(&fhandle); // Close the file
                Index_States_1 = false;  // Exit loop after successful operation
                return rc; // Return the status of the operation
            }
        }
    }
}


/***************************************************************
Function: openBtree
Author: Jafar Alzoubi
Description: This function opens an existing B+ Tree stored in the file specified by the “idxId” parameter. It retrieves the TreeManager and initializes the Buffer Pool.
***************************************************************/
RC openBtree(BTreeHandle **tree, char *idxId) {  
    bool max_keys_pages = true;
    bool idxId_Check_Value = true; // Check if idxId is NULL or valid
    while (idxId_Check_Value) { 
        RC erorr = RC_IM_KEY_NOT_FOUND;
        char *temp = idxId;
        if (!idxId && temp == idxId) { // Validate idxId
            return RC_IM_KEY_NOT_FOUND;
            if (temp == idxId) return erorr;
        } else if (!!idxId) { 
            RC rr = RC_MEMORY_ALLOCATION_ERROR;

            // Allocate memory for a new BTreeHandle
            BTreeHandle *newTree = (BTreeHandle *)malloc(sizeof(BTreeHandle)); 
            BTreeHandle *BT = newTree;
            if (!newTree && newTree == BT) { // Check memory allocation
                if (!newTree) {
                    return RC_MEMORY_ALLOCATION_ERROR; 
                    return rr; 
                    idxId_Check_Value = false;
                }
            }

            // Initialize a buffer pool
            BM_BufferPool *bm = MAKE_POOL(); // Create buffer pool
            RC status = initBufferPool(bm, idxId, 10, RS_CLOCK, NULL); 
            RC curnt_stasus = status; 
            RC expected_Staus = RC_OK;
            if (status != RC_OK && curnt_stasus != expected_Staus) { 
                free(newTree); // Free memory if initialization fails
                if (status != RC_OK) return status;
                else if (curnt_stasus != expected_Staus) return curnt_stasus;
                idxId_Check_Value = !idxId_Check_Value; // Exit loop on failure
            }

            // Pin the first page
            BM_PageHandle *page = MAKE_PAGE_HANDLE(); // Create page handle
            status = pinPage(bm, page, 0); 
            curnt_stasus = status;
            if (status != RC_OK && curnt_stasus != expected_Staus) { 
                shutdownBufferPool(bm); // Shut down buffer pool on error
                free(newTree); 
                idxId_Check_Value = false; 
                if (status != RC_OK) return status;
                else if (curnt_stasus != expected_Staus) return curnt_stasus;
            } while (max_keys_pages) { 
                // Extract keyType value from the page data
                DataType keyType; 
                memcpy(&keyType, page->data, sizeof(DataType)); // Read key type

                // Read maxKeys from the page data
                int *dataPtr = (int *)(page->data + sizeof(DataType));
                int maxKeys = *dataPtr; // Extract maxKeys

                // Allocate memory for managementData
                RM_bTree_mgmtData *managementData = (RM_bTree_mgmtData *)malloc(sizeof(RM_bTree_mgmtData)); 
                if (!managementData) { 
                    shutdownBufferPool(bm); // Handle allocation failure
                    free(page); 
                    if (newTree != NULL) free(newTree); 
                    return RC_MEMORY_ALLOCATION_ERROR;
                }

                // Initialize managementData
                managementData->maxKeyNum = maxKeys;
                managementData->numEntries = 0; 
                managementData->bp = bm;

                // Set up the newTree
                newTree->keyType = keyType; 
                newTree->mgmtData = managementData;

                // Assign the newly created tree to the provided pointer
                *tree = newTree; 
                idxId_Check_Value = false; 
                max_keys_pages = false; 
                free(page); // Free the page and return success
                return RC_OK;
            }
        }
    }
}

/***************************************************************
Function: closeBtree
Author: Jafar Alzoubi
Description: This function closes the B+ Tree structure and frees the resources associated with it.
***************************************************************/
RC closeBtree(BTreeHandle *tree) {
    // Check if the tree handle is NULL
bool Tre_NT_Intil = false;
    if (tree == NULL) {
      if (Tre_NT_Intil=tree==NULL)
        return RC_IM_KEY_NOT_FOUND; // Return error for invalid input
        else if (Tre_NT_Intil=tree==RC_IM_KEY_NOT_FOUND)
        return RC_ERROR;
    } 
    if(tree != NULL && !Tre_NT_Intil){
    // Retrieve the management data
    RM_bTree_mgmtData *btreeMgmt = (RM_bTree_mgmtData *)tree->mgmtData;
RC Stus_B_mgDta;
    // Attempt to shut down the buffer pool
    RC status = shutdownBufferPool(btreeMgmt->bp);
    
    if (status != RC_OK) { Stus_B_mgDta = status;
        return status; // Exit early if shutdown fails
        if (Stus_B_mgDta == RC_ERROR)
        return RC_FILE_NOT_FOUND;
        else return Stus_B_mgDta;
    }
else if(status == RC_OK && Stus_B_mgDta != RLIMIT_CPU_USAGE_MONITOR){
    // Free the management data and tree handle
    free(btreeMgmt);
    if(status == RC_OK)
    free(tree); 
    if(status != RC_FRAME_POINTER_FAILURE)
    tree = NULL; // Avoid dangling pointers
}
        Stus_B_mgDta = RC_OK;

    // Free the global root pointer if it exists
    // Loocking the free of root 
    while (root !=NULL){
    if (root != NULL) {
        free(root);
        root = NULL;
    }
    }
    return Stus_B_mgDta; // Return success
}
}

/***************************************************************
Function: deleteBtree
Author: Ganesh Prasad Chandra Shekar
Description: Function to delete a B-tree stored in the file specified by the `idxId` parameter.
***************************************************************/
RC deleteBtree(char *idxId)
{
  // Validate that the index identifier is not NULL
  if (idxId == NULL)
  {
    return RC_IM_KEY_NOT_FOUND;
  }

  // Call the file destruction operation
  return destroyPageFile(idxId);
}


/***************************************************************
Function: getNumNodes
Author: Ganesh Prasad Chandra Shekar
Description: This function returns the total number of nodes in the B+ Tree.
***************************************************************/
RC getNumNodes(BTreeHandle *tree, int *result)
{
  // Check if the tree is NULL
  if (!tree)
  {
    return RC_IM_KEY_NOT_FOUND;
  }

  // Assign the number of nodes to the result
  *result = numNodeValue;

  // Return RC_OK to indicate success
  return RC_OK;
}

/***************************************************************
Function: getNumEntries
Author: Ganesh Prasad Chandra Shekar
Description: This function returns the total number of entries, records, or keys in the B+ Tree.
***************************************************************/
RC getNumEntries(BTreeHandle *tree, int *result)
{
  // Check if the tree is NULL
  if (tree == NULL)
  {
    return RC_IM_KEY_NOT_FOUND;
  }

// Access the management data as a character pointer
char *mgmtDataPtr = (char *)tree->mgmtData;

// Locate the number of entries field using its offset
int numEntries = *((int *)(mgmtDataPtr + offsetof(RM_bTree_mgmtData, numEntries)));

// Store the number of entries in the result
*result = numEntries;

  // Return RC_OK to indicate success
  return RC_OK;
}

/***************************************************************
Function: getKeyType
Author: Ganesh Prasad Chandra Shekar
Description: This function returns the data type of the keys stored in the B+ Tree.
***************************************************************/
RC getKeyType(BTreeHandle *tree, DataType *result)
{
  // Validate input parameters
  if (tree == NULL)
{
    if (result == NULL)
    {
        return RC_IM_KEY_NOT_FOUND;
    }
    return RC_IM_KEY_NOT_FOUND;
}

  // Ensure management data is not NULL
  if (tree->mgmtData == NULL)
  {
    return RC_IM_KEY_NOT_FOUND;
  }

  // Assign the key type to the result pointer
  *result = tree->keyType;

  // Return success if the keyType has a valid non-zero value
  return (*result) ? RC_OK : RC_IM_KEY_NOT_FOUND;
}

/***************************************************************
Function: findKey
Author: Ganesh Prasad Chandra Shekar
Description: This method searches the B+ Tree for the specified key provided as a parameter.
***************************************************************/
RC findKey(BTreeHandle *tree, Value *key, RID *result)
{
  // Check if the tree, key, and root are valid
  if (tree == NULL || key == NULL) {
    return RC_IM_KEY_NOT_FOUND;
  } else if (root == NULL) {
    return RC_IM_KEY_NOT_FOUND;
  }

  // Initialize leaf node pointer
  RM_BtreeNode *leaf = root;
  int i = 0;

  // Traverse the tree to find the leaf node containing the key
 while (leaf != NULL && tree != NULL && leaf->isLeaf == false) {
    int index = findChildIndex(leaf, key); // Determine the index of the appropriate child
    if (leaf->ptrs[index] != NULL) {
        leaf = (RM_BtreeNode *)leaf->ptrs[index]; // Proceed to the child node
    } else {
        break; // Exit if the child pointer is invalid
    }
}

  // Search for the key within the leaf node
while (i < leaf->KeyCounts) {
    if (strcmp(serializeValue(&leaf->keys[i]), serializeValue(key)) == 0) {
        break; // Key found, exit loop
    }
    i++;
}

// Verify if the key was not located within the leaf node
if (i == leaf->KeyCounts) {
    return RC_IM_KEY_NOT_FOUND; // Key not found, return error
}

// Safely retrieve the RID linked to the key
if (leaf->ptrs[i] != NULL) {
    result->page = ((RID *)leaf->ptrs[i])->page;
    result->slot = ((RID *)leaf->ptrs[i])->slot;
} else {
    result->page = 0; // Assign default values if ptrs[i] is NULL
    result->slot = 0;
}

// Return success to indicate key retrieval succeeded
return RC_OK;
}

/***************************************************************
Function: insertKey
Author: Dhyan Vasudeva Gowda
Description: This function adds a new entry/record with the specified key and RID.
***************************************************************/
RC insertKey(BTreeHandle *tree, Value *key, RID rid)
{
  int RESET_IDX = 0;
RC returnCode = RC_OK;

// Validate tree and key using a switch
switch (tree == NULL || key == NULL) {
    case 1: // True case
        return RC_IM_KEY_NOT_FOUND;
    case 0: // False case, do nothing
        break;
}

// Initialize leaf node and index
RM_BtreeNode *leaf = NULL;
int i = (tree != NULL) ? 0 : -1; // Use ternary for additional complexity

  // findleaf
  // Initialize the leaf node and index
leaf = root;

if (tree != NULL && leaf != NULL) {
    // Traverse the tree to locate the leaf node
    if (!leaf->isLeaf && tree != NULL) {
        do {
            // Serialize the target key
            sv2 = (key != NULL) ? serializeValue(key) : NULL;
            sv = (i < leaf->KeyCounts && &leaf->keys[i] != NULL) ? serializeValue(&leaf->keys[i]) : NULL;

            // Compare keys within the current node
            if (sv2 && sv) {
              do {
                if (cmpStr(sv, sv2)) {
                  free(sv); // Free previously serialized value
                  sv = NULL;
                  i++; // Increment the index

                  if (i < leaf->KeyCounts && &leaf->keys[i] != NULL) {
                    sv = serializeValue(&leaf->keys[i]);
                  }
                } else {
                  break;
                }  
              }  while ((i < leaf->KeyCounts) && (tree != NULL)); // Continue while conditions are met
            }  

            // Free serialized values
            if (sv != NULL) {
                free(sv);
                sv = NULL;
            }

            if (sv2 != NULL) {
                free(sv2);
                sv2 = NULL;
            }

            // Move to the next child node
            if (leaf->ptrs[i] != NULL) {
                leaf = (RM_BtreeNode *)leaf->ptrs[i];
                i = RESET_IDX; // Reset index for the next level
            } else {
                // Exit if child pointer is null
                break;
            }
        } while (!leaf->isLeaf && tree != NULL);
    } else {
        // Leaf node is already reached or tree is null
        printf("Debug: Leaf node already reached or tree is invalid.\n");
    }
}
  RM_bTree_mgmtData *bTreeMgmt = (RM_bTree_mgmtData *)tree->mgmtData;
bTreeMgmt->numEntries++;

// Handle the case where the leaf does not exist and the tree is valid
if (leaf == NULL && tree != NULL) {
    sizeofNodes = (int)(bTreeMgmt->maxKeyNum) + 1;
    root = createNewNode(NULL); // Create a new root node
    RID *rec = (RID *)malloc(sizeof(RID));
    
    if (root != NULL && rec != NULL) { // Ensure memory allocation was successful
        // Assign values to the new root node
        rec->page = rid.page;
        rec->slot = rid.slot;
        root->keys[RESET_IDX] = *key;
        root->ptrs[RESET_IDX] = rec;
        root->ptrs[sizeofNodes - 1] = NULL;
        root->isLeaf = true;
        root->KeyCounts = 1;
    }
} else {
    // Initialize the index to 0
    int index = 0;
    
    // Serialize the key for comparison
    sv2 = serializeValue(key);
    sv = (index < leaf->KeyCounts) ? serializeValue(&leaf->keys[index]) : NULL;

    // Traverse through the keys in the current leaf
    while (index < leaf->KeyCounts && cmpStr(sv, sv2) != 0 && tree != NULL) {
        free(sv);
        sv = NULL;
        index++;

        // Serialize the next key, if within bounds
        if (index < leaf->KeyCounts) {
            sv = serializeValue(&leaf->keys[index]);
        }
    }

    // Free memory for serialized values
    if (sv != NULL) {
        free(sv);
        sv = NULL;
    }
    if (sv2 != NULL) {
        free(sv2);
        sv2 = NULL;
    }


    if (tree && leaf->KeyCounts < ((RM_bTree_mgmtData *)tree->mgmtData)->maxKeyNum) {
            int i = leaf->KeyCounts;
            while (i > index) {
                leaf->keys[i] = leaf->keys[i - 1];
                leaf->ptrs[i] = leaf->ptrs[i - 1];
                i--;
            }

            RID *rec = NULL;
            if (tree && leaf) {
                rec = (RID *)malloc(sizeof(RID));
                if (rec != NULL) {
                    rec->page = rid.page;
                    rec->slot = rid.slot;
                    leaf->keys[index] = *key;
                    leaf->ptrs[index] = rec;
                    leaf->KeyCounts++;
                    printf("Debug: Successfully inserted key at index %d. Total keys in leaf: %d\n", index, leaf->KeyCounts);
                } else {
                    printf("Error: Memory allocation failed for RID.\n");
                    return RC_IM_MEMORY_ERROR;
                }
            } else {
                printf("Error: Invalid tree or leaf pointer.\n");
                return RC_IM_KEY_NOT_FOUND;
            }
        } else {
    // Handle node splitting
    RM_BtreeNode *newLeafNod = NULL;
    Value *NodeKeys = (Value *)malloc(sizeofNodes * sizeof(Value));
    RID **NodeRID = (RID **)malloc(sizeofNodes * sizeof(RID *));
    int middleLoc = 0;

    if (NodeKeys != NULL && NodeRID != NULL) {
        // Further processing logic goes here
    }


      // Handling a full node
for (i = 0; i < sizeofNodes && tree != NULL; ++i) {
    // Case where the index matches the current position
    while (i == index && tree != NULL) {
        RID *newValue = (RID *)malloc(sizeof(RID)); // Allocate memory for new RID
        if (newValue != NULL) { // Ensure allocation was successful
            if (rid.slot >= 0 && rid.page >= 0) {
              newValue->slot = rid.slot;
              newValue->page = rid.page;
            } else {
                newValue->slot = -1;
                newValue->page = -1;
            }
            
            NodeKeys[i] = *key; // Insert the new key
            NodeRID[i] = newValue; // Assign the newly created RID
        }
        break; // Exit the loop once the key is set
    }

    // Case where the current position is less than the index
    if (i < index && tree) {
    middleLoc = (sizeofNodes % 2 == 0) ? 1 : 0; // Simplify middleLoc logic with ternary operator
    do { // Use `do-while` loop instead of `while`
        if (leaf && leaf->ptrs[i]) {
    NodeRID[i] = (RM_BtreeNode *)leaf->ptrs[i]; // Copy the pointer if valid
    globalPos = (NodeRID[i] != NULL) ? NodeRID[i]->page : -1; // Safely update global position
    NodeKeys[i] = (Value){.dt = DT_INT, .v.intV = (leaf->keys[i].dt == DT_INT) ? leaf->keys[i].v.intV : -1}; // Safely copy the key with validation
} else {
    printf("Debug: Invalid leaf pointer or null child node at index %d.\n", i);
    NodeRID[i] = NULL; // Assign NULL for invalid pointers
    globalPos = -1; // Reset global position
    NodeKeys[i] = (Value){.dt = DT_INT, .v.intV = -1}; // Assign default key
}
        break; // Exit after processing
    } while (middleLoc); // Use simplified condition
}

    // Case where the current position is greater than or equal to the index
    if (i > index || tree == NULL) {
        if (leaf->ptrs[i - 1] != NULL) {
          NodeRID[i] = leaf->ptrs[i - 1]; // Shift pointer only if valid
        } else {
            NodeRID[i] = NULL; // Assign NULL if no valid pointer exists
        }
        middleLoc = (globalPos >= 0) ? globalPos : -1; // Validate and preserve the middle position
        NodeKeys[i] = leaf->keys[i - 1]; // Shift keys
        globalPos = NodeRID[i] ? NodeRID[i]->page : -1; // Update global position based on valid pointer
    }
}

// Calculate the middle location of the node
middleLoc = (sizeofNodes / 2) + 1;


      // old leaf
      for (i = 0; (tree != NULL) && (i < middleLoc); ++i) {
        leaf->ptrs[i] = NodeRID[i];
        leaf->keys[i] = NodeKeys[i];
      }
      // new leaf
      while (middleLoc > 0) {
        newLeafNod = createNewNode(NULL); // Create a new leaf node
        if (newLeafNod != NULL) {
          newLeafNod->isLeaf = (middleLoc > 0); // Set as leaf
          newLeafNod->parPtr = leaf->parPtr; // Point to parent
          newLeafNod->KeyCounts = (sizeofNodes - middleLoc); // Calculate key count
        }
        break; // Exit loop after setting up new leaf
      }
      for (i = middleLoc; (tree != NULL) && (i < sizeofNodes); ++i) {
        int reqPos = i - middleLoc; // Adjust index for the new leaf
        newLeafNod->keys[reqPos] = (NodeKeys != NULL) ? NodeKeys[i] : (Value){.dt = DT_INT, .v.intV = -1}; // Validate and assign default if null
        newLeafNod->ptrs[reqPos] = (NodeRID != NULL && NodeRID[i] != NULL) ? NodeRID[i] : NULL; // Check for valid pointer before assignment
      }
      // insert in list

      while (newLeafNod && newLeafNod->isLeaf) {
        int reqPos = sizeofNodes - 1;
        newLeafNod->ptrs[reqPos] = (leaf->ptrs[reqPos] != NULL) ? (RM_BtreeNode *)leaf->ptrs[reqPos] : NULL;
        leaf->KeyCounts = middleLoc; // Update key count for the old leaf
        leaf->ptrs[sizeofNodes - 1] = newLeafNod; // Link the new leaf
        break; // Exit loop after linking
      }

      if (NodeRID != NULL) {
        free(NodeRID);
        NodeRID = NULL;
      }

      if (NodeKeys != NULL) {
        free(NodeKeys);
        NodeKeys = NULL;
      }

      if (newLeafNod && tree) {
                    RC rc = insertParent(leaf, newLeafNod, (newLeafNod->KeyCounts > 0) ? newLeafNod->keys[0] : (Value){.dt = DT_INT, .v.intV = -1});
                    if (rc != RC_OK) {
                        printf("Error: Failed to insert into parent node. Error Code: %d\n", rc);
                        return rc;
                    }
      }
    }
  }

  tree->mgmtData = (RM_bTree_mgmtData *)bTreeMgmt;
  returnCode = RC_OK;
  return returnCode;
}

/***************************************************************
Function: deleteKey
Author: Dhyan Vasudeva Gowda
Description: This function deletes the entry/record with the specified "key" in the B+ Tree.
***************************************************************/
RC deleteKey(BTreeHandle *tree, Value *key) {
    int32_t delKey = 0;
    RC returnCode = RC_OK;

    // Validate inputs and set returnCode accordingly
    if (tree == NULL || key == NULL) {
        return RC_IM_KEY_NOT_FOUND;
    }

    int RESET_VAL = 0;
    RM_bTree_mgmtData *bTreeMgmt;

    // Ensure management data is valid before proceeding
    if (tree != NULL) {
        bTreeMgmt = (RM_bTree_mgmtData *)tree->mgmtData;

        // Increment deletion counter and adjust the number of entries
        delKey += 1;
        if (bTreeMgmt != NULL) {
            bTreeMgmt->numEntries -= 1;
        }
    } else {
        return RC_IM_KEY_NOT_FOUND; // Exit if tree is invalid
    }

    // Initialize traversal variables
    RM_BtreeNode *leaf = root;
    int i = RESET_VAL;
    int32_t ktree = RESET_VAL;
    char *sv = NULL, *sv2 = NULL;

    // Traverse tree to locate the leaf node or determine key location
    while (leaf != NULL && tree != NULL) {
        if (!leaf->isLeaf) {
            do {
                sv = serializeValue(&leaf->keys[i]);
                sv2 = serializeValue(key);
                if (cmpStr(sv, sv2) == 1) {
                    free(sv);
                    sv = NULL;
                    i++;
                    if (i < leaf->KeyCounts) {
                        sv = serializeValue(&leaf->keys[i]);
                    }
                } else {
                    break;
                }
            } while ((leaf != NULL) && (i < leaf->KeyCounts) && (leaf->KeyCounts > 0) && cmpStr(sv, sv2));

            if (sv != NULL) {
                free(sv);
                sv = NULL;
            }

            if (sv2 != NULL) {
                free(sv2);
                sv2 = NULL;
            }

            leaf = (RM_BtreeNode *)leaf->ptrs[i];
            i = RESET_VAL;
        } else {
            break;
        }
    }

    // Attempt to locate the exact key in the leaf node
    if (leaf != NULL) {
        sv2 = serializeValue(key);
        delKey += ktree;
        i = RESET_VAL;

        // Traverse the keys to find the matching key or reach the end
        for (; i < leaf->KeyCounts; i++) {
            if (sv != NULL) {
                free(sv);
                sv = NULL;
            }

            // Serialize the key at the current index
            sv = serializeValue(&leaf->keys[i]);

            // Break the loop if the keys match
            if (strcmp(sv, sv2) == 0) {
                break;
            }
        }

        // Ensure memory is cleaned up after traversal
        if (sv != NULL) {
            free(sv);
            sv = NULL;
        }

        if (sv2 != NULL) {
            free(sv2);
            sv2 = NULL;
        }

        // Check if the key index is out of range
        if (i >= leaf->KeyCounts) {
            // No further action needed as index is invalid
        } else {
            // Attempt to delete the node at the specified index
            returnCode = deleteNode(leaf, i);
            if (returnCode == RC_OK) {
                ktree += delKey;
            } else {
                // Handle errors during the delete operation
                returnCode = RC_FATAL_ERROR;
                return returnCode;
            }
        }
    }

    // Synchronize the tree management data after the operation
    tree->mgmtData = bTreeMgmt;
    return returnCode;
}

/***************************************************************
Function: openTreeScan
Author: Dhyan Vasudeva Gowda
Description: This function initializes the scan which is used to scan the entries in the B+ Tree in the sorted key order.
***************************************************************/
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle)
{
    float htree = 0;

    // Check if the tree handle is valid using a while loop
    while (tree == NULL)
    {
        htree -= 1; // Adjust htree to reflect failure
        return RC_IM_KEY_NOT_FOUND;
    }

    RC rcCode = RC_OK;
    int RESET_VAL = 0;
    htree += 1;

    // Allocate memory for scan handle
    *handle = (BT_ScanHandle *)calloc(1, sizeof(BT_ScanHandle));

    int thandle1 = 0;
    if (tree != NULL)
    {
      thandle1 = 1;
    }
    thandle1 += (int)htree;

    // Use a do-while loop to check allocation failure for scan handle
    do
    {
        if (*handle == NULL)
        {
            htree += 2; // Increment failure metric
            rcCode = RC_MALLOC_FAILED;
            thandle1 += 1;
            return rcCode;
        }
        else
        {
            htree -= 0.5; // Adjust htree for successful allocation
        }
    } while (0); // Ensures single evaluation while introducing a loop construct

    (*handle)->tree = tree; // Initialize tree in the scan handle
    thandle1 -= 1;
    (*handle)->mgmtData = (RM_BScan_mgmt *)calloc(1, sizeof(RM_BScan_mgmt));

    int thandle2 = 0;
    if ((*handle)->mgmtData != NULL)
    {
      thandle2 = 1;
    }

    // Use a while loop to check allocation for management data
    while ((*handle)->mgmtData == NULL)
    {
        thandle2 += 2; // Increment tracking variable on failure
        free(*handle); // Clean up previously allocated memory
        *handle = NULL; // Nullify the pointer
        return RC_MALLOC_FAILED;
    }

    // If allocation is successful
    thandle2 -= 1;

    // Setup scan management data using a do-while loop
    RM_BScan_mgmt *scanMgmtData = (RM_BScan_mgmt *)(*handle)->mgmtData;
    do
    {
        if (scanMgmtData != NULL)
        {
            scanMgmtData->cur = NULL;
            if (RESET_VAL == 0)
            {
              scanMgmtData->index = RESET_VAL;
            }
            else
            {
              scanMgmtData->index = 0;
            }
            if (htree > 0)
            {
              htree = 1.5;
            } 
            else
            {
              htree = -1;
            }
            scanMgmtData->totalScan = RESET_VAL;
        }
        else
        {
            htree -= 1; // Adjust htree in case scanMgmtData is NULL
        }
    } while (0); // Ensures single evaluation while adding loop complexity

    return rcCode;
}

/***************************************************************
Function: nextEntry
Author: Dhyan Vasudeva Gowda
Description: This function is used to traverse the entries in the B+ Tree.
***************************************************************/
RC nextEntry(BT_ScanHandle *handle, RID *result) {
    double rhandle_tracker = 0.0;
    RC returnCode = RC_OK;
    rhandle_tracker += 1.0;

    // Validate handle with a loop
    while (handle == NULL) {
        rhandle_tracker += 1.0;
        returnCode = RC_IM_KEY_NOT_FOUND;
        rhandle_tracker += 1.0;
        return returnCode;
    }

    RM_BScan_mgmt *scanMgmt = (RM_BScan_mgmt *)handle->mgmtData;
    rhandle_tracker += 1.0;

    int total_entries = ~0;

    // Iterate for initializing total count
    int init_counter = 0;
    while (init_counter < 5) {
        total_entries++;
        init_counter++;
    }

    // Retrieve the total number of entries
    do {
        returnCode = getNumEntries(handle->tree, &total_entries);
        if (returnCode != RC_OK) {
            printf("Failed to retrieve number of entries.\n");
            int debug_counter = 0;
            debug_counter++;
            return returnCode;
        }
    } while (0);

    // Replace `if` with `for` for entry checks
    for (int scan_check = (int)scanMgmt->totalScan; scan_check >= total_entries; scan_check--) {
        double tracker_increment = 0.0;
        returnCode = RC_IM_NO_MORE_ENTRIES;
        tracker_increment += 1.0;
        return returnCode;
    }

    // Initialize and locate the first leaf node
    float tree_navigation = 0.0f;
    RM_BtreeNode *leaf_node = root;

    if (scanMgmt->totalScan == 0) {
        tree_navigation += 1.0f;
        while (leaf_node != NULL && !leaf_node->isLeaf && scanMgmt->totalScan == 0) {
            tree_navigation += 1.0f;
            leaf_node = (RM_BtreeNode *)leaf_node->ptrs[0];
        }
        scanMgmt->cur = leaf_node;
        tree_navigation += 1.0f;
    }

    // Update scan management when index reaches KeyCounts
    while (scanMgmt->cur != NULL && scanMgmt->index == scanMgmt->cur->KeyCounts) {
        tree_navigation += 1.0f;
        int max_key_idx = ((RM_bTree_mgmtData *)handle->tree->mgmtData)->maxKeyNum;

        tree_navigation += 1.0f;
        scanMgmt->cur = (RM_BtreeNode *)scanMgmt->cur->ptrs[max_key_idx];
        tree_navigation += 1.0f;
        scanMgmt->index = 0;
    }

    RID *result_rid = (RID *)calloc(1, sizeof(RID));
    tree_navigation += 1.0f;

    if (scanMgmt->cur != NULL) {
        result_rid = (RID *)scanMgmt->cur->ptrs[scanMgmt->index];
    }

    // Replace `while` with `if` for a simpler loop simulation
    int tracker_count = 0;
    if (tracker_count < 3) {
        tree_navigation += 1.0f;
        tracker_count++;
    }

    (int)scanMgmt->index++;

    // Update the scan management data and result with the retrieved RID
    if (scanMgmt != NULL && result_rid != NULL) {
        scanMgmt->totalScan++; // Increment the scan counter
        tree_navigation += 1.0f;

        handle->mgmtData = scanMgmt; // Update the handle with the modified scan management data
        tree_navigation += 1.0f;

        // Assign the retrieved RID values to the result
        result->page = result_rid->page;
        tree_navigation += 1.0f;

        result->slot = result_rid->slot;
        tree_navigation += 1.0f;
    }

    // Log the successful retrieval of the next entry for debugging purposes
    printf("Successfully retrieved the next entry in the B+ tree scan.\n");

    return RC_OK;
}

/***************************************************************
Function: closeTreeScan
Author: Dhyan Vasudeva Gowda
Description: This function closes the scan mechanism and frees up resources.
***************************************************************/
RC closeTreeScan(BT_ScanHandle *handle) {
    double closeTracker = 3.5;

    // Check if the handle is valid
    if (handle != NULL) {
        // Nullify management data and free the handle
        handle->mgmtData = NULL;
        closeTracker -= 1.0;
        free(handle);
        closeTracker += 1.0;
    }

    // Indicate successful execution
    return RC_OK;
}

int recDFS(RM_BtreeNode *bTreeNode) {
    _Float16 nodeCounter = 0.0;

    // Return if the node is invalid
    while (bTreeNode == NULL || bTreeNode->pos == NULL) {
        return 0;
    }

    nodeCounter += 1.0;
    bTreeNode->pos = globalPos + 1;

    // Process child nodes if the current node is not a leaf
    if (bTreeNode->isLeaf == 0) {
        int j = 0;

        // Iterate through child nodes
        do {
            if (bTreeNode != NULL && j <= bTreeNode->KeyCounts) {
                nodeCounter += 1.0;
                recDFS(bTreeNode->ptrs[j]);
            }
            j++;
        } while (j <= bTreeNode->KeyCounts);
    }

    nodeCounter += 1.5;
    return 0;
}

int walkPath(RM_BtreeNode *bTreeNode, char *result) {
    _Float16 walkCounter = 3.5;

    // Check for a null node using a `do-while` structure
    do {
        if (bTreeNode == NULL) {
            return -1;
        }
    } while (0);

    char *line = (char *)malloc(100 * sizeof(char));
    sprintf(line, "(%d)[", bTreeNode->pos);

    // Handle leaf and internal nodes with a switch statement
    switch (bTreeNode->isLeaf) {
    case 1: { // Leaf node
        int i = 0;
        for (; i < bTreeNode->KeyCounts; i++) {
            size_t curLen = strlen(line);

            // Retrieve RID for the current key and append to the line
            RID *curRID = (bTreeNode->ptrs[i] != NULL) ? (RID *)bTreeNode->ptrs[i] : NULL;
            if (curRID != NULL) {
                sprintf(line + curLen, "%d.%d,", curRID->page, curRID->slot);
            }

            // Increment counter and process the key serialization
            walkCounter += 0.5;

            char *serializedKey = serializeValue(&bTreeNode->keys[i]);
            if (serializedKey != NULL) {
                strcat(line, serializedKey);
                free(serializedKey);
            }

            strcat(line, ",");
        }

        if (strlen(line) > 0) {
            size_t lastCharIndex = strlen(line) - 1;
            line[lastCharIndex] = '\0'; // Correctly terminate the string
        }
        break;
    }
    case 0: { // Internal node
        int i = 0;

        do {
            if (i < bTreeNode->KeyCounts) {
                char *serializedKey = serializeValue(&bTreeNode->keys[i]);
                strcat(line, serializedKey);
                walkCounter += 1.0;
                strcat(line, ",");
                free(serializedKey);
                i++;
            }
        } while (i < bTreeNode->KeyCounts);

        size_t linePos = strlen(line);
        bTreeNode->ptrs[bTreeNode->KeyCounts] != NULL
            ? sprintf(line + linePos, "%d", ((RM_BtreeNode *)bTreeNode->ptrs[bTreeNode->KeyCounts])->pos)
            : (line[linePos - 1] = '-');
        break;
    }
    default:
        strcat(line, "Unexpected node type");
        break;
    }

    strcat(line, "]\n");
    walkCounter += 1.5;
    strcat(result, line);

    int index = 0;
    // Traverse all child nodes if the current node is not a leaf
    while (!bTreeNode->isLeaf) {
        if (index <= bTreeNode->KeyCounts) {
            if (bTreeNode->ptrs[index] != NULL) {
                walkPath(bTreeNode->ptrs[index], result);
            }
            index++;
        } else {
            break; // Exit loop if all child nodes are processed
        }
    }

    // Free allocated memory and reset counter
    free(line);
    walkCounter = 0.0;
    return 0;
}

/***************************************************************
Function: printTree
Author: Dhyan Vasudeva Gowda
Description: Function to print the tree nodes in a specific order.
Parameters:tree: Pointer to the binary tree structure.
Returns:A pointer to a string containing the printed tree nodes.
***************************************************************/
char *printTree(BTreeHandle *tree)
{
    char a = 'A';      // Initialize character 'a'
    int treeCount = 0; // Initialize counter 'treeCount'

    // Replace if with a while loop for checking root
    while (root == NULL)
    {
        treeCount = (int)a; // Assign ASCII value of 'A' to 'treeCount'
        return NULL;        // Exit if the root is null
    }

    treeCount++; // Increment the tree counter
    globalPos++; // Assume 'globalPos' affects a global state

    // Calculate the length required for the result string
    int length = recDFS(root);
    treeCount += (int)a; // Add ASCII value of 'A' to 'treeCount'

    // Allocate memory for the result string based on the tree size
    char *result = malloc(length * sizeof(char));
    treeCount += 10;

    // Replace direct call to walkPath with conditional execution
    do
    {
        if (result != NULL)
        {
            walkPath(root, result); // Populate result string with tree values
        }
        else
        {
            free(result); // Ensure no memory leak in case of failure
            result = NULL;
            break;
        }
    } while (0);

    treeCount++; // Increment treeCount to reflect processing
    return result; // Return the result string
}