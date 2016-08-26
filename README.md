## TRMM Radar Software Library (RSL)

Cloned from http://trmm-fc.gsfc.nasa.gov/trmm_gv/software/rsl/functionality_index.html

Because git does not store original timestamps of files, running a simple `make` results in a call to various autotools to regenerate Makefiles, which causes errors. To compile with original Makefile:
```
./configure
make AUTOCONF=: AUTOHEADER=: AUTOMAKE=: ACLOCAL=:
```
or untar rsl-v1.49.tar locally to preserve timestamps
