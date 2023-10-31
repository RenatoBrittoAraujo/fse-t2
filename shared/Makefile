dev:
	@echo "Compiling shared dev"
	make -f Makefile_dev

dev_test:
	@echo "Running shared dev test"
	make -f Makefile_dev_test

run_prod:
	@echo "Running shared dev test"
	make -f Makefile_prod

prod:
	@echo "shared prod!"
	make -f Makefile_prod

clean:
	@make -f Makefile_dev clean >/dev/null
	@make -f Makefile_prod clean >/dev/null
	@make -f Makefile_dev_test clean >/dev/null