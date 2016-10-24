all : decode_ar2v

decode_ar2v: decode_ar2v.c
	gcc -o decode_ar2v -lbz2 decode_ar2v.c

.PHONY : install
install : decode_ar2v
	prefix=$$(cat ../Makefile | grep "prefix = " | grep -v "exec_prefix" | grep -v "top_build_prefix" | cut -c 10-); \
	install -d $${prefix}/bin; \
	install -m 755 decode_ar2v $${prefix}/bin/decode_ar2v

.PHONY : install
clean:
	rm decode_ar2v
