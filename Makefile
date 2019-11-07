UNAME = $(shell uname)

all: t0.test

% :
ifeq ($(UNAME),Darwin)
	@echo "*****************************"
	@echo "*** Building docker image ***"
	@echo "*****************************"
	docker build -t cs439 .
	@echo ""
	@echo "*************************"
	@echo "*** Running in Docker ***"
	@echo "*************************"
	@echo ""
	docker run -v `pwd`:/work -ti cs439 make -$(MAKEFLAGS) -f Makefile.impl $@
else
	make -f Makefile.impl $@
endif
