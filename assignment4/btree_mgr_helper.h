#ifndef BTREE_MGR_HELPER_H
#define BTREE_MGR_HELPER_H

#include "dberror.h"
#include "tables.h"
#include "buffer_mgr.h"
#include "btree_mgr.h"

bool cmpStr(const char *c1, const char *c2);
void release(RM_BtreeNode* tempNode, void* tempKeys);
#endif

