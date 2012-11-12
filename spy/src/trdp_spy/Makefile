# Modify to point to your Wireshark and glib include directories
INCS = -I../../ -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/include/libxml2 

SRCS     = plugin.c packet-trdp_spy.c parsebody.c

CC   = gcc

OBJS = $(foreach src, $(SRCS), $(src:.c=.o))

PLUGIN_NAME = packet-trdp_spy
#PLUGIN_DIR  = $(HOME)/.wireshark/plugins
PLUGIN_DIR  = .
PLUGIN      = $(PLUGIN_DIR)/$(PLUGIN_NAME).so

CFLAGS = -DHAVE_CONFIG_H $(INCS) -DINET6 -D_U_=__attribute__\(\(unused\)\) -Wall -Wpointer-arith -g -DXTHREADS -D_REENTRANT -DXUSE_MTSAFE_API -fPIC -DPIC

$(PLUGIN) : $(OBJS)
	mkdir -p $(PLUGIN_DIR)
	$(CC) -shared $(OBJS) -lxml2 -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@


clean:
	rm -f $(PLUGIN) $(OBJS)

