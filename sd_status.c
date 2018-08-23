#include <stdio.h>
#include <stdlib.h>

#define N_BITS (512)
#define TYPE   (32)
/* #define NIB(data,high,low) ((*(data+64-(low/8)))>>(low%8))&((1<<(high-low))-1) */
int NIB(u_int32_t *data, int high, int low)
{
	int mask = 1<< (high-low+1);
	mask -= 1;
	int fullbytes = low/TYPE;
	int startbit = low%TYPE;
	return (*(data+(N_BITS/TYPE - 1)-fullbytes) >> startbit) & mask;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s dump.bin\n", *argv);
		return -1;
	}
	FILE *dump = fopen(argv[1], "rb");
	if (dump == NULL) {
		fprintf(stderr, "cannot open file %s\n", argv[1]);
		return -1;
	}
	u_int32_t dd[N_BITS/TYPE];
	int n =fread(dd, TYPE/8, N_BITS/TYPE, dump);
	fclose(dump);
	if (n != N_BITS/TYPE) {
		fprintf(stderr, "could not read %dbits from file\n", N_BITS);
		return -1;
	}
	fprintf(stderr,"SD Status:\n"
	"DAT_BUS_WIDTH\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,511,510));
	fprintf(stderr,
	"SECURED_MODE\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,509,509));
	fprintf(stderr,
	"SD_CARD_TYPE\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,495,480));
	fprintf(stderr,
	"SIZE_OF_PROTECTED_AREA\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,479,448));
	fprintf(stderr,
	"SPEED_CLASS\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,447,440));
	fprintf(stderr,
	"PERFORMANCE_MOVE\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,439,432));
	fprintf(stderr,
	"AU_SIZE\t\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,431,428));
	fprintf(stderr,
	"ERASE_SIZE\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,423,408));
	fprintf(stderr,
	"ERASE_TIMEOUT\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,407,402));
	fprintf(stderr,
	"ERASE_OFFSET\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,401,400));
	return 0;
}


