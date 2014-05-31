CC =gcc
LIBS = libmedia-service

OBJS = media_svc_plugin_test.o
SRCS = media_svc_plugin_test.c
TARGET = plugin_test

TARGET : $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) -ldl `pkg-config $(LIBS) --cflags --libs` -g
$(OBJS) : $(SRCS)
	$(CC) -c $(SRCS) -I../../include -L../../cmake_build_tmp `pkg-config $(LIBS) --cflags --libs` -g
clean :
	rm $(TARGET)
	rm $(OBJS)
