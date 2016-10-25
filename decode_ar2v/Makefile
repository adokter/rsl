all : decode_ar2v

decode_ar2v: decode_ar2v.c
	libs=$$(cat ../Makefile | grep "LIBS = " | cut -c 7-); \
	echo $${libs}; \
	gcc -o decode_ar2v $${libs} -lbz2 decode_ar2v.c

.PHONY : install
install : decode_ar2v
	prefix=$$(cat ../Makefile | grep "prefix = " | grep -v "exec_prefix" | grep -v "top_build_prefix" | cut -c 10-); \
	install -d $${prefix}/bin; \
	install -m 755 decode_ar2v $${prefix}/bin/decode_ar2v

.PHONY : install
clean:
	rm decode_ar2v
