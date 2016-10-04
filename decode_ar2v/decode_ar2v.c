#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <bzlib.h>

static char *compression_type = "BZIP2";

/*
  nexradII outfile
*/
static void
    usage(
	char *av0 /*  id string */
	  )
{
    (void)fprintf(stderr, "Usage: %s <inputfile> <outputfile>\n", av0);
    exit(1);
}

int main(int argc, char *argv[], char *envp[])
{
    char clength[4];
    char *block = (char *)malloc(8192), *oblock = (char *)malloc(262144);
    unsigned isize = 8192, osize=262144, olength;
    char stid[5]={0};						/* station id: not used */
		
    /*
     * process command line arguments
     */
		
    if (argc < 2) usage(argv[0]);
		
    char *infile = argv[1];
    char *outfile = argv[2];
		
    int fdin = open(infile, O_RDONLY);
		
    if (fdin == -1)
    {
	fprintf(stderr, "Failed to open input file %s\n", infile);
	exit(1);
    }

    int fd = open(outfile, O_WRONLY | O_CREAT, 0664);
		
    if (fd == -1)
    {
	fprintf(stderr, "Failed to open output file %s\n", outfile);
	exit(1);				
    }

    /*
     * Loop through the blocks
     */

    int go = 1;
    while (go) {

	/*
	 * Read first 4 bytes
	 */

	ssize_t i = read(fdin, clength, 4);
	if (i != 4) {
	    if (i > 0) {
		fprintf ( stderr, "Short block length\n");
		exit(1);
	    }
	    else
	    {
		fprintf ( stderr, "Can't read file identifier string\n");
		exit(1);						
	    }
	}

	/*
	 * If this is the first block, read/write the remainder of the
	 * header and continue
	 */
		
	if ( (memcmp(clength, "ARCH", 4)==0) ||
	     (memcmp(clength, "AR2V", 4)==0) ) {
	    memcpy(block, clength, 4);
				
	    i = read(fdin, block+4, 20);
	    if (i != 20) {
		fprintf ( stderr, "Missing header\n");
		exit(1);
	    }
				
	    if ( stid[0] != 0 ) memcpy(block+20,stid,4);
	    lseek(fd, 0, SEEK_SET);
	    write(fd, block, 24);
	    continue;
	}

	/*
	 * Otherwise, this is a compressed block.
	 */
		
	int length = 0;
	for(i=0;i<4;i++)
	    length = ( length << 8 ) + (unsigned char)clength[i];
		
	if(length < 0) {						/* signals last compressed block */
	    length = -length;
	    go = 0;
	}
		
	if (length > isize) {
	    isize = length;
	    if ((block = (char *)realloc(block, isize)) == NULL) {
		fprintf ( stderr, "Cannot re-allocate input buffer\n");
		exit(1);
	    }
	}

	/* 
	 * Read, uncompress, and write
	 */

	i = read(fdin, block, length);
	if (i != length) {
	    fprintf ( stderr, "Short block read\n");
	    exit(1);
	}
	if (length > 10) {
	    int error;
				
	tryagain:
	    olength = osize;
#ifdef BZ_CONFIG_ERROR
	    error = BZ2_bzBuffToBuffDecompress(oblock, &olength, block, length, 0, 0);
	    /*error = bzBuffToBuffDecompress(oblock, &olength,*/
#else
	    error = bzBuffToBuffDecompress(oblock, &olength,block, length, 0, 0);
#endif

	    if (error) {
		if (error == BZ_OUTBUFF_FULL) {
		    osize += 262144;
		    if ((oblock=(char*) realloc(oblock, osize)) == NULL) {
			fprintf(stderr, "Cannot allocate output buffer\n");
			exit(1);
		    }
		    goto tryagain;
		}
		fprintf(stderr, "decompress error - %d\n", error);
		exit(1);
	    }
				
	    write(fd, oblock, olength);
	}
    }
    close(fdin);
    close(fd);
}
