/*****************************************************************************
 * Name :   fup                                                              *
 *          Management program for Fortis .ird flash files                   *
 *                                                                           *
 * Author:  Schischu, enhanced by Audioniek                                  *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program; if not, write to the Free Software               *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
 *                                                                           *
 *****************************************************************************
 *
 * + TODO: change loader reseller ID.
 * + TODO: -i for loader 6.XX: display actual memory usage
 *         in stead of "Variable".
 *
 * Changes in Version 1.9.6a:
 * + Fix 0x13 error: ird file CRC16 in header was wrong
 *   after -r(v) and -n(v).
 * + Default resellerId moved to fup.h.
 * + Corrected DP7050 info on -i.
 * + Total block count changed to 32 bit.
 * + -c & -ce: SW version number default is 1.00.00.
 *
 * Changes in Version 1.9.6:
 * + -i displays detailed ird file info.
 * + Fixed several errors introduced in 1.9.3 - 1.9.5
 * + define USE_ZLIB uncommented
 *
 * Changes in Version 1.9.5:
 * + -c and -ce accept subarguments -i (set resellerID) and
 *   -s (set software version).
 *
 * Changes in Version 1.9.4:
 * + -rv displays model names.
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
 * + Squashfs dummy file now padded with 0xFF in stead of 0x00; 0xFF
 *   is the erased state of flash memory.
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

#define VERSION "1.9.6a"
#define DATE "04.02.2020"

// Global variables
uint8_t verbose = 1;
char printstring;
uint8_t has[MAX_PART_NUMBER];
FILE *fd[MAX_PART_NUMBER];
uint16_t loaderFound;
uint32_t column;
uint8_t t_has[MAX_PART_NUMBER];
uint32_t partcount;

/* Functions */

void printVerbose(const char *printstring)
{
	if (verbose)
	{
		printf("%s", printstring);
	}
}

void printProgress(const char *printstring)
{
	if (verbose)
	{
		printf("%s", printstring);
		column++;
		if (column > 80)
		{
			column = 0;
			printf("\n");
		}
	}
}

#if 0  // not needed/used anywhere
void fromint16_t(uint8_t **int16_tBuf, uint16_t val)
{
	*int16_tBuf[0] = val >> 8;
	*int16_tBuf[1] = val & 0xFF;
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

int32_t extractAndWrite(FILE *file, uint8_t *buffer, uint16_t len, uint16_t decLen, uint8_t writeflag)
{
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

		if (writeflag)
		{
			fwrite(out, 1, decLen, file);
			printProgress("z");
		}
		return decLen;
	}
	else
	{
		if (writeflag)
		{
			fwrite(buffer, 1, len, file);
			printProgress(".");
		}
		return len;
	}
}

uint16_t readAndCompress(FILE *file, uint8_t **dataBuf, uint16_t pos, uint16_t *ucDataLen)
{
	uint8_t in[DATA_BLOCK_SIZE + 6];
	uint8_t out[DATA_BLOCK_SIZE + 6];
	z_stream strm;
	uint16_t have;

	*ucDataLen = fread((*dataBuf) + pos, 1, *ucDataLen, file);
	// So now we have to check if zlib can compress this or not

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	deflateInit(&strm, Z_DEFAULT_COMPRESSION);

	strm.avail_in = *ucDataLen;
	memcpy(in, (*dataBuf) + pos, *ucDataLen);
	strm.next_in = in;

	have = 0;

	strm.avail_out = *ucDataLen;
	strm.next_out = out + have;

	deflate(&strm, Z_FINISH);
	have = *ucDataLen - strm.avail_out;

	deflateEnd(&strm);

	if (have < *ucDataLen)  // if data was compressed
	{
		memcpy((*dataBuf) + pos, out, have);  // copy the compressed out buffer to dataBuf
		printProgress("z");
	}
	else
	{
		have = *ucDataLen;  // return uncompressed length & untouched dataBuf
		printProgress(".");
	}
	return have;
}

uint16_t insertint16_t(uint8_t ** dataBuf, uint16_t pos, uint16_t value)
{
	(*dataBuf)[pos] = value >> 8;
	(*dataBuf)[pos + 1] = value & 0xFF;
	return 2;
}

uint32_t insertint32_t(uint8_t ** dataBuf, uint16_t pos, uint32_t value)
{
	(*dataBuf)[pos]     = value >> 24;
	(*dataBuf)[pos + 1] = value >> 16;
	(*dataBuf)[pos + 2] = value >>  8;
	(*dataBuf)[pos + 3] = value & 0xFF;
	return 2;
}

void setHeader(FILE *irdFile, uint32_t headerDataBlockLen, uint8_t *dataBuf)
{
	uint16_t dataCrc;

	// Rewrite Header Data Block
	fseek(irdFile, 0x04, SEEK_SET);
	fwrite(dataBuf, 1, headerDataBlockLen, irdFile);

	// Update CRC
	dataCrc = crc16(0, dataBuf, headerDataBlockLen);
	insertint16_t(&dataBuf, 0, dataCrc);

	// Rewrite CRC
	fseek(irdFile, 0x02, SEEK_SET);
	fwrite(dataBuf, 1, 2, irdFile);

	free(dataBuf);
}

uint16_t getHeaderLen(FILE *irdFile)
{
	fseek(irdFile, 0x00, SEEK_SET);

	return readShort(irdFile);
}

