/*
  +----------------------------------------------------------------------+
  | tombs                                                                |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2020                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: krakjoe                                                      |
  +----------------------------------------------------------------------+
 */

#ifndef ZEND_TOMBS
# define ZEND_TOMBS

#define ZEND_TOMBS_EXTNAME   "Tombs"
#define ZEND_TOMBS_VERSION   "0.0.3-dev"
#define ZEND_TOMBS_AUTHOR    "krakjoe"
#define ZEND_TOMBS_URL       "https://github.com/krakjoe/tombs"
#define ZEND_TOMBS_COPYRIGHT "Copyright (c) 2020"

#if defined(__GNUC__) && __GNUC__ >= 4
# define ZEND_TOMBS_EXTENSION_API __attribute__ ((visibility("default")))
#else
# define ZEND_TOMBS_EXTENSION_API
#endif

#include "php.h"
#include "zend_observer.h"
#include "zend_tombs.h"
#include "zend_tombs_strings.h"
#include "zend_tombs_graveyard.h"
#include "zend_tombs_ini.h"
#include "zend_tombs_io.h"
#include "zend_tombs_markers.h"
#include "zend_tombs_function_table.h"

static zend_tombs_markers_t   *zend_tombs_markers;
static zend_tombs_graveyard_t *zend_tombs_graveyard;
static int                     zend_tombs_resource = -1;
static pid_t                   zend_tombs_started = 0;
static zend_tombs_function_table_t *zend_tombs_function_table;

static int  zend_tombs_startup(zend_extension*);
static void zend_tombs_shutdown(zend_extension *);
static void zend_tombs_activate(void);
static void zend_tombs_setup(zend_op_array*);
static zend_observer_fcall_handlers zend_tombs_observer_init( zend_execute_data *execute_data );
static void zend_tombs_observer_begin(zend_execute_data *execute_data);

ZEND_TOMBS_EXTENSION_API zend_extension_version_info extension_version_info = {
    ZEND_EXTENSION_API_NO,
    ZEND_EXTENSION_BUILD_ID
};

ZEND_TOMBS_EXTENSION_API zend_extension zend_extension_entry = {
    ZEND_TOMBS_EXTNAME,
    ZEND_TOMBS_VERSION,
    ZEND_TOMBS_AUTHOR,
    ZEND_TOMBS_URL,
    ZEND_TOMBS_COPYRIGHT,
    zend_tombs_startup,
    zend_tombs_shutdown,
    zend_tombs_activate,
    NULL,
    NULL,
    zend_tombs_setup,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    STANDARD_ZEND_EXTENSION_PROPERTIES
};

static int zend_tombs_startup(zend_extension *ze) {
    zend_tombs_ini_startup();

    if (!zend_tombs_ini_socket && !zend_tombs_ini_dump) {
        zend_error(E_WARNING,
            "[TOMBS] socket and dump are both disabled by configuration, shutting down."
            "may be misconfigured");
        zend_tombs_ini_shutdown();

        return SUCCESS;
    }

    if (!zend_tombs_strings_startup(zend_tombs_ini_strings)) {
        zend_error(E_WARNING, "[TOMBS] failed to allocate strings, shutting down.");
        zend_tombs_ini_shutdown();

        return SUCCESS;
    }

    if (!(zend_tombs_markers = zend_tombs_markers_startup(zend_tombs_ini_slots))) {
        zend_error(E_WARNING, "[TOMBS] failed to allocate markers, shutting down.");
        zend_tombs_strings_shutdown();
        zend_tombs_ini_shutdown();

        return SUCCESS;
    }

    if (!(zend_tombs_graveyard = zend_tombs_graveyard_startup(zend_tombs_ini_slots))) {
        zend_error(E_WARNING, "[TOMBS] failed to allocate graveyard, shutting down.");
        zend_tombs_markers_shutdown(zend_tombs_markers);
        zend_tombs_strings_shutdown();
        zend_tombs_ini_shutdown();

        return SUCCESS;
    }

    if (!zend_tombs_io_startup(zend_tombs_ini_socket, zend_tombs_graveyard)) {
        zend_error(E_WARNING, "[TOMBS] failed to start io, shutting down.");
        zend_tombs_graveyard_shutdown(zend_tombs_graveyard);
        zend_tombs_markers_shutdown(zend_tombs_markers);
        zend_tombs_strings_shutdown();
        zend_tombs_ini_shutdown();

        return SUCCESS;
    }

    if (!(zend_tombs_function_table = zend_tombs_function_table_startup(zend_tombs_ini_slots))) {
        zend_error(E_WARNING, "[TOMBS] failed to allocate function table, shutting down.");
        zend_tombs_io_shutdown();
        zend_tombs_graveyard_shutdown(zend_tombs_graveyard);
        zend_tombs_markers_shutdown(zend_tombs_markers);
        zend_tombs_strings_shutdown();
        zend_tombs_ini_shutdown();

        return SUCCESS;
    }

    zend_tombs_started  = getpid();
#if ZEND_EXTENSION_API_NO > 400000000
    zend_tombs_resource = zend_get_resource_handle(ZEND_TOMBS_EXTNAME);
#else
    zend_tombs_resource = zend_get_resource_handle(ze);
#endif

    ze->handle = 0;

    zend_observer_fcall_register( zend_tombs_observer_init );

    return SUCCESS;
}

