include $(TOPDIR)/.config

CC = gcc

all: genvcd gendesc gendiags
#######build genvcd##############
ifdef CONFIG_TARGET_siflower_sf19a28_fullmask
GENVCD_CSOURCES = genvcd_a28.c
GENVCD_CFLAGS = -O2 -g -Wall -DSIFLOWER_SF19A28
else
ifdef CONFIG_TARGET_siflower_sf19a28_mpw1

GENVCD_CSOURCES = genvcd_a28.c

GENVCD_CFLAGS = -O2 -g -Wall -DSIFLOWER_SF19A28
else

GENVCD_CSOURCES = genvcd.c

GENVCD_CFLAGS = -O2 -g -Wall -DSIFLOWER_SF16A18
endif
endif

GENVCD_COBJECTS=${patsubst %.c, %.o, $(GENVCD_CSOURCES)}

$(GENVCD_COBJECTS):%o:%c
	    $(CC) $(GENVCD_CFLAGS) -o $@ -c $<

genvcd: $(GENVCD_COBJECTS)
		$(CC) -o $@ $(GENVCD_COBJECTS)

########build gendsc##############
GENDESC_CSOURCES = gendesc.c

GENDESC_CFLAGS = -O2 -g -Wall

GENDESC_COBJECTS=${patsubst %.c, %.o, $(GENDESC_CSOURCES)}

$(GENDESC_COBJECTS):%o:%c
	    $(CC) $(GENDESC_CFLAGS) -o $@ -c $<

gendesc: $(GENDESC_COBJECTS)
		$(CC) -o $@ $(GENDESC_COBJECTS)

########build gendiags##############
GENDIAGS_CSOURCES = gendiags.c

GENDIAGS_CFLAGS = -O2 -g -Wall

GENDIAGS_COBJECTS=${patsubst %.c, %.o, $(GENDIAGS_CSOURCES)}

$(GENDIAGS_COBJECTS):%o:%c
	    $(CC) $(GENDIAGS_CFLAGS) -o $@ -c $<

gendiags: $(GENDIAGS_COBJECTS)
		$(CC) -o $@ $(GENDIAGS_COBJECTS)

clean:
	rm -f rbcfg *.o
