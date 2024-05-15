#ifndef ZEND_TOMBS_FUNCTION_TABLE_H
#define ZEND_TOMBS_FUNCTION_TABLE_H

#include "php.h"
#include "zend.h"
#include "zend_string.h"
#include "zend_types.h"
#include "zend_hash.h"

typedef struct _zend_tombs_function_entry_t {
    uint64_t hash;
    zend_long marker_index;
    int8_t used;
} zend_tombs_function_entry_t;

typedef struct _zend_tombs_function_table_t {
    zend_long size;
    zend_long used;
    zend_tombs_function_entry_t entries[0];
} zend_tombs_function_table_t;


uint64_t zend_tombs_hash_key(const zend_op_array *ops);
zend_tombs_function_table_t *zend_tombs_function_table_startup(zend_long slots);
zend_tombs_function_entry_t *zend_tombs_function_find_or_insert(uint64_t hash, zend_tombs_function_table_t *thetab);
void zend_tombs_function_table_shutdown(zend_tombs_function_table_t *table);

#endif /* ZEND_TOMBS_FUNCTION_TABLE_H */
