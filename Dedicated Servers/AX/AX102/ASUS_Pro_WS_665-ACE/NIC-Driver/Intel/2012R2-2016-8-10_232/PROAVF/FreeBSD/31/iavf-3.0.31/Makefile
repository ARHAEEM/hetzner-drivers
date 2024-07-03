#$FreeBSD$

# Automatically enable multi-job make if it wasn't specified. To disable
# running in parallel, the caller may pass -j1.
.if !defined(.MAKE.JOBS)
.MAKE.JOBS != sysctl -n hw.ncpu
.endif

#Ensure that pass the maximum number of jobs to all recursive Make processes
MAKE += -j $(.MAKE.JOBS)

KMODDIR ?= /boot/kernel

# Attempt to locate the running kernel object directory using the
# kern.version sysctl. This enables reading the kernel configuration,
# including things like INVARIANTS, RSS, INET, DTRACE, and other optional
# features.
#
# Without this, the module is built purely from the source with "default"
# options that may not match the running kernel.
#
# To override this, specify the KERNBUILDDIR manually on the command line.
.if !defined(KERNBUILDDIR)
KERNBUILDDIR := ${:!sysctl -n kern.version!:[-1]:C/^.*://}
.if !(exists(${KERNBUILDDIR}) && \
      exists(${KERNBUILDDIR}/Makefile) && \
      exists(${KERNBUILDDIR}/opt_global.h))
.info Unable to locate the running kernel object directory. Kernel options may not be accurate.
.undef KERNBUILDDIR
.endif
.endif

THIS_FILE := $(.PARSEDIR)/$(.PARSEFILE)

.include <bsd.own.mk>
.include <bsd.compiler.mk>

.PATH: ${.CURDIR}

KMOD = if_iavf
SRCS != ls *.[ch]
SRCS += device_if.h bus_if.h pci_if.h

# kernel option files
SRCS += opt_inet.h opt_inet6.h opt_rss.h

# Set to 1 to force compile without iflib
LEGACY_ENABLE ?= 0
# Uncomment to compile in debugging code
# CFLAGS += -DIAVF_DEBUG

KERNVER != uname -K
NO_IFLIB != test $(KERNVER) -ge 1100110; echo $$?

.if $(NO_IFLIB)
  LEGACY_ENABLE = 1
.endif

.if $(LEGACY_ENABLE)
  CFLAGS += -DIAVF_NO_IFLIB
  SRCS := ${SRCS:N*iflib*}
.else
  CFLAGS += -DIFLIB
  SRCS += ifdi_if.h
  SRCS := ${SRCS:N*legacy*}
.endif

CFLAGS += -Wformat=2 -Wextra -Wno-missing-field-initializers

# Mark some include files as system headers, to ignore warnings we can't fix
CFLAGS += --system-header-prefix=net --system-header-prefix=sys
CFLAGS += --system-header-prefix=dev/pci

DEBUG_FLAGS = -g3 -gdwarf-4

.if ${COMPILER_TYPE} == "clang"
.if ${COMPILER_VERSION} >= 60000
DEBUG_FLAGS += -fdebug-macro
.endif
.endif

clean:
	rm -f opt_*.h *_if.h setdef* *_StripErr
	rm -f *.o *.kld *.ko *.ko.* .depend.* *.profraw
	rm -f @ export_syms machine x86

.include <bsd.kmod.mk>

legacy:
	$(MAKE) -f $(THIS_FILE) LEGACY_ENABLE=1
