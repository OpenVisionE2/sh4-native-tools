/**************************************************************************/
/* Name :   fup                                                           */
/*                                                                        */
/* Author:  Schischu, enhanced by Audioniek                               */
/*                                                                        */
/* License: This file is subject to the terms and conditions of the       */
/*          GNU General Public License version 2.                         */
/**************************************************************************/
/*
 * + TODO: -i: display detailed ird info.
 * + TODO: change loader reseller ID.
 *
 * Changes in Version 1.9.3:
 * + -xv displays model name based on header resellerID.
 *
 * Changes in Version 1.9.2:
 * + Use correct mtd numbers and partition names on extract
 *   depending on resellerID.
  *
 * Changes in Version 1.9.1:
 * + -d and -dv options added: create dummy squashfs file.
 *
 * Changes in Version 1.9.0:
 * + Dummy squash header file is only created when needed.
 * + -rv shows old resellerID also.
 *
 * Changes in Version 1.8.3b:
 * + More rigid argument checking with -c and -ce; missing filenames for
 *   suboptions requiring them are reported.
 * + Several bugs involving suboptions -v and -1G in -c and -ce fixed.
 *
 * Changes in Version 1.8.3a:
 * + -ce can now add block types 2, 3, 4 and 5 (config0 - configA);
 * + Option -1G added to -ce to use squashfs dummy file with squashfs 3.0 in
 *   stead of 3.1 as required by first generation Fortis receivers.
 *
 * Changes in Version 1.8.2:
 * + -tv added;
 * + Cosmetic changes to output of -t and -tv.
 *
 * Changes in Version 1.8.1:
 * + Fixed two compiler warnings.
 * + Squashfs dummy file now padded with 0xFF in stead of 0x00.
 *
 * Changes in Version 1.8:
 * + If the file dummy.squash.signed.padded does not exist in the
 *   current directory at program start, it is created;
 * + -r option can now change all four reseller ID bytes;
 * + -ce suboptions now have numbered aliases;
 * + On opening a file, the result is tested: no more crashes
 *   on non existent files, but a neat error message;
 * + -n can change version number in IRD;
 * + Silent operation now possible on -x, -s, -n, -c, -r and -ce;
 * + Errors in mtd numbering corrected.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <zlib.h>

#include "fup.h"
#include "crc16.h"
#include "dummy30.h"
#include "dummy31.h"

#define VERSION "1.9.3"
#define DATE "10.01.2020"

//#define USE_ZLIB

uint8_t verbose = 1;
char printstring;
uint8_t has[MAX_PART_NUMBER];
FILE *fd[MAX_PART_NUMBER];


/* Functions */

void printverbose(const char *printstring)
{
	if (verbose)
	{
		printf("%s", printstring);
	}
}

#if 0
void fromint16_t(uint8_t *int16_tBuf, uint16_t val)
{
	int16_tBuf[0] = val >> 8;
	int16_tBuf[1] = val & 0xFF;
}
#endif

uint16_t toShort(uint8_t int16_tBuf[2])
{
	return (uint16_t)(int16_tBuf[1] + (int16_tBuf[0] << 8));
}

uint16_t extractShort(uint8_t dataBuf[], uint16_t pos)
{
	uint8_t int16_tBuf[2];

	memcpy(int16_tBuf, dataBuf + pos, 2);
	return toShort(int16_tBuf);
}

uint16_t readShort(FILE *file)
{
	uint8_t int16_tBuf[2];

	if (fread(int16_tBuf, 1, 2, file) == 2)
	{
		return toShort(int16_tBuf);
	}
	return 0;
}

int32_t extractAndWrite(FILE *file, uint8_t *buffer, uint16_t len, uint16_t decLen)
{
#if defined USE_ZLIB
	if (len != decLen)
	{
		// zlib
		z_stream strm;
		uint8_t out[decLen];

		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		inflateInit(&strm);

		strm.avail_in = len;
		strm.next_in = buffer;

		strm.avail_out = decLen;
		strm.next_out = out;
		inflate(&strm, Z_NO_FLUSH);

		inflateEnd(&strm);

		fwrite(out, 1, decLen, file);
		printverbose("z");
		return decLen;
	}
	else
#endif
	{
		fwrite(buffer, 1, len, file);
		printverbose(".");
		return len;
	}
}

uint16_t readAndCompress(FILE *file, uint8_t *dataBuf, uint16_t pos, uint16_t ucDataLen)
{
	uint8_t in[DATA_BLOCKSIZE + 6];
	uint8_t out[DATA_BLOCKSIZE + 6];
	uint16_t have;

	ucDataLen = fread(dataBuf + pos, 1, ucDataLen, file);
#if defined USE_ZLIB
	// So now we have to check if zlib can compress this or not
	z_stream strm;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	deflateInit(&strm, Z_DEFAULT_COMPRESSION);

	strm.avail_in = ucDataLen;
	memcpy(in, dataBuf + pos, ucDataLen);
	strm.next_in = in;

	have = 0;

	strm.avail_out = ucDataLen;
	strm.next_out = out + have;

	deflate(&strm, Z_FINISH);
	have = ucDataLen - strm.avail_out;

	deflateEnd(&strm);

	if (have < ucDataLen)
	{
		memcpy(dataBuf + pos, out, have);
//		printverbose("c");
	}
	else
#endif
	{
		have = ucDataLen;
//		printverbose("r");
	}
	printverbose(".");

	if ((ucDataLen != DATA_BLOCKSIZE))  // Last block of compressed partition was readr
	{
		printverbose("\n");  // Terminate progress bar
	}
	return have;
}

uint16_t insertint16_t(uint8_t *dataBuf, uint16_t pos, uint16_t value)
{
	dataBuf[pos] = value >> 8;
	dataBuf[pos + 1] = value & 0xFF;
	return 2;
}

