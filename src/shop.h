#ifndef SHOP_H
#define SHOP_H

#include <stdbool.h>

typedef enum ShopItemId {
    ITEM_WILDCARD = 0,   
    ITEM_MEMORY_FLUSH,   
    ITEM_BUFFER_RELOAD,  
    ITEM_COMPILER_PATCH, 
    ITEM_NULL_POINTER,   
    ITEM_REDUNDANT_COLOR,
    ITEM_BANKER_CHIP,  
    ITEM_TRY_CATCH,  
    ITEM_PREFETCH,      
    ITEM_GARBAGE_COLLECTOR, 
    ITEM_SEGFAULT_HANDLER, 
    ITEM_OVERCLOCK,        
    ITEM_STACK_TRACE,    
    ITEM_ROLLBACK,       
    ITEM_MULTITHREAD,   
    ITEM_COUNT
} ShopItemId;

typedef struct ShopItemInfo {
    const char *name;
    const char *description;
    int  cost;
    bool isModule;
} ShopItemInfo;

const ShopItemInfo *shop_getItemInfo(ShopItemId id);

#endif
