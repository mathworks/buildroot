LDLIBS += -lm

all: rf-init

rf-init: rf-init.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o rf-init

.PHONY: all clean
