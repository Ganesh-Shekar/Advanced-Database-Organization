#include <string.h>
#include <stdlib.h>
#include "btree_mgr.h"
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

extern bool cmpStr(const char *c1, const char *c2)
{
  return strcmp(c1, c2) <= 0;
}


extern void release(RM_BtreeNode* tempNode, void* tempKeys) {
    free(tempNode);
    tempNode = NULL;
    free(tempKeys);
    tempKeys = NULL;
}