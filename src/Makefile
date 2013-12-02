CFLAGS = -I/usr/local/include/gpod-1.0/gpod \
	-I /usr/include/gpod-1.0/gpod \
	-I /usr/include/glib-2.0 \
	-I /usr/lib64/glib-2.0/include \
	-I /usr/lib/glib-2.0/include \
	-I /usr/include/taglib \
	-lgpod \
	-ltag \
	-lglib-2.0

all: main.cc
	g++ main.cc -o cl-ipod ${CFLAGS}

clean:
	-rm *.o cl-ipod
