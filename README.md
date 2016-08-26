Cloned from http://trmm-fc.gsfc.nasa.gov/trmm_gv/software/rsl/functionality_index.html

Because git does not store original timestamps of files, running a simple `make` results in a call to various autotools to regenerate Makefiles, which causes errors. To compile with original Makefile, instead compile with:
```
./configure
make AUTOCONF=: AUTOHEADER=: AUTOMAKE=: ACLOCAL=:
```

