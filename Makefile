.PHONY: all 
#.SECONDARY:

ifneq ($(KERNELRELEASE),)

obj-m := projetNMV.o

else
	
  KERNELDIR ?= /lib/modules/$(shell uname -r)/build
  PWD := $(shell pwd)

all :
	$(info obj-m : $(obj-m))
	make -C $(KERNELDIR) M=$(PWD) modules
	gcc user_request.c -o user_request
	gcc proc_test.c -o proc_test

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

endif
