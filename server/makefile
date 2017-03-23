PROJECT_DIR := .
-include $(PROJECT_DIR)/makefile.shared

COMMON := commonlib	\
			frame/push_common \

MODULE := frame/client_test \
			frame/center \
			frame/proxy	\
			frame/connector \
			frame/dbworker	\
			frame/phpproxy \
			frame/loadsvr 	\

all: third proto common module

clean: clean_proto clean_common clean_module

third:
	@cd opensource/ && sh opensource.sh && cd -

common:
	@for dir in $(COMMON); do \
		make -C $$dir -j8; \
	done;

module:
	@for dir in $(MODULE); do \
		make -C $$dir -j8; \
	done;

proto:
	@cd ../com/proto/ && sh Build.sh && cd -
	@make -C frame/proto

clean_proto:
	@make clean -C frame/proto

clean_common:
	@for dir in $(COMMON); do \
		make clean -C $$dir; \
	done;

clean_module:
	@for dir in $(MODULE); do \
		make clean -C $$dir; \
	done;

install:
	@for dir in $(MODULE); do \
		make install -C $$dir;	\
	done;

.PHONY: all clean third common module proto clean_proto clean_common clean_module install


