/*
  +----------------------------------------------------------------------+
  | tombs                                                                |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2019                                       |
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

#ifndef ZEND_TOMBS_GRAVEYARD_H
# define ZEND_TOMBS_GRAVEYARD_H

typedef struct _zend_tomb_t zend_tomb_t;

typedef struct {
    zend_tomb_t *tombs;
    struct {
        zend_ulong tombs;
        zend_ulong populated;
    } stat;
    zend_ulong size;
} zend_tombs_graveyard_t;

zend_tombs_graveyard_t*      zend_tombs_graveyard_create(zend_ulong tombs);
void                         zend_tombs_graveyard_insert(zend_tombs_graveyard_t *graveyard, zend_ulong index, zend_op_array *ops);
void                         zend_tombs_graveyard_delete(zend_tombs_graveyard_t *graveyard, zend_ulong index);
void                         zend_tombs_graveyard_dump(zend_tombs_graveyard_t *graveyard, int fd);
void                         zend_tombs_graveyard_destroy(zend_tombs_graveyard_t *graveyard);

#endif	/* ZEND_TOMBS_GRAVEYARD_H */
