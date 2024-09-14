# The GPL applies to this program.
# In addition, as a special exception, the copyright holders give
# permission to link the code of portions of this program with the
# OpenSSL library under certain conditions as described in each
# individual source file, and distribute linked combinations
# including the two.
# You must obey the GNU General Public License in all respects
# for all of the code used other than OpenSSL.  If you modify
# file(s) with this exception, you may extend this exception to your
# version of the file(s), but you are not obligated to do so.  If you
# do not wish to do so, delete this exception statement from your
# version.  If you delete this exception statement from all source
# files in the program, then also delete it here.

include version

TARGET=httping

WFLAGS=-Wall -W
OFLAGS=-O2
CFLAGS+=$(WFLAGS) $(OFLAGS) -DVERSION=\"$(VERSION)\" -g

PACKAGE=$(TARGET)-$(VERSION)
PREFIX=/usr
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man
DOCDIR=$(PREFIX)/share/doc/$(TARGET)

INSTALL=install
INSTALLDIR=$(INSTALL) -m 0755 -d
INSTALLBIN=$(INSTALL) -m 0755
INSTALLMAN=$(INSTALL) -m 0644
INSTALLDOC=$(INSTALL) -m 0644
STRIP=/usr/bin/strip
RMDIR=/bin/rm -rf
MKDIR=/bin/mkdir
ARCHIVE=/bin/tar cf -
COMPRESS=/bin/gzip -9

OBJS=mem.o http.o io.o str.o error.o utils.o main.o tcp.o res.o

MANS=httping.1

DOCS=license.txt license.OpenSSL readme.txt

ifeq ($(SSL),no)
CFLAGS+=-DNO_SSL
else
OBJS+=mssl.o
LDFLAGS+=-lssl -lcrypto
endif

ifeq ($(DEBUG),yes)
CFLAGS+=-D_DEBUG -g
LDFLAGS+=-g
endif

ifeq ($(ARM),yes)
CC=arm-linux-gcc
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(WFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)
	#
	# Oh, blatant plug: http://keetweej.vanheusden.com/wishlist.html

install: $(TARGET)
	$(INSTALLDIR) $(DESTDIR)/$(BINDIR)
	$(INSTALLBIN) $(TARGET) $(DESTDIR)/$(BINDIR)
	$(INSTALLDIR) $(DESTDIR)/$(MANDIR)/man1
	$(INSTALLMAN) $(MANS) $(DESTDIR)/$(MANDIR)/man1
	$(INSTALLDIR) $(DESTDIR)/$(DOCDIR)
	$(INSTALLDOC) $(DOCS) $(DESTDIR)/$(DOCDIR)
ifneq (DEBUG,yes)
	$(STRIP) $(DESTDIR)/$(BINDIR)/$(TARGET)
endif

clean:
	$(RMDIR) $(OBJS) $(TARGET) *~ core

package: clean
	# source package
	$(RMDIR) $(PACKAGE)*
	$(MKDIR) $(PACKAGE)
	$(INSTALLDOC) *.c *.h Makefile version $(MANS) $(DOCS) $(PACKAGE)
	$(ARCHIVE) $(PACKAGE) | $(COMPRESS) > $(PACKAGE).tgz
	$(RMDIR) $(PACKAGE)
