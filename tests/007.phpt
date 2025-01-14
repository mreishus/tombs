--TEST--
Check if opcache recompile leads to duplicate tombstones
--EXTENSIONS--
opcache
--INI--
opcache.enable=1
opcache.enable_cli=1
tombs.socket=tcp://127.0.0.1:8010
--FILE--
<?php
// If this test isn't running, try using "make test_opcache".
include "zend_tombs.inc";
include "007_helper.php";

function test() {}
function test2() {}

test2();
test4();
$a = opcache_invalidate('007_helper.php', true);
opcache_compile_file('007_helper.php');
test4();

zend_tombs_display("127.0.0.1", 8010);
/*
Bugged output (4 entries):
{"location": {"file": "%s007.php", "start": 6, "end": 6}, "function": "test"}
{"location": {"file": "%s007_helper.php", "start": 3, "end": 3}, "function": "test3"}
{"location": {"file": "%s007_helper.php", "start": 3, "end": 3}, "function": "test3"}
{"location": {"file": "%s007_helper.php", "start": 6, "end": 6}, "function": "test4"}
Correct output below (2 entries):
 */
?>
--EXPECTF--
{"location": {"file": "%s007.php", "start": 6, "end": 6}, "function": "test"}
{"location": {"file": "%s007_helper.php", "start": 3, "end": 3}, "function": "test3"}
