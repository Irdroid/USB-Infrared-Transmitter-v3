#include <sys/ioctl.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/types.h>
#include <inttypes.h>

#define ENTER_BOOT_LOAD_IOCTL  _IOW('i', 0x00000013, __u32)

int main(int argc, char* argv[]) {
	
	uint32_t enterBoot = 6100000;
	
	if (argc == 1){
		printf("Usage: irdroidBoot [path to lirc dev entry]\n");
		return -1;
	}

	/* open LIRC dev file descriptor*/
	int fd = open(argv[1], O_WRONLY);	
	printf("%s", argv[1]);
	/* Check if file descriptor has opened correctly*/
	if (fd < 0) {
		printf("ERROR: %s\n", strerror(errno));
		return -1;
	}

	/* Write the command to the file descriptor */
	int ret = ioctl(fd, ENTER_BOOT_LOAD_IOCTL, &enterBoot);
	
	/* Check if was set correctly*/
	if (ret < 0) {
				printf("ERROR: %s\n", strerror(errno));
				return -2;
	}
	return 0;
}
