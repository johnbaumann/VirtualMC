#include "serial.h"
#include <time.h>
#include <string.h>

#if defined(__MINGW32__) || defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#define sleep(n) Sleep(n * 1000)
#endif

int mc_read(SERIAL *device, FILE *fp, int i)
{
	unsigned char block[130];

	unsigned char frame_lsb = i & 0xff;
	unsigned char frame_msb = i >> 8;

	printf("block %d read", i);

	serial_write(device, "\xa2", 1);

	serial_write(device, &frame_msb, 1);
	serial_write(device, &frame_lsb, 1);

	printf("...");

	unsigned char xor_data = (frame_msb ^ frame_lsb);

	while (serial_avaiable(device) < 130)
		;

	serial_read(device, block, 130);

	for (int j = 0; j < 128; j++)
	{
		xor_data ^= block[j];
	}

	if (block[129] == 0x47 && block[128] == xor_data)
	{
		fwrite(block, 1, 128, fp);
		i++;
		printf("done\n");
	}
	else
	{
		printf("fail. Checksum expected: 0x%02x Received: 0x%02x and block[129]: : 0x%02x\n", xor_data, block[128], block[129]);
		for (int h = 0; h < 129; h++)
		{
			printf("%02x", block[h]);
		}
		printf("\n");
		i++;
	}

	return i;
}

int mc_write(SERIAL *device, FILE *fp, int i)
{
	unsigned char block[130];

	unsigned char frame_lsb = i & 0xff;
	unsigned char frame_msb = i >> 8;

	serial_write(device, "\xa3", 1);

	serial_write(device, &frame_msb, 1);
	serial_write(device, &frame_lsb, 1);

	unsigned char xor_data = (frame_msb ^ frame_lsb);

	fread(block, 1, 128, fp);

	for (int j = 0; j < 128; j++)
	{
		xor_data ^= block[j];
	}

	printf("block %d write", i);

	serial_write(device, block, 128);
	serial_write(device, &xor_data, 1);

	printf("...");

	while (serial_avaiable(device) < 1)
		;

	serial_read(device, block, 1);

	if (block[0] == 0x47)
	{
		i++;
		printf("done\n");
	}
	else
	{
		fseek(fp, -128, SEEK_CUR);
		printf("fail(code=%02x)\n", block[0]);
	}

	return i;
}