uint32_t insertint32_t(uint8_t *dataBuf, uint16_t pos, uint32_t value)
{
	dataBuf[pos] = value >> 24;
	dataBuf[pos + 1] = value >> 16;
	dataBuf[pos + 2] = value >> 8;
	dataBuf[pos + 3] = value & 0xFF;
	return 2;
}

int32_t writeBlock(FILE *irdFile, FILE *file, uint8_t firstBlock, uint16_t type)
{
	uint32_t resellerId;
	uint16_t blockHeaderPos = 0;
	uint16_t nextBlockHeaderPos = 0;
	uint16_t cDataLen = 0;
	uint16_t ucDataLen = DATA_BLOCKSIZE;
	uint16_t dataCrc = 0;
	uint8_t *blockHeader = (uint8_t *)malloc(4);
	uint8_t *dataBuf = (uint8_t *)malloc(DATA_BLOCKSIZE + 4);

	if (firstBlock && type == 0x10)
	{  // header
		resellerId = 0x230200A0;  // Octagon SF 1028P HD Noblence L6.00

		insertint16_t(dataBuf, 0, type);
		insertint32_t(dataBuf, 2, resellerId);
		insertint16_t(dataBuf, 6, 0);
		insertint16_t(dataBuf, 8, 0xFFFF);
		insertint16_t(dataBuf, 10, 0);
		insertint16_t(dataBuf, 12, 0);  // version 1
		insertint16_t(dataBuf, 14, 0);  // version 2
		cDataLen = 12;
		ucDataLen = cDataLen;
	}
	else
	{
		cDataLen = readAndCompress(file, dataBuf, 4, ucDataLen);
		insertint16_t(dataBuf, 0, type);
		insertint16_t(dataBuf, 2, ucDataLen);
#if !defined USE_ZLIB
		ucDataLen = cDataLen;
#endif
	}
	dataCrc = crc16(dataCrc, dataBuf, cDataLen + 4);

	insertint16_t(blockHeader, 0, cDataLen + 6);
	insertint16_t(blockHeader, 2, dataCrc);
	fwrite(blockHeader, 1, 4, irdFile);
	fwrite(dataBuf, 1, cDataLen + 4, irdFile);

	free(blockHeader);
	free(dataBuf);
	return ucDataLen;
}

int32_t getGeneration(int32_t resellerID)
{
	int32_t generation;
	int32_t temp;

	temp = resellerID >> 24;  // get 1st resellerID byte
	switch (temp)
	{
		case 0x20:  // FS9000, FS9200, HS9510
		{
			generation = 1;
			break;
		}
		case 0x23:  // HS8200
		case 0x25:  // HS7110, HS7420, HS7810A
		{
			if (resellerID & 0xf0 == 0xa0)
			{
				generation = 2;  // loader 6.XX
			}
			else
			{
				generation = 1;  // loader 5.XX
			}
			break;
		}
		case 0x27:  // HS7119, HS7429, HS7819
		{
			generation = 3;  // loader 7.XX
			break;
		}
		case 0x29:  // DP2010, DP6010, DP7000, DP7001, DP7050
		case 0x2a:  // EP8000, EPP8000, GPV8000
		{
			generation = 4;  // loader 8.XX or X.0.X
			break;
		}
		default:
		{
			generation = -1;
			break;
		}
	}
	return generation;
}

char *getModelName(int32_t resellerID)
{
	int32_t i;
	int32_t generation;

	for (i = 0; i < sizeof(fortis_names); i++)
	{
		if (fortis_names[i].resellerID == resellerID)
		{
			break;
		}
	}
	if (i == sizeof(fortis_names))
	{
		generation = getGeneration(resellerID);
		if (generation == 2 || generation == 3)
		{
			resellerID |= 0xA0;
			for (i = 0; i < sizeof(fortis_names); i++)
			{
				if (fortis_names[i].resellerID == resellerID)
				{
					break;
				}
			}
		}
	}
	if (i == sizeof(fortis_names))
	{
		return (char *)"Unknown model";
	}
	return (char *)fortis_names[i].reseller_name;
}

