libiio_source := backend.c buffer.c channel.c context.c device.c dns_sd.c dns_sd_avahi.c iiod-client.c local.c lock.c \
network.c network-unix.c scan.c sort.c usb.c utilities.c xml.cpp \
deps/libini/libini.c \
deps/libusb/core.c deps/libusb/descriptor.c deps/libusb/hotplug.c deps/libusb/io.c deps/libusb/strerror.c deps/libusb/sync.c \
deps/libusb/os/events_posix.c deps/libusb/os/linux_netlink.c deps/libusb/os/linux_usbfs.c \
deps/libusb/os/threads_posix.c

libad9361_source := ad9361_baseband_auto_rate.c ad9361_calculate_rf_clock_chain.c ad9361_design_taps.c \
ad9361_fmcomms5_phase_sync.c ad9361_multichip_sync.c \
filterdesigner/internal_design_filter_cg.c filterdesigner/internal_design_filter_cg_emxutil.c filterdesigner/rt_nonfinite.c \
filterdesigner/rtGetInf.c filterdesigner/rtGetNaN.c

utils_source := gen_code.c iio_common.c

iiod_source := dns-sd.c lexer.c ops.c parser.c serial.c thread-pool.c usbd.c

libiio_objects := $(libiio_source)
libiio_objects := $(libiio_objects:.cpp=.o)
libiio_objects := $(libiio_objects:.c=.o)
libiio_objects_tmp := $(addprefix ./build/obj/, $(notdir $(libiio_objects)))
libiio_objects     := $(addprefix ./src/iio/, $(libiio_objects))

libad9361_objects := $(libad9361_source)
libad9361_objects := $(libad9361_objects:.c=.o)
libad9361_objects_tmp := $(addprefix ./build/obj/, $(notdir $(libad9361_objects)))
libad9361_objects     := $(addprefix ./src/libad9361/, $(libad9361_objects))


utils_source := $(addprefix ./src/utils/, $(utils_source))
iiod_source  := $(addprefix ./src/iiod/, $(iiod_source))


compile_flags := -Wall -pipe -fPIC -DLIBIIO_EXPORTS -I./include -I./src/Common
build_flags  := -O3  -fvisibility=hidden -fpermissive
link_flags   := -s -pipe
libraries    := -lavahi-client -lavahi-common

all: libiio libad9361 iiod utils


libiio: obj_dirs $(libiio_objects)
	g++ ./src/main_linux.cpp -shared -o ./build/libiio.so $(libiio_objects_tmp) $(compile_flags) $(link_flags) $(libraries)

libad9361: obj_dirs $(libad9361_objects)
	g++ ./src/main_linux.cpp -shared -o ./build/libad9361.so $(libad9361_objects_tmp) $(compile_flags) $(link_flags) -L./build/ -liio

utils: 
	gcc $(utils_source) ./src/utils/iio_genxml.c -o ./build/iio_genxml -O3 -Wl,-rpath,'$$ORIGIN' -I./include -L./build/ -liio
	gcc $(utils_source) ./src/utils/iio_info.c -o ./build/iio_info -O3 -Wl,-rpath,'$$ORIGIN' -I./include -L./build/ -liio
	gcc $(utils_source) ./src/utils/iio_readdev.c -o ./build/iio_readdev -O3 -Wl,-rpath,'$$ORIGIN' -I./include -L./build/ -liio
	gcc $(utils_source) ./src/utils/iio_reg.c -o ./build/iio_reg -O3 -Wl,-rpath,'$$ORIGIN' -I./include -L./build/ -liio
	gcc $(utils_source) ./src/utils/iio_writedev.c -o ./build/iio_writedev -O3 -Wl,-rpath,'$$ORIGIN' -I./include -L./build/ -liio

iiod:
	gcc $(iiod_source) ./src/iiod/iiod.c -o ./build/iiod -O3 -Wl,-rpath,'$$ORIGIN' -D_GNU_SOURCE -I./include -L./build/ -liio -laio -lavahi-client -lavahi-common


obj_dirs:
	@mkdir -p ./build/
	@mkdir -p ./build/obj/

%.o : %.cpp
	g++ -o ./build/obj/$(notdir $@) -c $< $(compile_flags) $(build_flags)

%.o : %.c
	gcc -o ./build/obj/$(notdir $@) -c $< $(compile_flags) $(build_flags)

clean:
	rm -rf ./build/obj
	rm -f  ./build/libiio.so
	rm -f  ./build/libad9361.so