int main(int argc, char *argv[])
{
	char portname[256] = "";
	char filename[256] = "";
	char deviceid[256] = "";
	char devicever[256] = "";
	char filler[128] = "";
	char bToggleCard = 1;
	char bNewCardState = 0;
	char bTogglePad = 1;
	char bNewPadState = 0;
	char bNonReadWriteCmnd = 1;
	char bGetVer = 1;
	char bSendPad = 1;

	int rw_mode = -1;
	char *to_write = NULL;

	if (argc == 1)
	{
		printf("usage : %s -p Serialport [-r Filename] [-w Filename] [-c] [-help]\n\n", argv[0]);
		printf("-p    : Specify serial port(Required)\n");
		printf("-r    : Specify the data load destination file(Optional)\n");
		printf("-w    : Specify the data writing source file(Optional)\n");
		printf("-c    : Specify auto-naming mode (read-only, -r and -w will be ignore)(Optional)\n");
		printf("-i    : Retrieve ID info from Arduino device\n");
		printf("-v    : Retrieve version info from Arduino device\n");
		printf("-o    : Toggle Memory Card On (Optional)\n");
		printf("-x    : Toggle Memory Card Off (Optional)\n");
		printf("-y    : Toggle Pad On (Optional)\n");
		printf("-z    : Toggle Pad Off (Optional)\n");
		printf("-d    : DEBUG TEST PAD COMMAND (Optional)\n");
		printf("-u    : DEBUG TEST PAD COMMAND (Optional)\n");
		printf("-help : Show this message\n");
		return 1;
	}

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-help") == 0)
		{
			printf("usage : %s -p Serialport [-r Filename] [-w Filename] [-c] [-help]\n\n", argv[0]);
			printf("-p    : Specify serial port(Required)\n");
			printf("-r    : Specify the data load destination file(Optional)\n");
			printf("-w    : Specify the data writing source file(Optional)\n");
			printf("-c    : Specify auto-naming mode (read-only, -r and -w will be ignore)(Optional)\n");
			printf("-i    : Retrieve ID info from Arduino device (Optional)\n");
			printf("-v    : Retrieve version info from Arduino device (Optional)\n");
			printf("-o    : Toggle Memory Card On (Optional)\n");
			printf("-x    : Toggle Memory Card Off (Optional)\n");
			printf("-y    : Toggle Pad On (Optional)\n");
			printf("-z    : Toggle Pad Off (Optional)\n");
			printf("-d    : DEBUG TEST PAD COMMAND (Optional)\n");
			printf("-u    : DEBUG TEST PAD COMMAND (Optional)\n");
			printf("-help : Show this message\n");
			return 1;
		}

		if (to_write != NULL)
		{
			strcpy(to_write, argv[i]);
			to_write = NULL;
			continue;
		}
		else
		{
			if (strcmp(argv[i], "-p") == 0)
			{
				to_write = portname;
			}
			else if (strcmp(argv[i], "-r") == 0)
			{
				if (rw_mode != -1)
				{
					printf("The options \"-r\" and \"-w\" can not be used at the same time.\n");
					return 2;
				}
				to_write = filename;
				rw_mode = 0;
				bGetVer = 0;
			}
			else if (strcmp(argv[i], "-w") == 0)
			{
				if (rw_mode != -1)
				{
					printf("The options \"-r\" and \"-w\" can not be used at the same time.\n");
					return 2;
				}
				to_write = filename;
				rw_mode = 1;
				bGetVer = 0;
			}
			else if (strcmp(argv[i], "-c") == 0)
			{
				time_t utctime = time(NULL);
				struct tm *local = localtime(&utctime);
				sprintf(filename, "%d-%d-%d-%02d-%02d-%02d.mcr",
						local->tm_year + 1900,
						local->tm_mon + 1,
						local->tm_mday,
						local->tm_hour,
						local->tm_min,
						local->tm_sec);
				rw_mode = 0;
			}
			else if (strcmp(argv[i], "-v") == 0)
			{
				bGetVer = 0;
			}
			else if (strcmp(argv[i], "-x") == 0)
			{
				bToggleCard = 0;
				bNewCardState = 1;
				bGetVer = 0;
			}
			else if (strcmp(argv[i], "-o") == 0)
			{
				if (bToggleCard == 0)
				{
					printf("The options \"-x\" and \"-o\" can not be used at the same time.\n");
					return 2;
				}
				else
				{
					bToggleCard = 0;
					bNewCardState = 0;
					bGetVer = 0;
				}
			}
			else if (strcmp(argv[i], "-y") == 0)
			{
				bTogglePad = 0;
				bNewPadState = 0;
				bGetVer = 0;
			}
			else if (strcmp(argv[i], "-z") == 0)
			{
				if (bTogglePad == 0)
				{
					printf("The options \"-y\" and \"-z\" can not be used at the same time.\n");
					return 2;
				}
				else
				{
					bTogglePad = 0;
					bNewPadState = 1;
					bGetVer = 0;
				}
			}
			else if (strcmp(argv[i], "-d") == 0)
			{
				bSendPad = 0;
			}
			else if (strcmp(argv[i], "-u") == 0)
			{
				if (bSendPad == 0)
				{
					printf("The options \"-u\" and \"-d\" can not be used at the same time.\n");
					return 2;
				}
				else
					bSendPad = 2;
			}
		}
	}

	if (bToggleCard == 0 || bTogglePad == 0 || bSendPad == 0 || bSendPad == 2 || bGetVer == 0)
		bNonReadWriteCmnd = 0;
	else
		bNonReadWriteCmnd = 1;

	if (filename[0] == 0 && bNonReadWriteCmnd == 1)
	{
		puts("nothing to do.");

		return 3;
	}

	if (portname[0] == 0)
	{
		puts("fatal : Serial port designation is a required option.");
		return 4;
	}

	SERIAL *device = serial_open(portname, SerialBaud38400);

	if (!device)
	{
		puts("fatal : Failed to open serial port.");
		return 5;
	}

	char id[7];

	sleep(2);

	while (serial_avaiable(device))
	{
		char dmy;
		serial_read(device, &dmy, 1);
	}

	puts("Detecting device ...");
	serial_write(device, "\xa0", 1);
	if (serial_read_with_timeout(device, id, 6, 10000))
	{
		puts("fatal : Connection failed.");
		return 6;
	}
	else
	{
		if (strncmp(id, "MCDINO", 6) == 0)
		{
			puts("Detected MemCARDuino.");
		}
		else if (strncmp(id, "MCDPLS", 6) == 0)
		{
			puts("Detected MemCarDuinoPlus.");
		}
		else if (strncmp(id, "VIRMCD", 6) == 0)
		{
			puts("Detected VirtualMC.");
		}
		else
		{
			puts("fatal : Unknown Device");
			//puts(id);
			return 7;
		}
		unsigned char version;
		if (bGetVer == 0)
		{
			serial_write(device, "\xa1", 1);
			while (serial_avaiable(device) < 1)
				;
			serial_read(device, &version, 1);
			int version_major = version >> 4, version_minor = version & 0xf;
			printf("Version : %d.%d\n", version_major, version_minor);
		}

		unsigned char temp;
		if (bToggleCard == 0)
		{
			if (bNewCardState == 0)
			{
				serial_write(device, "\xb2", 1);
				while (serial_avaiable(device) < 1)
					;
				serial_read(device, &temp, 1);
				printf("Card set to on\n");
			}
			else
			{
				serial_write(device, "\xb3", 1);
				while (serial_avaiable(device) < 1)
					;
				serial_read(device, &temp, 1);
				printf("Card set to off\n");
			}
		}

		if (bTogglePad == 0)
		{
			if (bNewPadState == 0)
			{
				serial_write(device, "\xb0", 1);
				while (serial_avaiable(device) < 1)
					;
				serial_read(device, &temp, 1);
				printf("Pad set to on\n");
			}
			else
			{
				serial_write(device, "\xb1", 1);
				while (serial_avaiable(device) < 1)
					;
				serial_read(device, &temp, 1);
				printf("Pad set to off\n");
			}
		}

		if (bSendPad == 0)
		{
			serial_write(device, "\xc0", 1); //Pad data coming
			serial_write(device, "\xFF", 1); //swhi Digital switches MSB
			serial_write(device, "\xBF", 1); //swlo Digital switches LSB
			serial_write(device, "\xFF", 1); //adc1 Analog 1 MSB
			serial_write(device, "\xFF", 1); //adc0 Analog 1 LSB
			serial_write(device, "\xFF", 1); //adc3 Analog 2 MSB
			serial_write(device, "\xFF", 1); //adc2 Analog 2 LSB
			printf("Down button pressed\n");
		}
		else if (bSendPad == 2)
		{
			serial_write(device, "\xc0", 1); //Pad data coming
			serial_write(device, "\xFF", 1); //swhi Digital switches MSB
			serial_write(device, "\xFF", 1); //swlo Digital switches LSB
			serial_write(device, "\xFF", 1); //adc1 Analog 1 MSB
			serial_write(device, "\xFF", 1); //adc0 Analog 1 LSB
			serial_write(device, "\xFF", 1); //adc3 Analog 2 MSB
			serial_write(device, "\xFF", 1); //adc2 Analog 2 LSB
			printf("Pad state cleared\n");
		}

		if (filename[0] == 0)
			return 0;
	}

	FILE *fp = fopen(filename, rw_mode == 0 ? "wb" : "rb");

	if (!fp)
	{
		printf("fatal : Failed to open the filename %s.\n", filename);
		return 6;
	}

	int i = 0;

	//while(i < 1024) // 1024 Frames = Full memory card read
	//while(i < 64) // 64 = 1 Block
	//while(i < 128 )// 128 = 2 Blocks
	while (i < 192) // 192 = 3 Blocks

	//while(i < 1 )// Basic testing
	{
		if (rw_mode == 0)
			i = mc_read(device, fp, i);
		if (rw_mode == 1)
			i = mc_write(device, fp, i);
	}

	if (rw_mode == 0)
	{
		for (int i2 = 0; i2 < 128; i2++)
		{
			filler[i2] = 0xFF;
		}

		for (i = i; i < 1024; i++)
		{
			fwrite(filler, 1, 128, fp);
		}
	}

	fclose(fp);
	serial_close(device);

	return 0;
}
