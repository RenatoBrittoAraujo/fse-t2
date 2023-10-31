dev:
	@make -f Makefile_dev

prod:
	@make -f Makefile_prod

run_dev:
	@make -f Makefile_dev run

run_prod:
	@make -f Makefile_prod run

clean:
	@make -f Makefile_dev clean
	@make -f Makefile_prod clean

all_dev: 
	make dev
	make run_dev

all_prod: 
	make prod
	make run_prod