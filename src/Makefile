CFLAGS = `pkg-config --cflags glib-2.0 libgpod-1.0 taglib`
LDFLAGS = `pkg-config --libs glib-2.0 libgpod-1.0 taglib`

all: main.cc
	g++ $(CFLAGS) main.cc -o cl-ipod $(LDFLAGS)

clean:
	-rm *.o cl-ipod
