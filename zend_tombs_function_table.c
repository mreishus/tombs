#include "zend_tombs_function_table.h"
#include "zend_tombs.h"

zend_tombs_function_table_t *zend_tombs_function_table_startup(zend_long slots) {
    zend_long function_table_size = slots * sizeof(zend_tombs_function_entry_t);
    zend_tombs_function_table_t *table = zend_tombs_map(sizeof(zend_tombs_function_table_t) + function_table_size);
    if (!table) {
        zend_error(E_WARNING, "[TOMBS] Failed to allocate shared memory for function table");
        return NULL;
    }

    memset(table, 0, sizeof(zend_tombs_function_table_t) + function_table_size);
    table->size = slots;
    table->used = 0;

    // Initialize marker_index to -1 for all entries
    for (zend_long i = 0; i < slots; i++) {
        table->entries[i].marker_index = -1;
    }

    return table;
}

uint64_t zend_tombs_hash_key(const zend_op_array *ops) {
    uint64_t hash = 5381;

    if (ops->filename) {
        hash = (hash * 33) + zend_string_hash_val(ops->filename);
    }

    if (ops->function_name) {
        hash = (hash * 33) + zend_string_hash_val(ops->function_name);
    } else {
        // Use line numbers as a fallback for anonymous functions
        hash = (hash * 33) + (uint64_t)ops->line_start;
        hash = (hash * 33) + (uint64_t)ops->line_end;
    }

    if (ops->scope && ops->scope->name) {
        hash = (hash * 33) + zend_string_hash_val(ops->scope->name);
    }

    return hash;
}

zend_tombs_function_entry_t *zend_tombs_function_find_or_insert(uint64_t hash, zend_tombs_function_table_t *table) {
    zend_long slot = hash % table->size;
    zend_long starting_slot = slot;

    while (1) {
        zend_tombs_function_entry_t *entry = &table->entries[slot];
        zend_bool expected_used = 0;
        if (__atomic_compare_exchange_n(&entry->used, &expected_used, 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
            // Found an empty slot, insert the new entry
            entry->hash = hash;
            entry->used = 1;
            __atomic_add_fetch(&table->used, 1, __ATOMIC_RELAXED);
            return entry;
        }
        if (entry->hash == hash) {
            return entry;
        }
        slot = (slot + 1) % table->size;
        if (slot == starting_slot) {
            // Hash table is full
            return NULL;
        }
    }

    return &table->entries[slot];
}