static void zend_tombs_shutdown(zend_extension *ze) {
    if (!zend_tombs_started) {
        return;
    }

    if (getpid() != zend_tombs_started) {
        return;
    }

    if (zend_tombs_ini_dump > 0) {
        zend_tombs_graveyard_dump(zend_tombs_graveyard, zend_tombs_ini_dump);
    }

    zend_tombs_function_table_shutdown(zend_tombs_function_table);
    zend_tombs_io_shutdown();
    zend_tombs_graveyard_shutdown(zend_tombs_graveyard);
    zend_tombs_markers_shutdown(zend_tombs_markers);
    zend_tombs_strings_shutdown();
    zend_tombs_ini_shutdown();

    zend_tombs_started = 0;
}

static void zend_tombs_activate(void) {
#if defined(ZTS) && defined(COMPILE_DL_TOMBS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    if (!zend_tombs_started) {
        return;
    }

    if (INI_INT("opcache.optimization_level")) {
        zend_string *optimizer = zend_string_init(
	        ZEND_STRL("opcache.optimization_level"), 1);
        zend_long level = INI_INT("opcache.optimization_level");
        zend_string *value;

        /* disable pass 1 (pre-evaluate constant function calls) */
        level &= ~(1<<0);

        /* disable pass 4 (optimize function calls) */
        level &= ~(1<<3);

        value = zend_strpprintf(0, "0x%08X", (unsigned int) level);

        zend_alter_ini_entry(optimizer, value,
	        ZEND_INI_SYSTEM, ZEND_INI_STAGE_ACTIVATE);

        zend_string_release(optimizer);
        zend_string_release(value);
    }
}

