# Unfortunately setup.py below cannot handle srctree being ".." which it often
# is. It fails with an error like:
# Fatal error: can't create build/temp.linux-x86_64-2.7/../lib/libfdt/fdt.o:
#    No such file or directory
# To fix this, use an absolute path.
LIBFDT_srcdir = $(abspath $(srctree)/$(src)/../libfdt)

include $(LIBFDT_srcdir)/Makefile.libfdt

# Unfortunately setup.py (or actually the Python distutil implementation) puts
# files into the same directory as the .i file. We cannot touch the source
# directory, so we "ship" .i file into the objtree.
PYLIBFDT_srcs = $(addprefix $(LIBFDT_srcdir)/,$(LIBFDT_SRCS)) \
		$(obj)/libfdt.i

python_inc = $(shell $(PYTHON3) -c "from sysconfig import get_paths; print(get_paths()['include'])")

quiet_cmd_pymod = PYMOD   $@
      cmd_pymod = unset CROSS_COMPILE; unset CFLAGS; \
		CC="$(HOSTCC)" LDSHARED="$(HOSTCC) -shared " \
		LDFLAGS="$(HOSTLDFLAGS)" \
		VERSION="u-boot-$(UBOOTVERSION)" \
		CPPFLAGS="$(HOSTCFLAGS) -I$(LIBFDT_srcdir) -I$(python_inc) -I/usr/include/python3.8" OBJDIR=$(obj) \
		SOURCES="$(PYLIBFDT_srcs)" \
		SWIG_OPTS="-I$(LIBFDT_srcdir) -I$(LIBFDT_srcdir)/.." \
		$(PYTHON3) $< --quiet build_ext --inplace

$(obj)/_libfdt.so: $(src)/setup.py $(PYLIBFDT_srcs) FORCE
	$(call if_changed,pymod)

always += _libfdt.so

clean-files += libfdt.i _libfdt.so libfdt.py libfdt_wrap.c