int32_t readBlock(FILE *file, char *name, uint8_t firstBlock)
{
	uint16_t blockCounter = 0;
	uint16_t len = 0;
	uint16_t decLen;
	uint16_t crc_16;
	uint16_t dataCrc = 0;
	uint16_t type = 0;
	char nameOut[64];
	uint8_t dataBuf[DATA_BLOCKSIZE + 6];
	uint16_t fpVersion;
	uint16_t systemId1;
	uint16_t systemId2;
	uint32_t systemId;
	uint32_t SWVersion0;
	uint16_t SWVersion1;
	uint16_t SWVersion2;
	uint16_t generation;
	struct tPartition *partTable;
	char *modelName;
	uint32_t stringLen;
	uint16_t loaderFound;
	uint16_t blockCount1;
	uint16_t blockCount2;
	uint32_t blockCount;

	len = readShort(file);  // get length of block
	if (len < 1)
	{
		return 0;
	}
	crc_16 = readShort(file);  // get block CRC in file

	if (fread(dataBuf, 1, len - 2, file) != len - 2)  // get block data
	{
		return 0;
	}
	dataCrc = 0;
	dataCrc = crc16(dataCrc, dataBuf, len - 2);  // get actual block CRC

	if (crc_16 != dataCrc)
	{
		printf("\nCRC data error occurred in block #%d (type %d)!\n", blockCounter, type);
//		getchar();
		return -1;
	}
	type = extractShort(dataBuf, 0);
	blockCounter++;

	if (firstBlock && (type == 0x10))
	{
//		if (verbose == 1)
//		{
//			printf("-> header\n");
//		}
		fpVersion = extractShort(dataBuf, 0);
		systemId1 = extractShort(dataBuf, 2);
		systemId2 = extractShort(dataBuf, 4);
		systemId = (systemId1 << 16) + systemId2;

		blockCount1 = extractShort(dataBuf, 6);
		blockCount2 = extractShort(dataBuf, 8);
		blockCount = (blockCount1 << 16) + blockCount2;

		SWVersion1 = extractShort(dataBuf, 12);
		SWVersion2 = extractShort(dataBuf, 14);

		if (verbose == 1)
		{
			modelName = getModelName(systemId);
			printf("\n Header data:\n");
			printf("  fpVersion       : %X\n", fpVersion);
			printf("  Reseller ID     : %08X (%s)\n", systemId, modelName);
			printf("  # of blocks     : %d (0x%0X) blocks\n", blockCount, blockCount);
			printf("  SoftwareVersion : V%X.%02X.%02X\n", SWVersion1, SWVersion2 >> 8, SWVersion2 & 0xFF);
		}

		generation = getGeneration(systemId);

		switch (generation)
		{
			case 1:
			{
				partTable = partData1;
				break;
			}
			case 2:
			{
				switch (systemId2 & 0xf0)  // get 4th reseller byte, less lower nibble
				{
					case 0:  // loader 5.XX
					{
						if ((systemId1 >> 8) == 0x25)
						{
							partTable = partData2a;  // HS7110, HS7420, HS7810A
						}
						else
						{
							partTable = partData2b;  // HS8200
						}
						break;
					}
					case 0xA0:  // loader 6.XX
					{
						if ((systemId1 >> 8) == 0x25)
						{
							partTable = partData2c;  // HS7110, HS7420, HS7810A
						}
						else
						{
							partTable = partData2d;  // HS8200
						}
						break;
					}
				}
			}
			case 3:
			{
				partTable = partData3;
				break;
			}
			case 4:
			{
				partTable = partData4;
				break;
			}
			default:
			{
				printf("\nCAUTION: Unknown receiver model detected (probably an SD model).\n");
				partTable = partData1;
				break;
			}
		}
	}
	else
	{  // not header block but normal partition block
		loaderFound = 0;
		if (type < MAX_PART_NUMBER)
		{
			if (!has[type])
			{
				has[type] = 1;
				if (verbose == 1)
				{
					printf("\nPartition found, type %02X -> %s (%s, %s", type, partTable[type].Description, partTable[type].Extension2, partTable[type].FStype);
					if (partTable[type].Flags & PART_SIGN)
					{
						printf(", signed");
					}
					printf(")\n");
				}
				if (type == 0x00)
				{
					loaderFound = 1;
				}

				/* Build output file name */
				stringLen = strlen(name);
				strncpy(nameOut, name, stringLen);  // get basic output file name
				nameOut[stringLen] = '\0';
				stringLen = strlen(nameOut);
				strncpy(nameOut + stringLen, partTable[type].Extension, strlen(partTable[type].Extension));
				stringLen += strlen(partTable[type].Extension);
				nameOut[stringLen] = '\0';
				strncpy(nameOut + stringLen, ".", 1);
				stringLen++;
				nameOut[stringLen] = '\0';
				strncpy(nameOut + stringLen, partTable[type].Extension2, strlen(partTable[type].Extension2));
				stringLen += strlen(partTable[type].Extension2);
				nameOut[stringLen] = '\0';
				if (verbose)
				{
					printf("Writing partition data to file %s\n", nameOut);
				}
				fd[type] = fopen(nameOut, "wb");

				if (fd[type] == NULL)
				{
					printf("\nError opening output file %s\n", nameOut);
					return -1;
				}
			}
			decLen = extractShort(dataBuf, 2);
			extractAndWrite(fd[type], dataBuf + 4, len - 6, decLen);
		}
		else
		{
			printf("\nERROR: Illegal partition type %02X found, quitting...\n", type);
			// getchar();
			return -1;
		}
	}
	return len;
}

void createDummy(void)
{
	FILE *file;

	// Check if the file dummy.squash.signed.padded exists. If not, create it
	file = fopen("dummy.squash.signed.padded", "rb");
	if (file == NULL)
	{
		printverbose("\nSigned dummy squashfs headerfile does not exist.\n");
		printverbose("Creating it...");

		file = fopen("dummy.squash.signed.padded", "wb");
		printverbose(".");

		fwrite(dummy, 1, dummy_size, file);
		printverbose(".");

		fclose(file);
		printverbose(".");
		file = fopen("dummy.squash.signed.padded", "rb");
		printverbose(".");

		if (file != NULL)
		{
			printverbose("\n\nCreating signed dummy squashfs header file successfully completed.\n");
		}
		else
		{
			printf("\nError: Could not write signed dummy squashfs header file.\n");
			remove("dummy.squash.signed.padded");
		}
	}
	else
	{
		printverbose("Signed dummy squashfs headerfile already exists, doing nothing\n");
	}
}

void deleteDummy(void)
{
	FILE *file;

	file = fopen("dummy.squash.signed.padded", "rb");
	if (file != NULL)
	{
		fclose(file);
		remove("dummy.squash.signed.padded");
	}
}