uint8_t *getHeader(FILE *irdFile)
{
	uint16_t headerDataBlockLen;
	uint8_t *dataBuf;

	headerDataBlockLen = 0;
	headerDataBlockLen = readShort(irdFile);
	headerDataBlockLen -= 2;

	// Read Header Data Block
	dataBuf = (uint8_t *)malloc(headerDataBlockLen);
	fseek(irdFile, 0x04, SEEK_SET);  // 0x04 -> skip length & CRC
	fread(dataBuf, 1, headerDataBlockLen, irdFile);

	return dataBuf;
}

uint32_t getresellerID(FILE *irdFile)
{
	uint8_t *dataBuf;

	dataBuf = getHeader(irdFile);	
	return (((dataBuf[2] << 8) + dataBuf[3]) << 16) + (dataBuf[4] << 8) + dataBuf[5];  // resellerID from file
}

uint32_t getSWversion(FILE *irdFile)
{
	uint8_t *dataBuf;

	dataBuf = getHeader(irdFile);		
	return (((dataBuf[12] << 8) + dataBuf[13]) << 16) + (dataBuf[14] << 8) + dataBuf[15];  // SWversion from file
}

int32_t writeBlock(FILE *irdFile, FILE *inFile, uint8_t firstBlock, uint16_t type)
{
	uint32_t resellerId;
	uint16_t cDataLen = 0;
	uint16_t ucDataLen = DATA_BLOCK_SIZE;
	uint16_t dataCrc = 0;
	uint8_t *blockHeader = (uint8_t *)malloc(4);
	uint8_t *dataBuf = (uint8_t *)malloc(DATA_BLOCK_SIZE + 4);

//#if defined USE_ZLIB
	if (firstBlock && type == 0x10)
//#else
//	if (firstBlock && type == 0x01)
//#endif
	{  // header
	/**********************************************************************************************
	 *
	 * Loader definition of header block:
	 * Offset   Size      CRC  Name/purpose
	 * -------------------------------------------
	 *   0x00	uint16_t   N  length of header or normal block length
	 *   0x02   uint16_t   N  CRC16 of rest over block
	 *   0x04   uint16_t   Y  _xfdVer -> transfer format, 0x10 = compressed (type in data block) 
	 *   0x06   uint32_t   Y  _systemId -> resellerId
	 *   0x0a   uint32_t   Y  _nDataBlk -> number of blocks in file  
	 *   0x0e   uint32_t   Y  SWversion
	 *
	 **********************************************************************************************/
		resellerId = RESELLER_ID;

		insertint16_t(&dataBuf,  0, type);  // _xfdVer
		insertint32_t(&dataBuf,  2, resellerId);
		insertint16_t(&dataBuf,  6, 0);  // uncompressed length
		insertint32_t(&dataBuf,  8, 0xFFFFFFFF);  // block count
		insertint32_t(&dataBuf, 12, 0x00010000);  // SWversion, default 1.00.00
		cDataLen = 12;
		ucDataLen = cDataLen;  // quit after 1 (this) block
	}
	else
	{  // normal partition data block
	/**********************************************************************************************
	 *
	 * Loader definition of data block format:
	 * Offset   Size      CRC  Name/purpose
	 * -------------------------------------------
	 *   0x00   uint16_t   N   block length
	 *   0x02	uint16_t   N   block type (0x00 - 0x0f) 
	 *   0x04   uint16_t   N   data length (uncompressed length)
	 *   0x08   uint16_t   N   CRC16 over rest of block
	 *   0x0a   uint16_t   Y   compressed length 
	 *   0x0c-   uint8_t   Y   block data (compressed length bytes in total)
	 *
	 **********************************************************************************************/
		cDataLen = readAndCompress(inFile, &dataBuf, 4, &ucDataLen);
		insertint16_t(&dataBuf, 0, type);
		insertint16_t(&dataBuf, 2, ucDataLen);
	}
	dataCrc = crc16(dataCrc, dataBuf, cDataLen + 4);

	insertint16_t(&blockHeader, 0, cDataLen + 6);
	insertint16_t(&blockHeader, 2, dataCrc);
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
			if ((resellerID & 0xf0) == 0xa0)
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
		case 0x29:  // DP2010, DP6010, DP7000, DP7001, DP7050, GPV8000
		case 0x2a:  // EP8000, EPP8000
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

	for (i = 0; i < sizeof(fortisNames); i++)
	{
		if (fortisNames[i].resellerID == resellerID)
		{
			break;
		}
	}
	if (i == sizeof(fortisNames))  // if not found, retry for loader 6.XX
	{
		generation = getGeneration(resellerID);
		if (generation == 2 || generation == 3)
		{
			resellerID |= 0xA0;
			for (i = 0; i < sizeof(fortisNames); i++)
			{
				if (fortisNames[i].resellerID == resellerID)
				{
					break;
				}
			}
		}
	}
	if (i == sizeof(fortisNames))
	{
		return (char *)"Unknown model";
	}
	return (char *)fortisNames[i].reseller_name;
}

