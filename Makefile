###########################################################################
## Initial configuration
# if pkgconfig is not in path, use
#   make PKGCONFIG=...
# if pkgconfig has no fuse, add FUSE_CFLAGS and FUSE_LIBS

PKGCONFIG := $(shell which pkg-config)

DESTDIR :=
PREFIX := /usr
BINDIR := $(PREFIX)/bin
INCLUDEDIR := $(PREFIX)/include

FUSE_CFLAGS := _notset
FUSE_LIBS := _notset

ERRORS=

###########################################################################
## Build information:

PATTERNDEV := patterndev
PATTERNDEV_SRC = src/pattern-device.c
PATTERNDEV_OBJ = $(patsubst %.c,%.o,${PATTERNDEV_SRC})

BINYES := binyes
BINYES_SRC = src/binyes.c
BINYES_OBJ = $(patsubst %.c,%.o,${BINYES_SRC})

TARGETS := $(PATTERNDEV) $(BINYES) header

###########################################################################
## What otherwise autoconf performs:

M_TARGETS=
ifeq ($(FUSE_CFLAGS)$(FUSE_LIBS), _notset_notset)
  ifeq ($(shell test -x $(PKGCONFIG) || echo NO), NO)
    ERRORS+=no-pkgconfig
  else
    FUSE_CFLAGS = $(shell $(PKGCONFIG) --cflags fuse)
    FUSE_LIBS = $(shell $(PKGCONFIG) --libs fuse)
  endif
else
  # clear _notsets
  ifeq ($(FUSE_CFLAGS), _notset)
    FUSE_CFLAGS=
  endif
  ifeq ($(FUSE_LIBS), _notset)
    FUSE_LIBS=
  endif
endif

M_CFLAGS = $(CFLAGS)
M_LDFLAGS = $(LDFLAGS)
M_LIBS = $(LIBS)

ifeq ($(ERRORS), )
  M_TARGETS=$(TARGETS)
endif

.PHONY: header

all: $(ERRORS) $(M_TARGETS)

no-pkgconfig:
	@echo pkg-config not found, please provide FUSE_CFLAGS and FUSE_LIBS

$(PATTERNDEV): $(PATTERNDEV_OBJ)
	$(CC) $(M_LDFLAGS) -o $@ $(PATTERNDEV_OBJ) $(FUSE_LIBS) $(M_LIBS)
$(BINYES): $(BINYES_OBJ)
	$(CC) $(M_LDFLAGS) -o $@ $(BINYES_OBJ) $(M_LIBS)

install: $(patsubst %,install-%,$(M_TARGETS))
install-header:
	-install -d -m755                     $(DESTDIR)$(INCLUDEDIR)
	-install    -m644 src/patterndevice.h $(DESTDIR)$(INCLUDEDIR)/
install-$(PATTERNDEV): $(PATTERNDEV)
	-install -d -m755                     $(DESTDIR)$(BINDIR)
	-install    -m755 $(PATTERNDEV)       $(DESTDIR)$(BINDIR)/
install-$(BINYES): $(BINYES)
	-install    -m755 $(BINYES)           $(DESTDIR)$(BINDIR)/

#%.o: %.c
#	$(CC) $(M_CFLAGS) -c -o $@ $^ -MMD -MF $@.d -MT $@
$(PATTERNDEV_OBJ):
	$(CC) $(FUSE_CFLAGS) $(M_CFLAGS) -c -o $@ $(patsubst %.o,%.c,$@) -MMD -MF $@.d -MT $@

%.o: %.c
	$(CC) $(M_CFLAGS) -c -o $@ $^ -MMD -MF $@.d -MT $@

clean:
	-rm -f src/*.o src/*.d
	-rm -f $(PATTERNDEV) $(BINYES)

check:
	-sh test-binyes.sh

-include src/*.d
