## TRMM Radar Software Library (RSL)

Cloned from http://trmm-fc.gsfc.nasa.gov/trmm_gv/software/rsl/functionality_index.html

First compile decode_ar2v decoder:
```
cd decode_ar2v
./configure
make
cd ..
```
Next we compile RSL library. Because git does not store original timestamps of files, running a simple `make` results in a call to various autotools to regenerate Makefiles, which causes errors. To compile with original Makefile:
```
./configure
make AUTOCONF=: AUTOHEADER=: AUTOMAKE=: ACLOCAL=:
make install AUTOCONF=: AUTOHEADER=: AUTOMAKE=: ACLOCAL=:
```
or untar [rsl-v1.49.tar](https://github.com/adokter/rsl/blob/master/rsl-v1.49.tar) locally to preserve timestamps. This tar file contains the original RSL v1.49 code, i.e. not the bugfixes and commits in this repository