int32_t main(int32_t argc, char* argv[])
{
//	pos = 0;

	if ((argc == 2 && strlen(argv[1]) == 2 && strncmp(argv[1], "-d", 2) == 0)
	|| (argc == 2 && strlen(argv[1]) == 3 && strncmp(argv[1], "-dv", 3) == 0))  // force create dummy
	{
		if (strncmp(argv[1], "-dv", 3) == 0)
		{
			verbose = 1;
		}
		else
		{
			verbose = 0;
		}
		createDummy();
	}
	else if ((argc == 3 && strlen(argv[1]) == 2 && strncmp(argv[1], "-s", 2) == 0)
	     ||  (argc == 3 && strlen(argv[1]) == 3 && strncmp(argv[1], "-sv", 3) == 0))  // sign squashfs part
	{
		uint32_t crc = 0;
		char signedFileName[128];
		strcpy(signedFileName, argv[2]);
		strcat(signedFileName, ".signed");
		uint8_t buffer[DATA_BUFFER];
		int32_t count;
		FILE *infile;
		FILE *signedFile;

		if (strncmp(argv[1], "-sv", 3) == 0)
		{
			verbose = 1;
		}
		else
		{
			verbose = 0;
		}
		signedFile = fopen(signedFileName, "wb");
		infile = fopen(argv[2], "r");
		if (infile == NULL)
		{
			printf("Error while opening input file %s\n", argv[2]);
			return -1;
		}

		if (signedFile == NULL)
		{
			printf("Error while opening output file %s\n", argv[3]);
			return -1;
		}
		while (!feof(infile))
		{
			count = fread(buffer, 1, DATA_BUFFER, infile);  // Actually it would be enough to fseek and only to read q byte.
			fwrite(buffer, 1, count, signedFile);
			crc = crc32(crc, buffer, 1);
		}

		if (verbose == 1)
		{
			printf("Signature in footer: 0x%08x\n", crc);
			printf("Output file name is: %s\n", signedFileName);
		}
		fwrite(&crc, 1, 4, signedFile);

		fclose(infile);
		fclose(signedFile);
	}
	else if ((argc == 3 && strlen(argv[1]) == 2 && strncmp(argv[1], "-t", 2) == 0)
	     ||  (argc == 3 && strlen(argv[1]) == 3 && strncmp(argv[1], "-tv", 3) == 0))  // test signed squashfs part
	{
		uint32_t crc = 0;
		uint32_t orgcrc = 0;
		uint8_t buffer[DATA_BUFFER];
		FILE *file;

		if (strncmp(argv[1], "-tv", 3) == 0)
		{
			verbose = 1;
		}
		else
		{
			verbose = 0;
		}

		file = fopen(argv[2], "r");
		if (file == NULL)
		{
			printf("Error while opening input file %s\n", argv[2]);
			return -1;
		}

		while (!feof(file))
		{  // Actually we should remove the signature at the end
			int32_t count = fread(buffer, 1, DATA_BUFFER, file);
			if (count != DATA_BUFFER)
			{
				orgcrc = (buffer[count - 1] << 24) + (buffer[count - 2] << 16) + (buffer[count - 3] << 8) + (buffer[count - 4]);
			}
			crc = crc32(crc, buffer, 1);
		}
		fclose(file);

		if (verbose == 1)
		{
			printf("Correct signature: 0x%08x\n", crc);
			printf("Signature in file: 0x%08x\n", orgcrc);
		}
		else
		{
			if (crc != orgcrc)
			{
				printf("Signature is wrong, correct: 0x%08x, found in file: 0x%08x.\n", crc, orgcrc);
				return -1;
			}
		}
	}
	else if ((argc == 3 && strlen(argv[1]) == 2 && strncmp(argv[1], "-x", 2) == 0)
	     ||  (argc == 3 && strlen(argv[1]) == 3 && strncmp(argv[1], "-xv", 3) == 0))  // extract IRD into composing parts
	{
		int32_t i;
		int32_t pos = 0;
		uint16_t len = 0;
		uint8_t firstBlock;
		FILE *file;

		if (strncmp(argv[1], "-xv", 3) == 0)
		{
			verbose = 1;
		}
		else
		{
			verbose = 0;
		}

		for (i = 0; i < MAX_PART_NUMBER; i++)
		{
			has[i] = 0;
			fd[i] = NULL;
		}

		file = fopen(argv[2], "r");
		if (file == NULL)
		{
			printf("Error while opening input file %s\n", argv[2]);
			return -1;
		}
		firstBlock = 1;

		while (!feof(file))
		{
			pos = ftell(file);
			len = readBlock(file, argv[2], firstBlock);
			firstBlock = 0;
			if (len < 0)
			{
				fclose(file);
				return -1;  // error ocurred
			}
			if (len > 0)
			{
				pos += len + 2;
				fseek(file, pos, SEEK_SET);
			}
			else
			{
				break;
			}
		}
		fclose(file);

		for (i = 0; i < MAX_PART_NUMBER; i++)
		{
			if (fd[i] != NULL)
			{
				fclose(fd[i]);
			}
		}
		printverbose("\n");
		for (i = 0; i < MAX_PART_NUMBER; i++)
		{
			if (has[i] > 1)
			{
				printf("CAUTION, unusual condition: partition type %d occurs %d times.\n", i, has[i]);
			}
		}
		if (verbose)
		{
			printf("Extracting IRD file %s succesfully completed.\n", argv[2]);
		}
		verbose = 1;
	}
	else if (argc == 2 && strlen(argv[1]) == 2 && strncmp(argv[1], "-v", 2) == 0)  // print version info
	{
		printf("Version: %s  Date: %s\n", VERSION, DATE);
	}
	else if (argc >= 3 && strlen(argv[1]) == 2 && strncmp(argv[1], "-c", 2) == 0)  // create Fortis IRD
	{
		int32_t i;
		uint16_t totalBlockCount;
		uint16_t dataCrc = 0;
		uint16_t type;
		uint8_t *dataBuf;
		uint16_t headerDataBlockLen;
		uint8_t appendPartCount;
		uint16_t partBlockcount;
		FILE *infile;
		FILE *irdFile;

		irdFile = fopen(argv[2], "wb+");
		if (irdFile == NULL)
		{
			printf("Error while opening output file %s\n", argv[2]);
			return -1;
		}

		totalBlockCount = 0;
		headerDataBlockLen = 0;

		// Header
		headerDataBlockLen = writeBlock(irdFile, NULL, 1, 0x10);
		headerDataBlockLen += 4;

		appendPartCount = argc;
		// search for -v
		verbose = 0;
		for (i = 3; i < argc; i++)
		{
			if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-v", 2)) == 0)
			{
				verbose = 1;
			}
		}
		appendPartCount = appendPartCount - verbose - 3;

		if (appendPartCount == 0)
		{
			printf("\nError: No input files for output file %s specified.\n", argv[2]);
			fclose(irdFile);
			remove(argv[2]);
			return -1;
		}

		// evaluate suboptions
		for (i = 3; i < appendPartCount + 3 + verbose; i += 2)
		{
			type = 0x99;

			if (strlen(argv[i]) == 3 && (strncmp(argv[i], "-ll", 3) && strncmp(argv[i], "-00", 3)) == 0)
			{
				type = 0x00;
			}
			else if (strlen(argv[i]) == 13 && (strncmp(argv[i], "-feelinglucky", 13)) == 0)
			{
				type = 0x00;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-a", 2) && strncmp(argv[i], "-1", 2)) == 0)
			{
				type = 0x01;
			}
			else if (strlen(argv[i]) == 3 && (strncmp(argv[i], "-c0", 3)) == 0)
			{
				type = 0x02;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-2", 2)) == 0)
			{
				type = 0x02;
			}
			else if (strlen(argv[i]) == 3 && (strncmp(argv[i], "-c4", 3)) == 0)
			{
				type = 0x03;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-3", 2)) == 0)
			{
				type = 0x03;
			}
			else if (strlen(argv[i]) == 3 && (strncmp(argv[i], "-c8", 3)) == 0)
			{
				type = 0x04;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-4", 2)) == 0)
			{
				type = 0x04;
			}
			else if (strlen(argv[i]) == 3 && (strncmp(argv[i], "-ca", 3)) == 0)
			{
				type = 0x05;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-5", 2)) == 0)
			{
				type = 0x05;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-k", 2) && strncmp(argv[i], "-6", 2)) == 0)
			{
				type = 0x06;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-d", 2) && strncmp(argv[i], "-7", 2)) == 0)
			{
				type = 0x07;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-r", 2) && strncmp(argv[i], "-8", 2)) == 0)
			{
				type = 0x08;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-u", 2) && strncmp(argv[i], "-9", 2)) == 0)
			{
				type = 0x09;
			}
#if 0
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-i", 2)) == 0)
			{
				type = 0x81;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-s", 2)) == 0)
			{
				type = 0x82;
			}
#endif
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-v", 2)) == 0)
			{
				type = 0x88;
				i--;
			}

			if (type == 0x99)
			{
				printf("Unknown suboption %s.\n", argv[3 + i]);
				fclose(irdFile);
				irdFile = fopen(argv[2], "rb");
				if (irdFile != NULL)
				{
					fclose (irdFile);
					remove(argv[2]);
				}
				return -1;
			}

#if 0
			if (type == 0x81)
			{
				if (((appendPartCount) % 2 == 1) && (argc == i + 1))
				{
					printf("\nError: ResellerID (4 or 8 digits) for suboption %s not specified.\n", argv[i]);
					return -1;
				}
				else
				{
					printf("Insert resellerID %s.\n", argv[1 + i]);
				}
			}

			if (type == 0x82)
			{
				if (((appendPartCount) % 2 == 1) && (argc == i + 1))
				{
					printf("\nError: Software version for suboption %s not specified.\n", argv[i]);
					return -1;
				}
				else
				{
					printf("Insert softwareversion %s.\n", argv[1 + i]);
				}
			}
#endif
			if (type != 0x88)
			{
				if (((appendPartCount) % 2 == 1) && (argc == i + 1))
				{
					printf("\nError: Input file name for suboption %s not specified.\n", argv[i]);
					return -1;
				}

				infile = fopen(argv[i + 1], "rb");
				if (infile == NULL)
				{
					printf("Error opening input file %s\n", argv[i + 1]);
					return -1;
				}

				if (verbose)
				{
					printf("Adding type %d block, file: %s\n", type, argv[i + 1]);
				}
				partBlockcount = totalBlockCount;
				while (writeBlock(irdFile, infile, 0, type) == DATA_BLOCKSIZE)
				{
					totalBlockCount++;
//					printverbose("w");
					printverbose(".");
				}
				totalBlockCount++;
				partBlockcount = totalBlockCount - partBlockcount;
				if (verbose)
				{
					printf("Added %d blocks, total is now %d blocks\n", partBlockcount, totalBlockCount);
				}
				fclose(infile);
			}
		}
		// Refresh Header
		dataBuf = (uint8_t *)malloc(headerDataBlockLen);

		// Read Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fread(dataBuf, 1, headerDataBlockLen, irdFile);

		// Update Blockcount
		insertint16_t(dataBuf, 8, totalBlockCount);

		// Rewrite Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fwrite(dataBuf, 1, headerDataBlockLen, irdFile);

		// Update CRC
		dataCrc = crc16(0, dataBuf, headerDataBlockLen);
		insertint16_t(dataBuf, 0, dataCrc);

		// Rewrite CRC
		fseek(irdFile, 0x02, SEEK_SET);
		fwrite(dataBuf, 1, 2, irdFile);

		free(dataBuf);
		fclose(irdFile);

		if (verbose)
		{
			printf("Creating IRD file %s succesfully completed.\n", argv[2]);
		}
		verbose = 1;
	}
	else if (argc >= 3 && strlen(argv[1]) == 3 && strncmp(argv[1], "-ce", 3) == 0)  // Create Enigma2 IRD
	{
		int32_t i;
		uint16_t type;
		uint16_t dataCrc = 0;
		uint16_t totalBlockCount;
		uint16_t headerDataBlockLen;
		uint8_t appendPartCount;
		uint16_t partBlockcount;
		uint8_t oldsquash = 0;
		uint8_t *dataBuf;
		FILE *file;
		FILE *irdFile;

		verbose = 0;
		irdFile = fopen(argv[2], "wb+");
		if (irdFile == NULL)
		{
			printf("Error while opening output file %s\n", argv[2]);
			return -1;
		}

		totalBlockCount = 0;
		headerDataBlockLen = 0;

		// Header
		headerDataBlockLen = writeBlock(irdFile, NULL, 1, 0x10);
		headerDataBlockLen += 4;

		appendPartCount = argc;
		// search for -v and -1G
		for (i = 3; i < argc; i++)
		{
			if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-v", 2)) == 0)
			{
				verbose = 1;
			}
		}
		for (i = 3; i < argc; i++)
		{
			if (strlen(argv[i]) == 3 && (strncmp(argv[i], "-1G", 3)) == 0)
			{
				oldsquash = 1;
			}
		}

		appendPartCount = argc - verbose - oldsquash - 3;

		if (appendPartCount == 0)
		{
			printf("\nError: No input files for output file %s specified.\n",argv[2]);
			fclose(irdFile);
			remove(argv[2]);
			return -1;
		}

		for (i = 3; i < appendPartCount + 3 + verbose + oldsquash; i += 2)
		{
			type = 0x99;

			if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-f", 2) && strncmp(argv[i], "-1", 2) == 0))
			{  // Original APP, now FW
				type = 0x01;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-k", 2) && strncmp(argv[i], "-6", 2)) == 0)
			{  // KERNEL
				type = 0x06;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-e", 2) && strncmp(argv[i], "-8", 2)) == 0)
			{  // Original ROOT, now EXT
				type = 0x08;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-g", 2) && strncmp(argv[i], "-7", 2)) == 0)
			{  // Original DEV, now G
				type = 0x07;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-r", 2) && strncmp(argv[i], "-9", 2)) == 0)
			{  // Original USER, now ROOT
				type = 0x09;
			}
			else if (strlen(argv[i]) == 2 && strncmp(argv[i], "-2", 2) == 0)
			{
				type = 0x02;
			}
			else if (strlen(argv[i]) == 2 && strncmp(argv[i], "-3", 2) == 0)
			{
				type = 0x03;
			}
			else if (strlen(argv[i]) == 2 && strncmp(argv[i], "-4", 2) == 0)
			{
				type = 0x04;
			}
			else if (strlen(argv[i]) == 2 && strncmp(argv[i], "-5", 2) == 0)
			{
				type = 0x05;
			}
			else if (strlen(argv[i]) == 3 && strncmp(argv[i], "-1G", 3) == 0)
			{
				type = 0x88;
				i--;
			}
			else if (strlen(argv[i]) == 2 && strncmp(argv[i], "-v", 2) == 0)
			{
				type = 0x88;
				i--;
			}

			if (type == 0x99)
			{
				printf("Unknown suboption %s.\n", argv[i]);
				fclose(irdFile);
				irdFile = fopen(argv[2], "rb");
				if (irdFile != NULL)
				{
					fclose (irdFile);
					remove(argv[2]);
				}
				return -1;
			}
			if (type != 0x88)
			{
				if (verbose) 
				{
					printf("\nNew partition, type %02X\n", type);
				}
//				printf("\ntype = %02X, appendPartcount = %d, argc = %d, i = %d, argv[%d] = [%s]\n",type,appendPartCount,argc,i,i,argv[i]);

				if (((appendPartCount) % 2 == 1) && (argc == i + 1))
				{
					printf("\nError: Input file name for option %s not specified.\n",argv[i]);
					return -1;
				}
				if (type == 0x01 || type == 0x08 || type== 0x07) // these must be signed squashfs
				{
					printverbose("Adding signed dummy squashfs3.0 header");
					if (oldsquash == 1)
					{
						// Check if the file dummy.squash30.signed.padded exists. If not, create it
						file = fopen("dummy.squash30.signed.padded", "rb");
						if (file == NULL)
						{
							printverbose("\nSigned dummy squashfs 3.0 headerfile does not exist.\n");
							printverbose("Creating it...");

							file = fopen("dummy.squash30.signed.padded", "w");
							printverbose(".");

							fwrite(dummy30, 1, dummy30_size, file);
							printverbose(".");

							fclose(file);
							printverbose(".");

							file = fopen("dummy.squash30.signed.padded", "rb");
							printverbose(".");
							if (file == NULL)
							{
								printf("\n\nCould not write signed dummy squashfs3.0 header file.\n");
								remove("dummy.squash30.signed.padded");
								return -1;
							}
							else
							{
							printverbose("\nCreating signed dummy squashfs3.0 header file successfully completed.\n");
							}
						}
					}
					else  // new squashfs dummy
					{
						createDummy();  //  Create dummy squashfs header
						printverbose("Adding signed dummy squashfs header");
						file = fopen("dummy.squash.signed.padded", "rb");
						printverbose(".");
					}

					if (file != NULL)
					{
						while (writeBlock(irdFile, file, 0, type) == DATA_BLOCKSIZE)
						{
							printverbose(".");
							totalBlockCount++;
						}
						totalBlockCount++;
						fclose(file);
// 						printverbose("Dummy squashfs header written to output file.\n");
					}
					else
					{
						printf("\nCould not read signed dummy squashfs header file.\n");
						remove("dummy.squash.signed.padded");
						return -1;
					}
				}  // squash header added, test if something more to add to this partition
				if ((strlen(argv[i + 1]) == 3 && strncmp(argv[i + 1], "foo", 3) == 0)
				&&  (strlen(argv[i + 1]) == 5 && strncmp(argv[i + 1], "dummy", 5) == 0))
				{
					printverbose("This is a foo partition (squashfs dummy header only).\n");
				}
				else
				{  // append input file
					file = fopen(argv[i + 1], "rb");
					if (file != NULL)
					{
						if (verbose)
						{
							printf("Adding type %d block, file: %s\n", type, argv[i + 1]);
						}
						partBlockcount = totalBlockCount;
						while (writeBlock(irdFile, file, 0, type) == DATA_BLOCKSIZE)
						{
							totalBlockCount++;
							printverbose(".");
						}
						totalBlockCount++;
						partBlockcount = totalBlockCount - partBlockcount;
						if (verbose)
						{
							printf("\nAdded %d blocks, total is now %d blocks\n", partBlockcount, totalBlockCount);
						}
						fclose(file);
					}
					else
					{
						printf("\nCould not append input file %s\n", argv[i + 1]);
						printf("\n");
					}
				}
			}
		}
		// Refresh Header
		dataBuf = (uint8_t *)malloc(headerDataBlockLen);

		// Read Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fread(dataBuf, 1, headerDataBlockLen, irdFile);

		// Update Blockcount
		insertint16_t(dataBuf, 8, totalBlockCount);

		// Rewrite Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fwrite(dataBuf, 1, headerDataBlockLen, irdFile);

		// Update CRC
		dataCrc = crc16(0, dataBuf, headerDataBlockLen);
		insertint16_t(dataBuf, 0, dataCrc);

		// Rewrite CRC
		fseek(irdFile, 0x02, SEEK_SET);
		fwrite(dataBuf, 1, 2, irdFile);

		free(dataBuf);

		fclose(irdFile);
		if (verbose)
		{
			printf("Creating IRD file %s succesfully completed.\n", argv[2]);
		}
		deleteDummy();
		if (oldsquash)
		{
			file = fopen("dummy.squash30.signed.padded", "rb");
			if (file != NULL)
			{
				fclose(file);
				remove("dummy.squash30.signed.padded");
//				printverbose("File dummy.squash30.signed.padded deleted.\n");
			}
			else
			{
				printf("Error: removing file dummy.squash30.signed.padded failed.\n");
			}
		}
		oldsquash = 0;
		verbose = 1;
	}
	else if ((argc == 4 && strlen(argv[1]) == 2 && strncmp(argv[1], "-r", 2) == 0)
	     ||  (argc == 4 && strlen(argv[1]) == 3 && strncmp(argv[1], "-rv", 3) == 0))  // Change reseller ID
	{
		uint32_t resellerId;
		uint16_t systemId1;
		uint16_t systemId2;
		uint32_t systemId;
		uint16_t dataCrc = 0;
		uint16_t headerDataBlockLen;
		uint8_t *dataBuf;
		FILE *irdFile;

		if (strncmp(argv[1], "-rv", 3) == 0)
		{
			verbose = 1;
		}
		else
		{
			verbose = 0;
		}

		headerDataBlockLen = 0;
		resellerId = 0;

		if (strlen(argv[3]) != 4 && strlen(argv[3]) != 8)
		{
			printf("Reseller ID must be 4 or 8 characters long.\n");
			return -1;
		}
		sscanf(argv[3], "%X", &resellerId);

		if (strlen(argv[3]) == 4)
		{
			resellerId = resellerId >> 16;
		}
		irdFile = fopen(argv[2], "r+");
		if (irdFile == NULL)
		{
			printf("Error while opening IRD file %s\n", argv[2]);
			return -1;
		}
		headerDataBlockLen = readShort(irdFile);
		headerDataBlockLen -= 2;

		// Refresh Header
		dataBuf = (uint8_t *)malloc(headerDataBlockLen);

		// Read Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fread(dataBuf, 1, headerDataBlockLen, irdFile);

		systemId1 = (dataBuf[2] << 8) + dataBuf[3];  // Get resellerID hi from file
		systemId = (systemId1 << 16) + (dataBuf[4] << 8) + dataBuf[5];  // resellerID lo from file

		// Update Reseller ID
		insertint32_t(dataBuf, 2, resellerId);

		// Rewrite Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fwrite(dataBuf, 1, headerDataBlockLen, irdFile);

		// Update CRC
		dataCrc = crc16(0, dataBuf, headerDataBlockLen);
		insertint16_t(dataBuf, 0, dataCrc);

		// Rewrite CRC
		fseek(irdFile, 0x02, SEEK_SET);
		fwrite(dataBuf, 1, 2, irdFile);

		free(dataBuf);

		fclose(irdFile);
		if (verbose)
		{
			printf("Changed reseller ID in file %s from %08X to %08X.\n", argv[2], systemId, resellerId);
		}
	}
	else if ((argc == 4 && strlen(argv[1]) == 2 && strncmp(argv[1], "-n", 2) == 0)
	     ||  (argc == 4 && strlen(argv[1]) == 3 && strncmp(argv[1], "-nv", 3) == 0))  // Change SW version number
	{
		uint32_t resellerId;
		uint32_t SWVersion;
		uint16_t SWVersion1;
		uint16_t SWVersion2;
		uint16_t dataCrc = 0;
		uint8_t *dataBuf;
		uint32_t temp;
		uint16_t headerDataBlockLen;
		FILE *irdFile;

		if (strncmp(argv[1], "-nv", 3) == 0)
		{
			verbose = 1;
		}
		else
		{
			verbose = 0;
		}
		headerDataBlockLen = 0;
		resellerId = 0;

		sscanf(argv[3], "%x", &SWVersion);
		SWVersion2 = SWVersion & 0x0000FFFF;  // Split the entire long version number into two words
		SWVersion1 = SWVersion >> 16;

		irdFile = fopen(argv[2], "r+");
		if (irdFile == NULL)
		{
			printf("Error while opening IRD file %s\n", argv[2]);
			return -1;
		}
		headerDataBlockLen = readShort(irdFile);
		headerDataBlockLen -= 2;

		// Refresh Header
		dataBuf = (uint8_t *)malloc(headerDataBlockLen);

		// Read Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fread(dataBuf, 1, headerDataBlockLen, irdFile);

		resellerId = extractShort(dataBuf, 12);  // Get SW version hi from file
		temp = extractShort(dataBuf, 14);  // Get SW version lo from file
		if (verbose)
		{
			printf("Current Software version number is V%X.%02X.%02X\n", resellerId & 0xFFFF, temp >> 8, temp & 0xFF);
			printf("Changing Software version number to V%X.%02X.%02X\n", SWVersion1 & 0xFFFF, SWVersion2 >> 8, SWVersion2 & 0xFF);
		}

		// Update Software version number
		insertint16_t(dataBuf, 12, (SWVersion1&0xFFFF));
		insertint16_t(dataBuf, 14, (SWVersion2&0xFFFF));

		// Rewrite Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fwrite(dataBuf, 1, headerDataBlockLen, irdFile);

		// Update CRC
		dataCrc = crc16(0, dataBuf, headerDataBlockLen);
		insertint16_t(dataBuf, 0, dataCrc);

		// Rewrite CRC
		fseek(irdFile, 0x02, SEEK_SET);
		fwrite(dataBuf, 1, 2, irdFile);

		free(dataBuf);

		fclose(irdFile);
	}
	else // show usage
	{
		printf("\n");
#ifdef USE_ZLIB
		printf("Version: %s  Date: %s\nUse ZLIB: yes\n", VERSION, DATE);
#else
		printf("Version: %s  Date: %s\n", VERSION, DATE);
#endif
		printf("\n");
		printf("Usage: %s -x|-xv|-c|-ce|-s|-sv|-d|-dv|-t|-tv|-r|-rv|-n|-nv|-v []\n", argv[0]);
		printf("       -x [update.ird]            Extract IRD\n");
		printf("       -xv [update.ird]           As -x, verbose\n");
		printf("       -c [update.ird] Options    Create Fortis IRD\n");
		printf("         Options for -c:\n");
		printf("          -ll [file.part]          Append Loader   (0) -> mtd0\n");
		printf("          -k  [file.part]          Append Kernel   (6) -> mtd1\n");
		printf(" s        -a  [file.part]          Append App      (1) -> mtd2\n");
		printf(" s        -r  [file.part]          Append Root     (8) -> mtd3\n");
		printf(" s        -d  [file.part]          Append Dev      (7) -> mdt4\n");
		printf("          -c0 [file.part]          Append Config0  (2) -> mtd5, offset 0\n");
		printf("          -c4 [file.part]          Append Config4  (3) -> mtd5, offset 0x40000\n");
		printf("          -c8 [file.part]          Append Config8  (4) -> mtd5, offset 0x80000\n");
		printf("          -ca [file.part]          Append ConfigA  (5) -> mtd5, offset 0xA0000\n");
		printf("          -u  [file.part]          Append User     (9) -> mtd6\n");
#if 0
		printf("          -i  [resellerID]         Set resellerID to argument\n");
		printf("          -s  [version]            Set SW version\n");
#endif
		printf("          -00 [file.part]          Append Type 0   (0) (alias for -ll)\n");
		printf("          -1  [file.part]          Append Type 1   (1) (alias for -a)\n");
		printf("          ...\n");
		printf("          -9  [file.part]          Append Type 9   (9) (alias for -u)\n");
		printf("          -v                       Verbose operation\n");
		printf("       -ce [update.ird] Options   Create Enigma2 IRD (obsolete, 1G models with TDT Maxiboot only\n");
		printf("         Options for -ce:\n");
		printf("          -k|-6 [file.part]        Append Kernel   (6) -> mtd1\n");
		printf("          -f|-1 [file.part]        Append FW       (1)\n");
		printf("          -r|-9 [file.part]        Append Root     (9)\n");
		printf("          -e|-8 [file.part]        Append Ext      (8)\n");
		printf("          -g|-7 [file.part]        Append G        (7)\n");
#if 0
		printf("          -i  [resellerID]         Set resellerID to argument\n");
		printf("          -s  [version]            Set SW version\n");
#endif
		printf("          -2    [file.part]        Append Config0  (2) -> mtd5, offset 0\n");
		printf("          -3    [file.part]        Append Config4  (3) -> mtd5, offset 0x40000\n");
		printf("          -4    [file.part]        Append Config8  (4) -> mtd5, offset 0x80000\n");
		printf("          -5    [file.part]        Append ConfigA  (5) -> mtd5, offset 0xA0000\n");
		printf("          -1G                      Use squashfs3.0 dummy\n");
		printf("          -v                       Verbose operation\n");
		printf("       -s [unsigned.squashfs]     Sign squashfs part\n");
		printf("       -sv [unsigned.squashfs]    Sign squashfs part, verbose\n");
		printf("       -d                         Create squashfs dummy file\n");
		printf("       -dv                        As -d, verbose\n");
		printf("       -t [signed.squashfs]       Test signed squashfs part\n");
		printf("       -tv [signed.squashfs]      Test signed squashfs part, verbose\n");
		printf("       -r [update.ird] id         Change reseller id (e.g. 230300A0 for Atevio AV7500 L6.00)\n");
		printf("       -rv [update.ird] id        As -r, verbose\n");
		printf("       -n [update.ird] versionnr  Change SW version number\n");
		printf("       -nv [update.ird] versionnr As -n, verbose\n");
		printf("       -v                         Display program version\n");
		printf("\n");
		printf("Note: To create squashfs part, use mksquashfs v3.3:\n");
		printf("      ./mksquashfs3.3 squashfs-root flash.rootfs.own.mtd8 -nopad -le\n");
		printf("\n");
		printf("Examples:\n");
		printf("  Creating a new Fortis IRD file with rootfs and kernel:\n");
		printf("   %s -c my.ird [-v] -r flash.rootfs.own.mtd8.signed -k uimage.mtd6\n", argv[0]);
		printf("\n");
		printf("  Extracting a IRD file:\n");
		printf("   %s -x[v] my.ird\n", argv[0]);
		printf("\n");
		printf("  Signing a squashfs partition:\n");
		printf("   %s -s[v] my.squashfs\n", argv[0]);
		return -1;
	}
	return 0;
}
// vim:ts=4
