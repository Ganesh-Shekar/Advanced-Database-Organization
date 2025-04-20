## CS 525- ASSIGNMENT 4

---

## Team Members

| #   | Name                         | CWID      | Email                       | Contribution |
| --- | ---------------------------- | --------- | --------------------------- | ------------ |
| 1   | Jafar Alzoubi                | A20501723 | jalzoubi@hawk.iit.edu       | 33.33%       |
| 2   | Dhyan Vasudeva Gowda         | A20592874 | dgowda2@hawk.iit.edu        | 33.33%       |
| 3   | Ganesh Prasad Chandra Shekar | A20557831 | gchandrashekar@hawk.iit.edu | 33.33%       |

---

## Overview

This assginment implements a **B+ - Tree**. It provides various functionalities to create, manage, and manipulate B+ Trees for efficient data indexing and retrieval.

---

## Setup and Execution

### How to Run

1. Compile the program:
   ```
   make
   ```
2. Execute the test file:
   ```
   ./test_assign4
   ```
3. Clean up after execution:
   ```
   make clean
   ```

### Sample Output

You can redirect the program's output to a text file:

```bash
./test_assign4 > result.txt
```

Running `make clean` also removes this output file.

---

## Key Functions and Description:

---

### createNewNode

Allocates and initializes a new B-tree node.

**Function:**

```c
RM_BtreeNode *createNewNode();
```

**Purpose:** Creates a new B-tree node and initializes its components.

**Details:**

- Allocates memory for pointers, keys, and other fields.
- Initializes fields like `ptrs`, `keys`, `isLeaf`, etc.
- Returns a pointer to the newly created node or `RC_MEM_ALLOC_FAILED` on failure.

---

### insertParent

Inserts a new node into the B+ tree and updates the parent.

**Function:**

```c
RC insertParent(RM_BtreeNode *left, RM_BtreeNode *right, Value key);
```

**Purpose:** Manages key and pointer insertion into parent nodes, including splitting if necessary.

**Details:**

- Creates a new root node if the parent is `NULL`.
- Handles key and pointer rearrangement for parent updates.
- Splits nodes when full and recursively inserts into higher levels.

---

### deleteNode

Deletes a specified entry or key in the B+ tree.

**Function:**

```c
RC deleteNode(RM_BtreeNode *bTreeNode, int index);
```

**Purpose:** Removes keys and redistributes or merges nodes as required.

**Details:**

- Reorders or merges sibling nodes if underflow occurs.
- Supports both leaf and non-leaf node operations.

---

### initIndexManager

Initializes the B+ tree index manager.

**Function:**

```c
RC initIndexManager(void *mgmtData);
```

**Purpose:** Sets up the global variables and initial structures for managing the B+ tree.

---

### shutdownIndexManager

Shuts down the index manager.

**Function:**

```c
RC shutdownIndexManager();
```

**Purpose:** Cleans up resources and performs necessary shutdown operations.

---

### createBtree

Creates a new B+ tree.

**Function:**

```c
RC createBtree(char *idxId, DataType keyType, int n);
```

**Purpose:** Initializes metadata, creates the first page, and prepares the B+ tree for use.

---

### openBtree

Opens an existing B+ tree.

**Function:**

```c
RC openBtree(BTreeHandle **tree, char *idxId);
```

**Purpose:** Loads metadata and initializes structures for the given B+ tree.

---

### closeBtree

Closes the B+ tree and releases resources.

**Function:**

```c
RC closeBtree(BTreeHandle *tree);
```

**Purpose:** Safely shuts down the B+ tree instance.

---

### getNumEntries

Retrieves the number of entries in the B+ tree.

**Function:**

```c
RC getNumEntries(BTreeHandle *tree, int *result);
```

**Purpose:** Returns the total number of keys/entries stored in the B+ tree.

---

### getKeyType

A function designed to return the data type of the keys stored in the B+ tree linked to the specified tree handle.

**Function:**

```c
RC getKeyType(BTreeHandle *tree, DataType *result);
```

**Purpose:** This function provides the data type of the keys stored within the B+ tree.

---

### findKey

Searches for a key in the B+ tree.

**Function:**

```c
RC findKey(BTreeHandle *tree, Value *key, RID *result);
```

**Purpose:** Finds the specified key and retrieves its associated record ID (RID).

---

### insertKey

Inserts a new key into the B+ tree.

**Function:**

```c
RC insertKey(BTreeHandle *tree, Value *key, RID rid);
```

**Purpose:** Adds a key-value pair into the B+ tree and handles node splitting if needed.

---

### deleteKey

Deletes a key from the B+ tree.

**Function:**

```c
RC deleteKey(BTreeHandle *tree, Value *key);
```

**Purpose:** Removes a specified key and reorganizes the tree to maintain structure.

---

### openTreeScan

A function to initialize a scan for traversing the entries in the B+ tree in sorted key order.

**Function:**

```c
RC openTreeScan(BTreeHandle *tree, Value *key);
```

**Purpose:** This function initializes the scanning process for iterating through the entries in the B+ tree in sorted key order.

---

### nextEntry

A function designed to traverse and iterate through the entries stored in the B+ tree.

**Function:**

```c
RC nextEntry(BT_ScanHandle *handle, RID *result);
```

**Purpose:** This function facilitates the traversal of entries within the B+ tree.

---

### closeTreeScan

A function to terminate the scan mechanism and release allocated resources.

**Function:**

```c
RC closeTreeScan(BT_ScanHandle *handle);
```

**Purpose:** This function terminates the scan mechanism and releases the allocated resources.

---

### printTree

Prints the entire structure of the B+ tree.

**Function:**

```c
char *printTree(BTreeHandle *tree);
```

**Purpose:** Constructs and returns a string representation of the B+ tree.
