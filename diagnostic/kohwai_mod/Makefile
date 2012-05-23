CC 	   = gcc
CFLAGS = -g -DKOWHAI_DBG -fPIC
KOWHAI_VER = 0.0.1

SOCKET_LIB = 
TEST_EXECUTABLE = test
ifeq ($(OS),Windows_NT)
	# on windows we need the winsock library
	SOCKET_LIB = -lws2_32
	# on windows we need the file extension
	TEST_EXECUTABLE = test.exe
endif

all: jsmn kowhai test

test: tools/test.o tools/xpsocket.o tools/beep.o
	$(CC) $(LDFLAGS) -o $@ $^ $(SOCKET_LIB) -L. -Wl,-Bstatic -lkowhai -Wl,-Bdynamic

kowhai: src/kowhai.o src/kowhai_protocol.o src/kowhai_protocol_server.o src/kowhai_serialize.o src/kowhai_utils.o 3rdparty/jsmn/jsmn.o
	ar rs lib$@.a $?
	# make a shared library for linux/mac (@todo versioning)
	$(CC) $(CFLAGS) -shared -Wl,-soname,lib$@.so -o lib$@.so $?

jsmn: 3rdparty/jsmn/jsmn.o
	ar rs lib$@.a $?

3rdparty/jsmn/jsmn.o: 3rdparty/jsmn/jsmn.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/kowhai.o: src/kowhai.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/kowhai_protocol.o: src/kowhai_protocol.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/kowhai_protocol_server.o: src/kowhai_protocol_server.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/kowhai_serialize.o: src/kowhai_serialize.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/kowhai_utils.o: src/kowhai_utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/test.o: tools/test.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/xpsocket.o: tools/xpsocket.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/beep.o: tools/beep.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean: 
	rm -f ${TEST_EXECUTABLE} libjsmn.a libkowhai.a tools/*.o src/*.o 3rdparty/jsmn/*.o

.PHONY: clean
