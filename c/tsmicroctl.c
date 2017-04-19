/*  Copyright 2017, Unpublished Work of Technologic Systems
*  All Rights Reserved.
*
*  THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
*  PROPRIETARY AND TRADE SECRET INFORMATION OF TECHNOLOGIC SYSTEMS.
*  ACCESS TO THIS WORK IS RESTRICTED TO (I) TECHNOLOGIC SYSTEMS EMPLOYEES
*  WHO HAVE A NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
*  ASSIGNMENTS  AND (II) ENTITIES OTHER THAN TECHNOLOGIC SYSTEMS WHO
*  HAVE ENTERED INTO  APPROPRIATE LICENSE AGREEMENTS.  NO PART OF THIS
*  WORK MAY BE USED, PRACTICED, PERFORMED, COPIED, DISTRIBUTED, REVISED,
*  MODIFIED, TRANSLATED, ABRIDGED, CONDENSED, EXPANDED, COLLECTED,
*  COMPILED,LINKED,RECAST, TRANSFORMED, ADAPTED IN ANY FORM OR BY ANY
*  MEANS,MANUAL, MECHANICAL, CHEMICAL, ELECTRICAL, ELECTRONIC, OPTICAL,
*  BIOLOGICAL, OR OTHERWISE WITHOUT THE PRIOR WRITTEN PERMISSION AND
*  CONSENT OF TECHNOLOGIC SYSTEMS . ANY USE OR EXPLOITATION OF THIS WORK
*  WITHOUT THE PRIOR WRITTEN CONSENT OF TECHNOLOGIC SYSTEMS  COULD
*  SUBJECT THE PERPETRATOR TO CRIMINAL AND CIVIL LIABILITY.
*/
/* To compile tshwctl, use the appropriate cross compiler and run the
* command:
*
*  arm-linux-gnueabihf-gcc -fno-tree-cselim -Wall -O0 -DCTL -o tsmicroctl tsmicroctl.c
*
*/

#include <stdio.h>
#include <unistd.h>
#include <dirent.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef CTL
#include <getopt.h>
#endif

#include "i2c-dev.h"

const char copyright[] = "Copyright (c) Technologic Systems - " __DATE__ ;

#ifdef CTL
int model = 0;

int get_model()
{
	FILE *proc;
	char mdl[256];
	char *ptr;

	proc = fopen("/proc/device-tree/model", "r");
	if (!proc) {
	    perror("model");
	    return 0;
	}
	fread(mdl, 256, 1, proc);
	ptr = strstr(mdl, "TS-");
	return strtoull(ptr+3, NULL, 16);
}
#endif

int silabs_init()
{
	static int fd = -1;
	fd = open("/dev/i2c-0", O_RDWR);
	if(fd != -1) {
		if (ioctl(fd, I2C_SLAVE_FORCE, 0x2a) < 0) {
			perror("Microcontroller did not ACK 0x2a\n");
			return -1;
		}
	}

	return fd;
}

#ifdef CTL

// Scale voltage to silabs 0-2.5V
uint16_t inline sscale(uint16_t data){
	return data * (2.5/1023) * 1000;
}

// Scale voltage for resistor dividers
uint16_t inline rscale(uint16_t data, uint16_t r1, uint16_t r2)
{
	uint16_t ret = (data * (r1 + r2)/r2);
	return sscale(ret);
}

void do_info(int twifd)
{
	uint8_t data[32];
	unsigned int pct;
	bzero(data, 32);
	read(twifd, data, 32);

	printf("revision=0x%x\n", data[31]);
	printf("SUPERCAP_V=%d\n", sscale((data[0]<<8|data[1])));
	printf("SUPERCAP_TOT=%d\n", rscale((data[2]<<8|data[3]), 20, 20));
	pct = (((data[2]<<8|data[3])*100/237));
	if (pct > 311) {
		pct = pct - 311;
	} else {
		pct = 0;
	}
	printf("SUPERCAP_PCT=%d\n", pct > 100 ? 100 : pct);

	printf("VCHARGE=%d\n", rscale((data[16]<<8|data[17]), 422, 422));
	printf("V4P7=%d\n", rscale((data[4]<<8|data[5]), 20, 20));

	printf("VIN=%d\n", rscale((data[6]<<8|data[7]), 1910, 172));
	printf("V5_A=%d\n", rscale((data[8]<<8|data[9]), 536, 422));
	printf("V3P3=%d\n", rscale((data[10]<<8|data[11]), 422, 422));
	printf("RAM_V1P35=%d\n", sscale((data[12]<<8|data[13])));
	printf("VCORE=%d\n", sscale((data[14]<<8|data[15])));
	printf("VSOC=%d\n", sscale((data[18]<<8|data[19])));
	printf("VARM=%d\n", sscale((data[20]<<8|data[21])));

	printf("temp_sensor=0x%x\n", data[22]<<8|data[23]);
	printf("reboot_source=");

	switch(data[30] & 0x3) {
	  case 0:
		printf("poweron\n");
		break;
	  case 1:
		printf("WDT\n");
		break;
	  case 2:
		printf("sleep\n");
		break;
	  default:
		printf("unknown\n");
		break;
	}

 
}

static void usage(char **argv) {
	fprintf(stderr, "Usage: %s [OPTION] ...\n"
	  "Technologic Systems Microcontroller Access\n"
	  "\n"
	  "  -i, --info              Get info about the microcontroller\n"
	  "  -L, --sleep=<time>      Sleep CPU, <time> seconds to wake up in\n"
	  "  -S, --supercapon        Enable charging of TS-SILO supercaps\n"
	  "  -s, --supercapoff       Disable charging of TS-SILO supercaps\n"
	  "  -h, --help              This message\n",
	  argv[0]
	);
}

int main(int argc, char **argv)
{
	int c;
	int twifd;
	int opt_timewkup = 0xffffff, opt_supercap = 0, opt_sleep = 0;;

	static struct option long_options[] = {
	  { "info", 0, 0, 'i' },
	  { "sleep", 1, 0, 'L'},
	  { "supercapon", 0, 0, 'S'},
	  { "supercapoff", 0, 0, 's'},
	  { "help", 0, 0, 'h' },
	  { 0, 0, 0, 0 }
	};

	if(argc == 1) {
		usage(argv);
		return(1);
	}

	model = get_model();
	switch(model) {
	  case 0x7553:
		break;
	  default:
		fprintf(stderr, "Unsupported model TS-%x\n", model);
		return 1;
	}

	twifd = silabs_init();
	if(twifd == -1)
	  return 1;

	

	while((c = getopt_long(argc, argv, 
	  "iL:hSs",
	  long_options, NULL)) != -1) {
		switch (c) {
		  case 'i':
			do_info(twifd);
			break;
		  case 'L':
			opt_sleep = 1;
			opt_timewkup = strtoul(optarg, NULL, 0);
			break;
		  case 'S':
			opt_supercap = 1;
			break;
		  case 's':
			opt_supercap = 2;
			break;
		  case 'h':
		  default:
			usage(argv);
			return 1;
		}
	}

	if(opt_sleep) {
		unsigned char dat[4] = {0};

		dat[3] = (opt_timewkup & 0xff);
		dat[2] = ((opt_timewkup >> 8) & 0xff);
		dat[1] = ((opt_timewkup >> 16) & 0xff);
		dat[0] = ((opt_timewkup >> 24) & 0xff);
		write(twifd, &dat, 4);
	}

	if(opt_supercap) {
		unsigned char dat[1] = {(opt_supercap & 0x1)};
		write(twifd, dat, 1);
	}


	
	return 0;
}

#endif
