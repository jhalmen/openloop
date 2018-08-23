#include <stdio.h>
#include <stdlib.h>

#define N_BITS (128)

/* #define NIB(data,high,low) ((*(data+64-(low/8)))>>(low%8))&((1<<(high-low))-1) */
int NIB(u_int32_t *data, int high, int low)
{
	int mask = 1<< (high-low+1);
	mask -= 1;
	int fullbytes = low/32;
	int startbit = low%32;
	return (*(data+(N_BITS/32 - 1)-fullbytes) >> startbit) & mask;
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
	u_int32_t dd[N_BITS/32];
	int n =fread(dd, 4, N_BITS/32, dump);
	fclose(dump);
	if (n != N_BITS/32) {
		fprintf(stderr, "could not read %dbits from file\n", N_BITS);
		return -1;
	}
	printf("whole thing:\n");
	for (int i = 0; i < N_BITS/32; ++i)
		printf("%08x ", dd[i]);
	printf("\n");
	printf("SD CID Register:\n"
	"CSD_STRUCTURE\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,127,126));
	printf(
	"TAAC\t\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,119,112));
	printf(
	"NSAC\t\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,111,104));
	printf(
	"TRAN_SPEED\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,103,96));
	printf(
	"CCC\t\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,95,84));
	printf(
	"READ_BL_LEN\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,83,80));
	printf(
	"READ_BL_PARTIAL\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,79,79));
	printf(
	"WRITE_BLK_MISALIGN\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,78,78));
	printf(
	"READ_BLK_MISALIGN\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,77,77));
	printf(
	"DSR_IMPL\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,76,76));
	printf(
	"C_SIZE\t\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,73,62));
	printf(
	"VDD_R_CURR_MIN\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,61,59));
	printf(
	"VDD_R_CURR_MAX\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,58,56));
	printf(
	"VDD_W_CURR_MIN\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,55,53));
	printf(
	"VDD_W_CURR_MAX\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,52,50));
	printf(
	"C_SIZE_MULT\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,49,47));
	printf(
	"ERASE_BLK_EN\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,46,46));
	printf(
	"SECTOR_SIZE\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,45,39));
	printf(
	"WP_GRP_SIZE\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,38,32));
	printf(
	"WP_GRP_ENABLE\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,31,31));
	printf(
	"R2W_FACTOR\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,28,26));
	printf(
	"WRITE_BL_LEN\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,25,22));
	printf(
	"WRITE_BL_PARTIAL\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,21,21));
	printf(
	"FILE_FORMAT_GRP\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,15,15));
	printf(
	"COPY\t\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,14,14));
	printf(
	"PERM_WRITE_PROTECT\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,13,13));
	printf(
	"TMP_WRITE_PROTECT\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,12,12));
	printf(
	"FILE_FORMAT\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,11,10));
	printf(
	"CRC\t\t\t= 0x%1$x\t[%1$d]\n",
	NIB(dd,7,1));
	return 0;
}