tPartition *getTableAddr(uint32_t generation, uint32_t resellerId)
{
	tPartition *partTable;

	switch (generation)
	{
		case 1:
		{
			partTable = partData1;
			break;
		}
		case 2:
		{
			switch (resellerId & 0xf0)  // get 4th reseller byte, less lower nibble
			{
				case 0:  // loader 5.XX
				{
					if ((resellerId >> 24) == 0x25)
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
					if ((resellerId >> 24) == 0x25)
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
			break;
		}
		case 3:
		{
			partTable = partData3;
			break;
		}
		case 4:
		{
			if (((resellerId >> 8) & 0xff) != 0x10)
			{
				partTable = partData4;
			}
			else
			{
				partTable = partData4a;  // DP7050
			}
			break;
		}
		default:
		{
			printVerbose("\nCAUTION: Unknown receiver model detected (probably an SD model).\n");
			partTable = partData1;
			break;
		}
	}
	return partTable;
}

/***********************************************************
 *
 * readBlock
 *
 * Read one block of data from input file *file.
 * If writeflag set, writes data into output file *name
 *
 * Returns: number of bytes written.
 *
 */
int32_t readBlock(FILE *file, char *name, uint8_t firstBlock, uint8_t writeflag)
{
	uint16_t blockCounter = 0;
	uint16_t len = 0;
	uint16_t decLen;
	uint16_t crc_16;
	uint16_t dataCrc = 0;
	uint16_t type = 0;
	char nameOut[64] = "";
	uint8_t dataBuf[DATA_BLOCK_SIZE + 6];
	uint16_t fpVersion;
	uint32_t blockCount;
	uint32_t systemId;
	uint32_t SWVersion0;
	uint16_t SWVersion1;
	uint16_t SWVersion2;
	uint16_t generation;
	char *modelName;
	struct tPartition *tableAddr;

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
		printf("\nERROR: CRC16 not correct in block #%d (type %d, %s).\n", blockCounter, type, (type == 0 ? "Header" : tableAddr[type].Description));
//		getchar();
		return -1;
	}
	type = extractShort(dataBuf, 0);
	blockCounter++;

//	if (firstBlock && ((type == 0x10) || type == 0x01))
	if (firstBlock && type == 0x10)
	{
		fpVersion = extractShort(dataBuf, 0);
		systemId = ((extractShort(dataBuf, 2)) << 16) + extractShort(dataBuf, 4);
		blockCount = ((extractShort(dataBuf, 6)) << 16) + extractShort(dataBuf, 8);
		SWVersion1 = extractShort(dataBuf, 12);
		SWVersion2 = extractShort(dataBuf, 14);

		if (verbose == 1)
		{
			modelName = getModelName(systemId);
			printf("\n Header data:\n");
			printf("  tfpVersion      : 0x%02X\n", fpVersion);
			printf("  Reseller ID     : %08X (%s)\n", systemId, modelName);
			printf("  # of blocks     : %d (0x%0X) blocks\n", blockCount, blockCount);
			printf("  SoftwareVersion : V%X.%02X.%02X\n", SWVersion1, SWVersion2 >> 8, SWVersion2 & 0xFF);
		}

		generation = getGeneration(systemId);
		tableAddr = getTableAddr(generation, systemId);
	}
	else
	{  // not header block but normal partition block
		loaderFound = 0;
		if (type < MAX_PART_NUMBER)
		{
			if (!has[type])
			{
//				has[type]++;
				has[type] = 1;
				column = 0;
				if (verbose == 1)
				{
					printf("\nPartition found, type %02X -> %s (%s, %s", type, tableAddr[type].Description, tableAddr[type].Extension2, tableAddr[type].FStype);
					if (tableAddr[type].Flags & PART_SIGN)
					{
						printf(", signed");
					}
					printf(")\n");
				}
				if (type == 0x00)
				{
					loaderFound = 1;
				}

				if (writeflag)
				{
					/* Build output file name */
					strncpy(nameOut, name, strlen(name));  // get basic output file name
					strcat(nameOut, tableAddr[type].Extension);
					strcat(nameOut, ".");
					strcat(nameOut, tableAddr[type].Extension2);

					fd[type] = fopen(nameOut, "wb");
	
					if (fd[type] == NULL)
					{
						printf("\nERROR: Cannot open output file %s.\n", nameOut);
						return -1;
					}
					if (verbose)
					{
						printf("Writing partition data to file %s\n", nameOut);
					}
				}
				else
				{
					t_has[partcount] = type;
					partcount++;
				}
			}
			decLen = extractShort(dataBuf, 2);
			extractAndWrite(fd[type], dataBuf + 4, len - 6, decLen, writeflag);
		}
		else
		{
			printf("\nERROR: Illegal partition type %02X found.\n", type);
			// getchar();
			return -1;
		}
	}
	return len;
}

void scanBlocks(FILE *irdFile)
{  // finds the types of partitions in irdFile and sets has[] accordingly.
	int32_t len;
	int32_t pos = 0;
	int32_t i, j;
	uint16_t *type = {0};
	uint8_t	firstBlock = 1;

	for (i = 0; i < MAX_PART_NUMBER; i++)
	{
		has[i] = 0;
	}
	verbose = 0;
	while (!feof(irdFile))
	{
		pos = ftell(irdFile);
		len = readBlock(irdFile, (char *)"dummy", firstBlock, 0);
		firstBlock = 0;
		if (len < 0)  // error ocurred
		{
			goto out;
		}
		if (len > 0)
		{
			pos += len + 2;
			fseek(irdFile, pos, SEEK_SET);
		}
		else
		{
			break;
		}
	}

out:
	fclose(irdFile);
}

void createDummy(void)
{
	FILE *file;

	// Check if the file dummy.squash.signed.padded exists. If not, create it
	file = fopen("dummy.squash.signed.padded", "rb");
	if (file == NULL)
	{
		printVerbose("\nSigned dummy squashfs headerfile does not exist.\n");
		printVerbose("Creating it...");
		column = 0;
		file = fopen("dummy.squash.signed.padded", "wb");
		printProgress(".");

		fwrite(dummy, 1, dummy_size, file);
		printProgress(".");

		fclose(file);
		printProgress(".");
		file = fopen("dummy.squash.signed.padded", "rb");
		printProgress(".");

		if (file != NULL)
		{
			printVerbose("\n\nCreating signed dummy squashfs header file successfully completed.\n");
		}
		else
		{
			printf("\nERROR: Could not write signed dummy squashfs header file.\n");
			remove("dummy.squash.signed.padded");
		}
	}
//	else
//	{
//		printVerbose("Signed dummy squashfs headerfile exists already, doing nothing\n");
//	}
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

void changeResellerID(FILE *irdFile, uint32_t resellerId)
{
	uint32_t systemId;
	uint16_t headerDataBlockLen;
	uint8_t *dataBuf;

	headerDataBlockLen = getHeaderLen(irdFile);
	dataBuf = getHeader(irdFile);

	systemId = (dataBuf[2] << 24) + (dataBuf[3] << 16) + (dataBuf[4] << 8) + dataBuf[5];  // resellerID from file

	// Update Reseller ID
	insertint32_t(&dataBuf, 2, resellerId);

	setHeader(irdFile, headerDataBlockLen - 2, dataBuf);

	if (verbose)
	{
		printf("Changed reseller ID\n");
		printf("from %08X (%s)\n", systemId, getModelName(systemId));
		printf("  to %08X (%s).\n", resellerId, getModelName(resellerId));
	}
}

void changeSWVersion(FILE *irdFile, uint32_t SWVersion)
{
	uint32_t oldSWversion1;
	uint32_t oldSWversion2;
	uint8_t *dataBuf;
	uint32_t headerDataBlockLen;

	headerDataBlockLen = getHeaderLen(irdFile);
	dataBuf = getHeader(irdFile);

	oldSWversion1 = extractShort(dataBuf, 12);  // Get current SW version hi from file
	oldSWversion2 = extractShort(dataBuf, 14);  // Get current SW version lo from file

	// Update Software version number
	insertint16_t(&dataBuf, 12, (SWVersion >> 16));
	insertint16_t(&dataBuf, 14, (SWVersion & 0x0000FFFF));

	setHeader(irdFile, headerDataBlockLen - 2, dataBuf);

	if (verbose)
	{
		printf("Changed software version number ");
		printf("from V%X.%02X.%02X ", oldSWversion1 & 0xFFFF, oldSWversion2 >> 8, oldSWversion2 & 0xFF);
		printf("to V%X.%02X.%02X\n", SWVersion >> 16, (SWVersion & 0x0000FFFF) >> 8, SWVersion & 0xFF);
	}
}


int32_t main(int32_t argc, char* argv[])
{
	if ((argc == 3 && strlen(argv[1]) == 2 && strncmp(argv[1], "-i", 2) == 0))
	{  // display file info
		uint32_t i, j;
		uint32_t resellerId;
		uint32_t SWVersion;
		uint16_t *present;
		uint8_t *dataBuffer;
		FILE *irdFile;
		uint16_t generation;
		tPartition *tableAddr;

		irdFile = fopen(argv[2], "r");
		if (irdFile == NULL)
		{
			printf("ERROR: cannot open input file %s.\n", argv[2]);
			return -1;
		}
		printf("\nInformation on flash file %s\n\n", argv[2]);
		printf("Headerinfo:\n");
		resellerId = getresellerID(irdFile);
		printf("  Reseller ID      : 0x%08X\n", resellerId);
		printf("  Reseller model   : %s\n", getModelName(resellerId));
		SWVersion = getSWversion(irdFile);
		printf("  Software version : V%X.%02X.%02X\n", SWVersion >> 16, (SWVersion & 0x0000FF00) >> 8, SWVersion & 0xFF);
		dataBuffer = getHeader(irdFile);
		i = ((extractShort(dataBuffer, 6)) << 16) + extractShort(dataBuffer, 8);  // block count 
		printf("  Number of blocks : %d (%04X)\n", i, i);
//#if not defined USE_ZLIB
//		j = extractShort(dataBuffer, 0) >> 1;
//		printf("  File size        : %d (Calculated from # of blocks)\n", ((i << 18) + j));
//#endif
		printf("\nPartitiondata (order as in file):\n");
		printf("  Type mtd  mtdname start      end        size       FS     flash signed\n");
		printf("  ======================================================================\n");
		fseek(irdFile, 0, SEEK_SET);
		partcount = 0;
		scanBlocks(irdFile);
		generation = getGeneration(resellerId);
		tableAddr = getTableAddr(generation, resellerId);

		for (i = 0; i < MAX_PART_NUMBER; i++)
		{
			if (t_has[i])
			{
				printf("    %d", t_has[i]);
				printf("  %s", tableAddr[t_has[i]].Extension2);
				printf(" %s", tableAddr[t_has[i]].Description);
				for (j = 0; j < (7 - strlen(tableAddr[t_has[i]].Description)); j++)
				{
					printf(" ");
				}
				// TODO: detailed loader 6.XX aspects
				if (generation != 2)
				{
					printf(" 0x%08X", tableAddr[t_has[i]].Offset);
					printf(" 0x%08X", tableAddr[t_has[i]].Offset + tableAddr[t_has[i]].Size - 1);
					printf(" 0x%08X", tableAddr[t_has[i]].Size);
				}
				else  // Loader 6.XX: variable offsets
				{
					if (tableAddr[t_has[i]].Offset == 0x7f)
					{
						printf(" Variable  ");
					}
					else
					{
						printf(" 0x%08X", tableAddr[t_has[i]].Offset);
					}
					if (tableAddr[t_has[i]].Size == 0x7f)
					{
						printf(" Variable  ");
						printf(" Variable  ");
					}
					else
					{
						printf(" 0x%08X", tableAddr[t_has[i]].Offset + tableAddr[t_has[i]].Size - 1);
						printf(" 0x%08X", tableAddr[t_has[i]].Size);
					}
				}
				printf(" %s", tableAddr[t_has[i]].FStype);
				for (j = 0; j < (8 - strlen(tableAddr[t_has[i]].FStype)); j++)
				{
					printf(" ");
				}
				printf(" %s", (tableAddr[t_has[i]].Flags & PART_FLASH) ? "Y" : "N");
				printf("     %s", (tableAddr[t_has[i]].Flags & PART_SIGN) ? "Y" : "N");
				printf("\n");
			}
		}
	}
	else if ((argc == 2 && strlen(argv[1]) == 2 && strncmp(argv[1], "-d", 2) == 0)
	     ||  (argc == 2 && strlen(argv[1]) == 3 && strncmp(argv[1], "-dv", 3) == 0))  // force create dummy
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
		uint8_t buffer[DATA_BUFFER_SIZE];
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

		infile = fopen(argv[2], "r");
		if (infile == NULL)
		{
			printf("ERROR: cannot open input file %s.\n", argv[2]);
			return -1;
		}

		strcpy(signedFileName, argv[2]);
		strcat(signedFileName, ".signed");
		signedFile = fopen(signedFileName, "wb");
		if (signedFile == NULL)
		{
			printf("ERROR: cannot open output file %s.\n", argv[3]);
			return -1;
		}

		while (!feof(infile))
		{
			count = fread(buffer, 1, DATA_BUFFER_SIZE, infile);  // Actually it would be enough to fseek and only to read q byte.
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
	     ||  (argc == 3 && strlen(argv[1]) == 3 && strncmp(argv[1], "-tv", 3) == 0))  // check signed squashfs part signature
	{
		uint32_t crc = 0;
		uint32_t orgcrc = 0;
		uint8_t buffer[DATA_BUFFER_SIZE];
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
			printf("ERROR: cannot open input file %s.\n", argv[2]);
			return -1;
		}

		while (!feof(file))
		{  // Actually we should remove the signature at the end
			int32_t count = fread(buffer, 1, DATA_BUFFER_SIZE, file);
			if (count != DATA_BUFFER_SIZE)
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
			printf("ERROR: cannot open input file %s.\n", argv[2]);
			return -1;
		}
		firstBlock = 1;

		while (!feof(file))
		{
			pos = ftell(file);
			len = readBlock(file, argv[2], firstBlock, 1);
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
		printVerbose("\n");
//		for (i = 0; i < MAX_PART_NUMBER; i++)
//		{
//			if (has[i] > 1)
//			{
//				printf("CAUTION, unusual condition: partition type %d occurs %d times.\n", i, has[i]);
//			}
//		}
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
		uint32_t resellerId;
		uint32_t SWVersion;
		uint16_t headerDataBlockLen;
		uint32_t totalBlockCount;
		uint16_t dataCrc = 0;
		uint16_t type;
		uint8_t *dataBuf;
		uint8_t appendPartCount;
		uint16_t partBlockcount;
		FILE *infile;
		FILE *irdFile;

		irdFile = fopen(argv[2], "wb+");
		if (irdFile == NULL)
		{
			printf("ERROR: cannot open output file %s.\n", argv[2]);
			return -1;
		}

		totalBlockCount = 0;
		headerDataBlockLen = 0;

		// construct header
//#if defined USE_ZLIB
		type = 0x10;
//#else
//		type = 0x01;
//#endif
		// Header
		headerDataBlockLen = writeBlock(irdFile, NULL, 1, type);  // input file = NULL, first block
		headerDataBlockLen += 4;  // allow for length and CRC

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
			printf("\nERROR: No input files for output file %s specified.\n", argv[2]);
			fclose(irdFile);
			remove(argv[2]);
			return -1;
		}
		if (verbose)
		{
			printf("Creating flash file %s\n", argv[2]);
		}

		// evaluate suboptions
		for (i = 3; i < appendPartCount + 3 + verbose; i += 2)
		{
			type = 0x99;

			if ((strlen(argv[i]) == 3 && (strncmp(argv[i], "-ll", 3) && strncmp(argv[i], "-00", 3)) == 0)
			||  (strlen(argv[i]) == 13 && (strncmp(argv[i], "-feelinglucky", 13)) == 0))
			{
				type = 0x00;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-a", 2) && strncmp(argv[i], "-1", 2)) == 0)
			{
				type = 0x01;
			}
			else if ((strlen(argv[i]) == 3 && (strncmp(argv[i], "-c0", 3)) == 0)
			||       (strlen(argv[i]) == 2 && (strncmp(argv[i], "-2", 2)) == 0))
			{
				type = 0x02;
			}
			else if ((strlen(argv[i]) == 3 && (strncmp(argv[i], "-c4", 3)) == 0)
			||       (strlen(argv[i]) == 2 && (strncmp(argv[i], "-3", 2)) == 0))
			{
				type = 0x03;
			}
			else if ((strlen(argv[i]) == 3 && (strncmp(argv[i], "-c8", 3)) == 0)
			||       (strlen(argv[i]) == 2 && (strncmp(argv[i], "-4", 2)) == 0))
			{
				type = 0x04;
			}
			else if ((strlen(argv[i]) == 3 && (strncmp(argv[i], "-ca", 3)) == 0)
			||       (strlen(argv[i]) == 2 && (strncmp(argv[i], "-5", 2)) == 0))
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
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-i", 2)) == 0)
			{
				type = 0x81;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-s", 2)) == 0)
			{
				type = 0x82;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-v", 2)) == 0)
			{
				type = 0x88;
				i--;
			}

			if (type == 0x99)
			{
				printf("ERROR: Unknown suboption %s.\n", argv[3 + i]);
				fclose(irdFile);
				irdFile = fopen(argv[2], "rb");
				if (irdFile != NULL)
				{
					fclose (irdFile);
					remove(argv[2]);
				}
				return -1;
			}

			if (type == 0x81)  // handle resellerID
			{
				if (((appendPartCount) % 2 == 1) && (argc == i + 1))
				{
					printf("\nERROR: Reseller ID (4 or 8 digits) for suboption %s not specified.\n", argv[i]);
					return -1;
				}
				else
				{
					resellerId = 0;
					if (strlen(argv[i + 1]) != 4 && strlen(argv[i + 1]) != 8)
					{
						printf("ERROR: Reseller ID must be 4 or 8 characters long.\n");
						return -1;
					}
					sscanf(argv[i + 1], "%X", &resellerId);

					if (strlen(argv[i + 1]) == 4)
					{
						resellerId = resellerId >> 16;
					}
					changeResellerID(irdFile, resellerId);
				}
			}

			if (type == 0x82)  // handle SW version
			{
				if (((appendPartCount) % 2 == 1) && (argc == i + 1))
				{
					printf("\nERROR: Software version for suboption %s not specified.\n", argv[i]);
					return -1;
				}
				else
				{
					SWVersion = 0;
					sscanf(argv[i + 1], "%X", &SWVersion);

					changeSWVersion(irdFile, SWVersion);
				}
			}

			if ((type != 0x88) && (type != 0x81) && (type != 0x82))
			{
				if (((appendPartCount) % 2 == 1) && (argc == i + 1))
				{
					printf("\nERROR: Input file name for suboption %s not specified.\n", argv[i]);
					return -1;
				}

				infile = fopen(argv[i + 1], "rb");
				if (infile == NULL)
				{
					printf("ERROR: cannot open input file %s.\n", argv[i + 1]);
					return -1;
				}
				column = 0;
				if (verbose)
				{
					printf("Adding type %d block, file name: %s.\n", type, argv[i + 1]);
				}
				partBlockcount = totalBlockCount;
				while (writeBlock(irdFile, infile, 0, type) == DATA_BLOCK_SIZE)  // input file = infile, normal block
				{
					totalBlockCount++;
//					printProgress("w");
//					printProgress(".");
				}
				totalBlockCount++;
				partBlockcount = totalBlockCount - partBlockcount;
				if (verbose)
				{
					printf("\nAdded %d blocks, total is now %d blocks.\n", partBlockcount, totalBlockCount);
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
		insertint16_t(&dataBuf, 0x08, (int16_t)totalBlockCount & 0xFFFF);
		insertint16_t(&dataBuf, 0x0a, (int16_t)totalBlockCount >> 16);  // zero in practice

		setHeader(irdFile, headerDataBlockLen, dataBuf);

		if (verbose)
		{
			printf("Creating IRD file %s succesfully completed.\n", argv[2]);
		}
		verbose = 1;
	}
	else if (argc >= 3 && strlen(argv[1]) == 3 && strncmp(argv[1], "-ce", 3) == 0)  // Create Enigma2 IRD (for TDT Maxiboot loader, obsolete)
	{
		int32_t i;
		uint32_t resellerId;
		uint32_t SWVersion;
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
			printf("ERROR: cannot open output file %s.\n", argv[2]);
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
			printf("\nERROR: No input files for output file %s specified.\n",argv[2]);
			fclose(irdFile);
			remove(argv[2]);
			return -1;
		}
		if (verbose)
		{
			printf("Creating flash file %s\n", argv[2]);
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
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-i", 2)) == 0)
			{
				type = 0x81;
			}
			else if (strlen(argv[i]) == 2 && (strncmp(argv[i], "-s", 2)) == 0)
			{
				type = 0x82;
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
			if (type == 0x81)  // check resellerID option for argument
			{
				if (((appendPartCount) % 2 == 1) && (argc == i + 1))
				{
					printf("\nERROR: Reseller ID (4 or 8 digits) for suboption %s not specified.\n", argv[i]);
					return -1;
				}
				else
				{
					resellerId = 0;
					if (strlen(argv[i + 1]) != 4 && strlen(argv[i + 1]) != 8)
					{
						printf("ERROR: Reseller ID must be 4 or 8 characters long.\n");
						return -1;
					}
					sscanf(argv[i + 1], "%X", &resellerId);

					if (strlen(argv[i + 1]) == 4)
					{
						resellerId = resellerId >> 16;
					}

					changeResellerID(irdFile, resellerId);
				}
			}

			if (type == 0x82)  // check SW version option for argument
			{
				if (((appendPartCount) % 2 == 1) && (argc == i + 1))
				{
					printf("\nERROR: Software version for suboption %s not specified.\n", argv[i]);
					return -1;
				}
				else
				{
					SWVersion = 0;
					sscanf(argv[i + 1], "%X", &SWVersion);

					changeSWVersion(irdFile, SWVersion);
				}
			}

			if ((type != 0x88) && (type != 0x81) && (type != 0x82))
			{
				if (verbose) 
				{
					printf("\nNew partition, type %02X\n", type);
				}
				if (((appendPartCount) % 2 == 1) && (argc == i + 1))
				{
					printf("\nERROR: Input file name for option %s not specified.\n",argv[i]);
					return -1;
				}
// TODO: use tables
				if (type == 0x01 || type == 0x08 || type== 0x07)  // these must be signed squashfs
				{
					if (oldsquash == 1)
					{
						printVerbose("Adding signed dummy squashfs3.0 header");

						// Check if the file dummy.squash30.signed.padded exists. If not, create it
						file = fopen("dummy.squash30.signed.padded", "rb");
						if (file == NULL)
						{
							printVerbose("\nSigned dummy squashfs 3.0 headerfile does not exist.\n");
							printVerbose("Creating it...");

							file = fopen("dummy.squash30.signed.padded", "w");
							printProgress(".");

							fwrite(dummy30, 1, dummy30_size, file);
							printProgress(".");

							fclose(file);
							printProgress(".");

							file = fopen("dummy.squash30.signed.padded", "rb");
							printProgress(".");
							if (file == NULL)
							{
								printf("\n\nERROR: Could not write signed dummy squashfs3.0 header file.\n");
								remove("dummy.squash30.signed.padded");
								return -1;
							}
							else
							{
							printVerbose("\nCreating signed dummy squashfs3.0 header file successfully completed.\n");
							}
						}
					}
					else  // new squashfs dummy
					{
						createDummy();  //  Create dummy squashfs header
						printVerbose("Adding signed dummy squashfs header.");
						file = fopen("dummy.squash.signed.padded", "rb");
						printProgress(".");
					}

					if (file != NULL)
					{
						while (writeBlock(irdFile, file, 0, type) == DATA_BLOCK_SIZE)
						{
							printProgress(".");
							totalBlockCount++;
						}
						totalBlockCount++;
						fclose(file);
// 						printVerbose("Dummy squashfs header written to output file.\n");
					}
					else
					{
						printf("\nCould not read signed dummy squashfs header file.\n");
						remove("dummy.squash.signed.padded");
						return -1;
					}
				}  // squash header added, test if something more to add to this partition
				if ((strlen(argv[i + 1]) == 3 && strncmp(argv[i + 1], "foo", 3) == 0)
				||  (strlen(argv[i + 1]) == 5 && strncmp(argv[i + 1], "dummy", 5) == 0))
				{
					printVerbose("This is a foo partition (squashfs dummy header only).\n");
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
						while (writeBlock(irdFile, file, 0, type) == DATA_BLOCK_SIZE)
						{
							totalBlockCount++;
							printProgress(".");
						}
						totalBlockCount++;
						partBlockcount = totalBlockCount - partBlockcount;
						if (verbose)
						{
							printf("\nAdded %d blocks, total is now %d blocks.\n", partBlockcount, totalBlockCount);
						}
						fclose(file);
					}
					else
					{
						printf("\nERROR: cannot not append input file %s.\n", argv[i + 1]);
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
		insertint16_t(&dataBuf, 0x08, (int16_t)totalBlockCount & 0xFFFF);
		insertint16_t(&dataBuf, 0x0a, (int16_t)totalBlockCount >> 16);  // zero in practice

		setHeader(irdFile, headerDataBlockLen, dataBuf);

		if (verbose)
		{
			printf("Creating IRD file %s succesfully completed.\n", argv[2]);
		}
		if (oldsquash)
		{
			file = fopen("dummy.squash30.signed.padded", "rb");
			if (file != NULL)
			{
				fclose(file);
				remove("dummy.squash30.signed.padded");
//				printVerbose("File dummy.squash30.signed.padded deleted.\n");
			}
			else
			{
				printf("ERROR: Cannot delete file dummy.squash30.signed.padded.\n");
			}
		}
		else
		{
			deleteDummy();
		}
		oldsquash = 0;
		verbose = 1;
	}
	else if ((argc == 4 && strlen(argv[1]) == 2 && strncmp(argv[1], "-r", 2) == 0)
	     ||  (argc == 4 && strlen(argv[1]) == 3 && strncmp(argv[1], "-rv", 3) == 0))  // Change reseller ID
	{
		uint32_t resellerId;
		FILE *irdFile;

		if (strncmp(argv[1], "-rv", 3) == 0)
		{
			verbose = 1;
		}
		else
		{
			verbose = 0;
		}
		resellerId = 0;
		if (strlen(argv[3]) != 4 && strlen(argv[3]) != 8)
		{
			printf("ERROR: Reseller ID must be 4 or 8 characters long.\n");
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
			printf("EROOR: cannot open IRD file %s.\n", argv[2]);
			return -1;
		}
		changeResellerID(irdFile, resellerId);
		fclose(irdFile);
	}
	else if ((argc == 4 && strlen(argv[1]) == 2 && strncmp(argv[1], "-n", 2) == 0)
	     ||  (argc == 4 && strlen(argv[1]) == 3 && strncmp(argv[1], "-nv", 3) == 0))  // Change SW version number
	{
		uint32_t SWVersion;
		FILE *irdFile;

		if (strncmp(argv[1], "-nv", 3) == 0)
		{
			verbose = 1;
		}
		else
		{
			verbose = 0;
		}

		sscanf(argv[3], "%X", &SWVersion);

		irdFile = fopen(argv[2], "r+");
		if (irdFile == NULL)
		{
			printf("ERROR: cannot open IRD file %s.\n", argv[2]);
			return -1;
		}
		changeSWVersion(irdFile, SWVersion);
		fclose(irdFile);
	}
	else // show usage
	{
		printf("\nfup - management program for Fortis .ird flash files");
		printf("Version: %s  Date: %s\n", VERSION, DATE);
		printf("\n");
		printf("Usage: %s -i|-x|-xv|-c|-ce|-s|-sv|-t|-tv|-d|-dv|-r|-rv|-n|-nv|-v []\n", argv[0]);
		printf("       -i [update.ird]               Display detailed IRD information\n");
		printf("       -x [update.ird]               Extract IRD\n");
		printf("       -xv [update.ird]              As -x, verbose\n");
		printf("       -c [update.ird] Options       Create Fortis IRD\n");
		printf("         Suboptions for -c:   NOTE: lettered options only correct for 1G and 2G models)\n");
		printf("          -ll [file.part]             Append Loader   (0) -> mtd0\n");
		printf("          -k [file.part]              Append Kernel   (6) -> mtd1\n");
		printf(" s        -a [file.part]              Append App      (1) -> mtd2\n");
		printf(" s        -r [file.part]              Append Root     (8) -> mtd3\n");
		printf(" s        -d [file.part]              Append Dev      (7) -> mdt4\n");
		printf("          -c0 [file.part]             Append Config0  (2) -> mtd5, offset 0\n");
		printf("          -c4 [file.part]             Append Config4  (3) -> mtd5, offset 0x40000\n");
		printf("          -c8 [file.part]             Append Config8  (4) -> mtd5, offset 0x80000\n");
		printf("          -ca [file.part]             Append ConfigA  (5) -> mtd5, offset 0xA0000\n");
		printf("          -u [file.part]              Append User     (9) -> mtd6\n");
		printf("          -i [resellerID]             Set resellerID\n");
		printf("          -s [versionnr]              Set SW version\n");
		printf("          -00 [file.part]             Append Type 0   (0) (alias for -ll)\n");
		printf("          -1 [file.part]              Append Type 1   (1) (alias for -a)\n");
		printf("          ...\n");
		printf("          -9  [file.part]             Append Type 9   (9) (alias for -u)\n");
		printf("          -v                          Verbose operation\n");
		printf("       -ce [update.ird] Options      Create Enigma2 IRD (obsolete, models with TDT Maxiboot only)\n");
		printf("         Subtions for -ce:\n");
		printf("          -k|-6 [file.part]           Append Kernel   (6) -> mtd1\n");
		printf("          -f|-1 [file.part]           Append FW       (1)\n");
		printf("          -r|-9 [file.part]           Append Root     (9)\n");
		printf("          -e|-8 [file.part]           Append Ext      (8)\n");
		printf("          -g|-7 [file.part]           Append G        (7)\n");
		printf("          -i  [resellerID]            Set resellerID to argument\n");
		printf("          -s  [versionnr]             Set SW version\n");
		printf("          -2  [file.part]             Append Config0  (2) -> mtd5, offset 0\n");
		printf("          -3  [file.part]             Append Config4  (3) -> mtd5, offset 0x40000\n");
		printf("          -4  [file.part]             Append Config8  (4) -> mtd5, offset 0x80000\n");
		printf("          -5  [file.part]             Append ConfigA  (5) -> mtd5, offset 0xA0000\n");
		printf("          -1G                         Use squashfs3.0 dummy\n");
		printf("          -v                          Verbose operation\n");
		printf("       -s [unsigned.squashfs]        Sign squashfs part\n");
		printf("       -sv [unsigned.squashfs]       Sign squashfs part, verbose\n");
		printf("       -t [signed.squashfs]          Test signed squashfs part\n");
		printf("       -tv [signed.squashfs]         Test signed squashfs part, verbose\n");
		printf("       -d                            Create squashfs dummy file\n");
		printf("       -dv                           As -d, verbose\n");
		printf("       -r [update.ird] [resellerID]  Change reseller id (e.g. 230300A0 for Atevio AV7500 L6.00)\n");
		printf("       -rv [update.ird] [resellerID] As -r, verbose\n");
		printf("       -n [update.ird] [versionnr]   Change SW version number\n");
		printf("       -nv [update.ird] [versionnr]  As -n, verbose\n");
		printf("       -v                            Display program version\n");
		printf("\n");
		printf("Note: To create squashfs part, use mksquashfs v3.3:\n");
		printf("      ./mksquashfs3.3 squashfs-root flash.rootfs.own.mtd8 -nopad -le\n");
		printf("\n");
		printf("Examples:\n");
		printf("  Creating a Fortis IRD file with rootfs and kernel:\n");
		printf("   %s -c my.ird [-v] -1 flash.rootfs.own.mtd2.signed -6 uimage.mtd6\n", argv[0]);
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
