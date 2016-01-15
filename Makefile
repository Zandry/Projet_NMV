.PHONY: all 
#.SECONDARY:

ifneq ($(KERNELRELEASE),)

obj-m := projet_nmv_async.o

else
	
  KERNELDIR ?= /lib/modules/$(shell uname -r)/build
  PWD := $(shell pwd)

all :
	$(info obj-m : $(obj-m))
	make -C $(KERNELDIR) M=$(PWD) modules
	gcc user_request_async.c -o user_request
	gcc proc_test.c -o proc_test

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm user_request
	rm proc_test

endif
