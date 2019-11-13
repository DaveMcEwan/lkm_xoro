
LKM_NAME := xoro

obj-m += $(LKM_NAME).o
$(LKM_NAME)-objs := lkm_$(LKM_NAME).o xoroshiro128plus.o

build: buildtest
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean: cleantest
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

buildtest:
	$(CC) test_$(LKM_NAME).c -o test_$(LKM_NAME)

cleantest:
	-rm test_$(LKM_NAME)