static void zend_tombs_setup(zend_op_array *ops) {
    zend_bool **slot,
               *nil = NULL,
              **marker = NULL;

    if (UNEXPECTED(!zend_tombs_started)) {
        return;
    }

    if (UNEXPECTED(NULL == ops->function_name)) {
        return;
    }

    if (UNEXPECTED(NULL != zend_tombs_ini_namespace)) {
        if (UNEXPECTED((NULL == ops->scope) || (SUCCESS != zend_binary_strncasecmp(
                ZSTR_VAL(ops->scope->name), ZSTR_LEN(ops->scope->name),
                ZSTR_VAL(zend_tombs_ini_namespace), ZSTR_LEN(zend_tombs_ini_namespace),
                ZSTR_LEN(zend_tombs_ini_namespace))))) {
            return;
        }
    }

    uint64_t hash = zend_tombs_hash_key(ops);
    zend_tombs_function_entry_t *entry = zend_tombs_function_find_or_insert(hash, zend_tombs_function_table);

    if (UNEXPECTED(NULL == entry)) {
        // no function space left
        return;
    }

    if (entry->marker_index != -1) {
        // Reuse existing marker and tomb
        zend_long marker_index = __atomic_load_n(&entry->marker_index, __ATOMIC_SEQ_CST);
        zend_bool **slot = (zend_bool**)(&ops->reserved[zend_tombs_resource]);
        zend_bool **marker = zend_tombs_markers_get(zend_tombs_markers, marker_index);
        __atomic_compare_exchange_n(slot, &nil, marker, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    } else {
        // Try to allocate new marker and tomb
        zend_long expected_marker_index = -1;

        if (__atomic_compare_exchange_n(&entry->marker_index, &expected_marker_index, -2, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            // We won the race to allocate a new marker and tomb (and set the marker_index to -2 to indicate we're working on it)
            zend_bool **slot = (zend_bool**)(&ops->reserved[zend_tombs_resource]);
            zend_bool **marker = zend_tombs_markers_create(zend_tombs_markers);

            if (UNEXPECTED(NULL == marker)) {
                // no marker space left
                // also reset the marker_index to -1
                __atomic_store_n(&entry->marker_index, -1, __ATOMIC_SEQ_CST);
                return;
            }

            zend_long marker_index = zend_tombs_markers_index(zend_tombs_markers, (zend_bool*)marker);
            __atomic_store_n(&entry->marker_index, marker_index, __ATOMIC_SEQ_CST);

            if (__atomic_compare_exchange_n(slot, &nil, marker, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                zend_tombs_graveyard_populate(zend_tombs_graveyard, marker_index, ops);
            }

        } else {
            // Another thread is already allocating a new marker and tomb.
            // Wait for it to finish.
            while ( __atomic_load_n(&entry->marker_index, __ATOMIC_SEQ_CST) == -2 ) {
                __asm__ __volatile__("pause" ::: "memory");
            }

            // Update the op_array with the newly allocated marker, if needed (not sure if we had the same op_array or not)
            zend_long marker_index = __atomic_load_n(&entry->marker_index, __ATOMIC_SEQ_CST);
            zend_bool **slot = (zend_bool**)(&ops->reserved[zend_tombs_resource]);
            zend_bool **marker = zend_tombs_markers_get(zend_tombs_markers, marker_index);
            __atomic_compare_exchange_n(slot, &nil, marker, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        }
    }
}

// Called the first time a function is called.
// We set up observers if it is a user function.
static zend_observer_fcall_handlers zend_tombs_observer_init( zend_execute_data *execute_data ) {
    zend_function *func = EX(func);
    if ( func->type == ZEND_INTERNAL_FUNCTION ) {
        // We did not set up markers for PHP core functions, so we can skip them
        return (zend_observer_fcall_handlers){NULL, NULL};
    }

    // It's a user function, so we set up observers
    return (zend_observer_fcall_handlers){zend_tombs_observer_begin, NULL};
}

// Called when a function is about to be executed, if zend_tombs_observer_init attached us to it.
static zend_always_inline void zend_tombs_observer_begin(zend_execute_data *execute_data)
{
    zend_op_array *ops = (zend_op_array*) EX(func);
    zend_bool *marker   = NULL,
              _unmarked = 0,
              _marked   = 1;

    if (UNEXPECTED(NULL == ops->function_name)) {
        return;
    }

    marker = __atomic_load_n(&ops->reserved[zend_tombs_resource], __ATOMIC_SEQ_CST);
    zend_long marker_index = zend_tombs_markers_index(zend_tombs_markers, marker);


    if (UNEXPECTED(NULL == marker)) {
        return;
    }

    if (__atomic_compare_exchange(
        marker,
        &_unmarked,
        &_marked,
        0,
        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    )) {
        zend_tombs_graveyard_vacate(
            zend_tombs_graveyard,
            zend_tombs_markers_index(
                zend_tombs_markers, marker));
    }
}

#if defined(ZTS) && defined(COMPILE_DL_TOMBS)
    ZEND_TSRMLS_CACHE_DEFINE();
#endif

#endif /* ZEND_TOMBS */
