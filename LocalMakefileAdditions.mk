# Target: test_opcache
# Description: Runs the `test` target with OPcache enabled, setting necessary PHP ini parameters.
test_opcache:
	@echo "Running tests with OPcache enabled"
	EXT_DIR=$$(php -r "echo ini_get('extension_dir');") ;\
	TEST_PHP_ARGS="-d zend_extension=$$EXT_DIR/opcache.so -d opcache.enable=1 -d opcache.enable_cli=1 -d open_basedir= -d output_buffering=0 -d memory_limit=-1" ;\
	export TEST_PHP_ARGS ;\
	echo "Setting TEST_PHP_ARGS to: $$TEST_PHP_ARGS" ;\
	$(MAKE) test

