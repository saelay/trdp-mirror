# Makefle to build the plugin on Linux
#
# Modify to point to your Wireshark and glib include directories
# Set the variable WIRESHAR_SRC in thie Makefile, of modify our environment:
# export WIRESHARK_SRC=$HOME/src/wireshark-1.8.3
# $Id: $


INCS = -I $(WIRESHARK_SRC) -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/include/libxml2 
# Debugging:
#INCS += -DPRINT_DEBUG

SRCS     = plugin.c packet-trdp_spy.c parsebody.c lookuptype.c trdp_env.c

CC   = gcc

OBJS = $(foreach src, $(SRCS), $(src:.c=.o))

PLUGIN_NAME = packet-trdp_spy
#PLUGIN_DIR  = $(HOME)/.wireshark/plugins
PLUGIN_DIR  = .
PLUGIN      = $(PLUGIN_DIR)/$(PLUGIN_NAME).so
REFMAN_PDF = "TCN-TRDP2- TRDP SPY Reference Manual.pdf"

CFLAGS = -DHAVE_CONFIG_H $(INCS) -DINET6 -D_U_=__attribute__\(\(unused\)\) -Wall -Wpointer-arith -g -DXTHREADS -D_REENTRANT -DXUSE_MTSAFE_API -fPIC -DPIC

$(PLUGIN) : $(OBJS)
	mkdir -p $(PLUGIN_DIR)
	$(CC) -shared $(OBJS) -lxml2 -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@


clean:
	rm -f $(PLUGIN) $(OBJS)
	rm -rf doc/latex
	rm -rf doc/html

doc: doc/latex

doc/latex: *.h* doc/custom/* Doxyfile
	@echo Run doxygen
	doxygen Doxyfile
	@echo Generate the PDF
	cd doc/latex; make >> /dev/null
	cp doc/latex/refman.pdf $(REFMAN_PDF)

