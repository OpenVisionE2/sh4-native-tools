#ifndef __FUP_H__
#define __FUP_H__

#define MAX_PART_NUMBER  0x0a    // highest possible partition number
#define EXTENSION_LEN    32      // maximum length of partition extension

#define PART_SIGN        1       // partition must be signed
#define PART_FLASH       2       // partition is flashable

#define DATA_BUFFER_SIZE 0x2000
#define DATA_BLOCK_SIZE  0x7FFA  // default block size (0x8000 - compressed length - block CRC16 - partition type)

/************************************************************
 *
 * The following tables describe the various partition
 * layouts used by the different generations of receivers
 * and boot loaders.
 * They are only used to display information and as file
 * extensions names.
 *
 */
struct tPartition
{
	char Extension[EXTENSION_LEN];
	char Extension2[EXTENSION_LEN];
	char Description[EXTENSION_LEN];
	uint32_t Offset;
	uint32_t Size;
	char FStype[EXTENSION_LEN];
	uint32_t Flags;
};

struct tPartition partData1[] =
{
	// 32MB flash (1st generation models, FS9000, FS9200, HS9510)
	{  ".loader", "mtd0",  "Loader", 0x00000000, 0x00300000, "binary", (PART_FLASH) },
	{     ".app", "mtd2",     "App", 0x00b00000, 0x00500000, "squash", (PART_FLASH | PART_SIGN) },
	{ ".config0", "mtd5", "Config0", 0x01b00000, 0x00040000, "binary", (PART_FLASH) },
	{ ".config4", "mtd5", "Config4", 0x01b40000, 0x00040000, "binary", (PART_FLASH) },
	{ ".config8", "mtd5", "Config8", 0x01b80000, 0x00020000, "binary", (PART_FLASH) },
	{ ".configA", "mtd5", "ConfigA", 0x01ba0000, 0x00020000, "binary", (PART_FLASH) },
	{  ".kernel", "mtd1",  "Kernel", 0x00300000, 0x00300000, "binary", (PART_FLASH) },
	{     ".dev", "mtd4",     "Dev", 0x01800000, 0x00300000, "squash", (PART_FLASH | PART_SIGN) },
	{  ".rootfs", "mtd3",    "Root", 0x01000000, 0x00800000, "squash", (PART_FLASH | PART_SIGN) },
	{    ".user", "mtd6",    "User", 0x01c00000, 0x00400000, "binary", (PART_FLASH) }
};

struct tPartition partData2a[] =
{
	// 32MB flash (2nd generation models, HS7110, HS7420, HS7810A, loader 5.XX)
	{  ".loader", "mtd0",  "Loader", 0x00000000, 0x00100000, "binary", (PART_FLASH) },
	{     ".app", "mtd2",     "App", 0x00b00000, 0x00500000, "squash", (PART_FLASH | PART_SIGN) },
	{ ".config0", "mtd5", "Config0", 0x01b00000, 0x00040000, "binary", (PART_FLASH) },
	{ ".config4", "mtd5", "Config4", 0x01b40000, 0x00040000, "binary", (PART_FLASH) },
	{ ".config8", "mtd5", "Config8", 0x01b80000, 0x00020000, "binary", (PART_FLASH) },
	{ ".configA", "mtd5", "ConfigA", 0x01ba0000, 0x00020000, "binary", (PART_FLASH) },
	{  ".kernel", "mtd1",  "Kernel", 0x00100000, 0x00200000, "binary", (PART_FLASH) },
	{     ".dev", "mtd4",     "Dev", 0x01800000, 0x00300000, "squash", (PART_FLASH | PART_SIGN) },
	{  ".rootfs", "mtd3",    "Root", 0x00d00000, 0x00800000, "squash", (PART_FLASH | PART_SIGN) },
	{    ".user", "mtd6",    "User", 0x01c00000, 0x00400000, "binary", (PART_FLASH) },
};

struct tPartition partData2b[] =
{
	// 64MB flash (HS8200, loader 5.0X)
	{  ".loader", "mtd0",  "Loader", 0x000000000, 0x00100000, "binary", (PART_FLASH) },
	{     ".app", "mtd2",     "App", 0x000b00000, 0x00700000, "squash", (PART_FLASH | PART_SIGN) },
	{ ".config0", "mtd5", "Config0", 0x002100000, 0x00040000, "binary", (PART_FLASH) },
	{ ".config4", "mtd5", "Config4", 0x002140000, 0x00040000, "binary", (PART_FLASH) },
	{ ".config8", "mtd5", "Config8", 0x002180000, 0x00020000, "binary", (PART_FLASH) },
	{ ".configA", "mtd5", "ConfigA", 0x0021a0000, 0x00020000, "binary", (PART_FLASH) },
	{  ".kernel", "mtd1",  "Kernel", 0x000100000, 0x00300000, "binary", (PART_FLASH) },
	{     ".dev", "mtd4",     "Dev", 0x001e00000, 0x00300000, "squash", (PART_FLASH | PART_SIGN) },
	{  ".rootfs", "mtd3",    "Root", 0x001200000, 0x00c00000, "squash", (PART_FLASH | PART_SIGN) },
	{    ".user", "mtd6",    "User", 0x002200000, 0x01e00000, "binary", (PART_FLASH) },
};

struct tPartition partData2c[] =
	{
	// 32MB flash (2nd generation models, HS7110, HS7810A, HS7810, loader 6.XX)
	{  ".loader", "mtd0",  "Loader", 0x000000000, 0x00030000, "binary", (PART_FLASH) },
	{     ".app", "mtd2",     "App", 0x00000007f, 0x0000007f, "squash", (PART_FLASH | PART_SIGN) },
	{ ".config0", "mtd5", "Config0", 0x001b00000, 0x00040000, "binary", (PART_FLASH) },
	{ ".config4", "mtd5", "Config4", 0x001b40000, 0x00040000, "binary", (PART_FLASH) },
	{ ".config8", "mtd5", "Config8", 0x001b80000, 0x00020000, "binary", (PART_FLASH) },
	{ ".configA", "mtd5", "ConfigA", 0x001ba0000, 0x00020000, "binary", (PART_FLASH) },
	{  ".kernel", "mtd1",  "Kernel", 0x000060000, 0x0000007f, "binary", (PART_FLASH) },
	{     ".dev", "mtd4",     "Dev", 0x00000007f, 0x0000007f, "squash", (PART_FLASH | PART_SIGN) },
	{  ".rootfs", "mtd3",    "Root", 0x00000007f, 0x0000007f, "squash", (PART_FLASH | PART_SIGN) },
	{    ".user", "mtd6",    "User", 0x001c00000, 0x00400000, "binary", (PART_FLASH) }
};

struct tPartition partData2d[] =
{
	// 64MB flash (HS8200, loader 6.00)
	{  ".loader", "mtd0",  "Loader", 0x000000000, 0x00060000, "binary", (PART_FLASH) },
	{     ".app", "mtd2",     "App", 0x00000007f, 0x0000007f, "squash", (PART_FLASH | PART_SIGN) },
	{ ".config0", "mtd5", "Config0", 0x002100000, 0x00040000, "binary", (PART_FLASH) },
	{ ".config4", "mtd5", "Config4", 0x002140000, 0x00040000, "binary", (PART_FLASH) },
	{ ".config8", "mtd5", "Config8", 0x002180000, 0x00020000, "binary", (PART_FLASH) },
	{ ".configA", "mtd5", "ConfigA", 0x0021a0000, 0x00020000, "binary", (PART_FLASH) },
	{  ".kernel", "mtd1",  "Kernel", 0x000600000, 0x0000007f, "binary", (PART_FLASH) },
	{     ".dev", "mtd4",     "Dev", 0x00000007f, 0x0000007f, "squash", (PART_FLASH | PART_SIGN) },
	{  ".rootfs", "mtd3",    "Root", 0x00000007f, 0x0000007f, "squash", (PART_FLASH | PART_SIGN) },
	{    ".user", "mtd6",    "User", 0x002200000, 0x01e00000, "binary", (PART_FLASH) }
};

struct tPartition partData3[] =
	{
	// 256MB flash (third generation, HS7119, HS7429, HS7819, loader 7.X0, 7.X6 or 7.X7)
	{  ".loader",   "mtd0",  "Loader", 0x00000000, 0x00400000, "binary", (PART_FLASH) },
	{  ".rootfs",   "mtd2",  "Rootfs", 0x00800000, 0x06000000, "UBI"   , (PART_FLASH) },
	{ ".config0",   "mtd5", "Config0", 0x00400000, 0x00040000, "binary", (PART_FLASH) }, // not writeable by IRD?
	{ ".config4",   "mtd5", "Config4", 0x00440000, 0x00040000, "binary", (PART_FLASH) }, // not writeable by IRD?
	{ ".config8",   "mtd5", "Config8", 0x00480000, 0x00020000, "binary", (PART_FLASH) }, // not writeable by IRD?
	{ ".configA",   "mtd5", "ConfigA", 0x004A0000, 0x00020000, "binary", (PART_FLASH) }, // not writeable by IRD?
	{  ".kernel",   "mtd1",  "Kernel", 0x00500000, 0x00300000, "binary", (PART_FLASH) },
	{  ".part.7", "Unused",  "Unused", 0x00000000, 0x00100000, "binary", 0 }, // not writeable by IRD
	{  ".part.8", "Unused",  "Unused", 0x00000000, 0x00000000, "binary", 0 }, // not writeable by IRD
	{    ".user",   "mtd6",    "User", 0x06800000, 0x09000000, "binary", 0 }
};

struct tPartition partData4[] =
{
	// 256MB flash (fourth generation, DP2010, Dp6010, DP7000, DP7001..., loader 8.X4, 8.X6 or 8.X7) # to be checked
	{ ".loader", "mtd0", "Loader", 0x00000000, 0x00100000, "binary", (PART_FLASH) },
	{ ".rootfs", "mtd2",   "Root", 0x00800000, 0x0b600000, "UBI"   , (PART_FLASH) },
	{ ".config", "mtd5", "Config", 0x08000000, 0x00100000, "binary", 0 }, // not writeable by IRD
	{ ".config", "mtd5", "Config", 0x08000000, 0x00100000, "binary", 0 }, // not writeable by IRD
	{ ".config", "mtd5", "Config", 0x08000000, 0x00100000, "binary", 0 }, // not writeable by IRD
	{ ".config", "mtd5", "Config", 0x08000000, 0x00100000, "binary", 0 }, // not writeable by IRD
	{ ".kernel", "mtd1", "Kernel", 0x00400000, 0x00400000, "binary", (PART_FLASH) },
	{ ".eeprom", "mtd4", "EEPROM", 0x00380000, 0x00080000, "binary", 0 }, // not writeable by IRD
	{   ".user", "mtd6",   "User", 0x00000000, 0x00000000, "binary", 0 }, // not writeable by IRD
	{   ".logo", "mtd3",   "Logo", 0x00180000, 0x00200000, "binary", (PART_FLASH) }
};

/************************************************************
 *
 * Simple table defining resellerID <-> reseller name
 *
 * Note: generation 2 has an A in digit 7, but is also
 *       checked for zero in this digit.
 *
 * (This table is almost certainly not complete).
 *
 */
struct model_name
{
	uint32_t resellerID;
	const char *reseller_name;
} fortisNames[] =
{
	{ 0x20000000, "Fortis FS-9000 HD PVR" },
	{ 0x20000100, "Fortis FS-9200 HD PVR" },
	{ 0x20000300, "Fortis HS-9510 HD PVR" },
	{ 0x20010000, "Rebox RE-9000HD PVR" },
	{ 0x20010100, "Rebox RE-8000HD PVR" },
	{ 0x20010300, "Rebox RE-4000HD PVR" },
	{ 0x20020000, "Octagon SF 1018P HD Alliance" },
	{ 0x20020300, "Octagon SF 1008P HD Intelligence" },
	{ 0x20020303, "Octagon SF 1008SE HD Intelligence" },
	{ 0x20030000, "HD Box FS-9300 HD" },
	{ 0x20030100, "HD Box FS-9200 HD" },
	{ 0x20030300, "HD Box FS-9100 HD" },
	{ 0x20040100, "Arcon Titan 2010 HD" },
	{ 0x20050000, "UltraPlus F-9000 HD" },
	{ 0x20050300, "UltraPlus 980 HD" },
	{ 0x20060000, "Openbox S8 HD PVR" },
	{ 0x20060100, "Openbox S7 HD PVR" },
	{ 0x20060302, "Openbox S5 HD PVR" },
	{ 0x20070000, "Tiviar F1 HD PVR" },
	{ 0x20070100, "Tiviar T1 HD PVR" },
	{ 0x20070300, "Tiviar S1 eco HD PVR" },
	{ 0x20080000, "Icecrypt S4000HDPVR" },
	{ 0x20090000, "Atevio AV 7000 HD PVR" },
	{ 0x20090300, "Atevio AV 700 HD" },
	{ 0x20110100, "Astro ASR 1200 Twin HD" },
	{ 0x20110300, "Astro ASR 1100 Single HD" },
	{ 0x20120100, "Dreamsky DSR-9700 HD PVR" },
	{ 0x20120302, "Dreamsky DSR-9600 HD PVR" },
	{ 0x20130000, "Skyway Diamond" },
	{ 0x20130100, "Skyway Platinum" },
	{ 0x20130302, "Skyway Classic" },
	{ 0x20140100, "I.Com Twin HD-9200 DIVX" },
	{ 0x20140101, "I.Com Twin HD-9200 DIVX II" },
	{ 0x20140300, "I.Com HD-9000 DIVX" },
	{ 0x20140602, "I.Com HD-9100 DIVX II" },
	{ 0x20140612, "Xspeed HD-9500 DIVX II" },
	{ 0x20140702, "I.Com HD-9000 DIVX II" },
	{ 0x20150300, "Optibox Koala HD" },
	{ 0x20160302, "XCruiser XDSR 400 HD" },
	{ 0x20170302, "VisionNet FS-9510 HD PVR" },
	{ 0x20180100, "Goldmaster HD-1090 PVR" },
	{ 0x20180302, "Goldmaster HD-1060,1070,1080 PVR" },
	{ 0x20190301, "Powersat PO-1000HD PVR" },
	{ 0x20210301, "O2 2012 HD" },
	{ 0x20210302, "Star Track SRT 2010 HD" },
	{ 0x20220302, "Forever HS-9510 HD PVR" },
	{ 0x20230000, "Dynavision DV-9000 HD PVR" },
	{ 0x20230100, "Dynavision DV-9200 HD PVR" },
	{ 0x20230301, "Dynavision DV-9600 HD PVR" },
	{ 0x20230302, "Dynavision DV-9500 HD PVR" },
	{ 0x20240602, "GI S8290 HD PVR" },
	{ 0x20250600, "Miraclebox 8 HD PVR" },
	{ 0x20260600, "Diginor 8800 HD" },
	{ 0x20270600, "Mediastar (Powers) HD 1200s" },
	{ 0x20280602, "MaxFly 9500 HD" },
	{ 0x20300302, "SuperBox SX 9518 HD" },
	{ 0x20310301, "Elcomax Elux I" },
	{ 0x22050001, "Icecrypt S1500 C" },
	{ 0x230000A0, "Fortis HX-8200 HD PVR" },
	{ 0x230100A0, "Rebox RE-8500HD PVR" },
	{ 0x230200A0, "Octagon SF 1028P HD Noblence" },
	{ 0x230300A0, "Atevio AV7500 HD PVR" },
	{ 0x230400A0, "Skyway Droid" },
	{ 0x230500A0, "Openbox S9 HD PVR" },
	{ 0x230600A0, "Icecrypt STC6000 HD PVR" },
	{ 0x230700A0, "Miraclebox 9 HD PVR" },
	{ 0x230900A0, "XCruiser XDSR 600 HD" },
	{ 0x231200A0, "UltraPlus X-9200 HD PVR" },
	{ 0x231500A0, "SkyS@t Royal HD PVR" },
	{ 0x231500A0, "Skytec Royal" },
	{ 0x231600A0, "SuperBox PRO HD 9818" },
	{ 0x231800A0, "Optibox Raptor" },
	{ 0x231900A0, "Forever HD-8200 PVR" },
	{ 0x232100A0, "Skytec Blackbox" },
	{ 0x232400A0, "Opticum Actus Duo" },
	{ 0x232700A0, "Mediastar (Powers) HD 8200s" },
	{ 0x250100A0, "Optibox Anaconda" },
	{ 0x250102A0, "Optibox Gekko" },
	{ 0x250112A0, "Optibox Gekko Cable" },
	{ 0x250200A0, "Octagon SF 1008 SE+ HD Intelligence" },
	{ 0x250202A0, "Octagon SF 918P SE+ HD Difference" },
	{ 0x250203A0, "Octagon SF 1008P SE+ HD Intelligence" },
	{ 0x250205A0, "Octagon SF 1008C SE+ HD Intelligence" },
	{ 0x250300A0, "Rebox RE-4200HD PVR" },
	{ 0x250302A0, "Rebox RE-2200HD PVR" },
	{ 0x250400A0, "UltraPlus 900 HD PVR" },
	{ 0x250500A0, "Skyway Nano" },
	{ 0x250502A0, "Skyway Light" },
	{ 0x250503A0, "Skyway Classic II" },
	{ 0x250600A0, "HD Box FS-9105 HD PVR" },
	{ 0x250602A0, "HD Box FS-7110 HD" },
	{ 0x250700A0, "Forever HD-7810 PVR, HD-7820 PVR" },
	{ 0x250703A0, "Forever HD-7420 PVR" },
	{ 0x250704A0, "Forever HD-7830 PVR" },
	{ 0x250800A0, "Arcon Titan 1010 HDTV" },
	{ 0x250900A0, "Openbox S6 HD PVR" },
	{ 0x250902A0, "Openbox S4 HD PVR" },
	{ 0x250903A0, "Openbox S6 PRO HD PVR" },
	{ 0x251000A0, "I.Com HD-1080P DIVX 2CI" },
	{ 0x251002A0, "I.Com HD-1070P DIVX" },
	{ 0x251004A0, "I.Com HD-1080P DIVX" },
	{ 0x251006A0, "I.Com HD-1090P DIVX" },
	{ 0x251100A0, "Goldmaster HD-1050 PVR" },
	{ 0x251102A0, "Goldmaster HD-1040,1045 PVR" },
	{ 0x251200A0, "Dynavision DV-7800 HD PVR" },
	{ 0x251202A0, "Dynavision DV-5000 HD PVR" },
	{ 0x251203A0, "Dynavision DV-8000 HD PVR" },
	{ 0x251300A0, "GI S8680" },
	{ 0x251302A0, "GI S8580" },
	{ 0x251303A0, "GI S8690" },
	{ 0x251403A0, "SuperBox PRO HD 9618" },
	{ 0x251700A0, "Elcomax Elux II" },
	{ 0x251703A0, "Elcomax Elux II Plus" },
	{ 0x251802A0, "XCruiser XDSR 380 HD" },
	{ 0x251803A0, "XCruiser XDSR 400 HD PLUS" },
	{ 0x251903A0, "Forsat FHD9100" },
	{ 0x251903A1, "Forsat FHD9200" },
	{ 0x251913A0, "Forsat FHD9000" },
	{ 0x252000A0, "Dreamsky HD6" },
	{ 0x252002A0, "Dreamsky HD4" },
	{ 0x252102A0, "Supermax 7100" },
	{ 0x252200A0, "SkyS@t Classic HD PVR" },
	{ 0x252202A0, "SkyS@t Mini HD PVR" },
	{ 0x252300A0, "Star Track SRT 2014 HD" },
	{ 0x252502A0, "Icecrypt S3500 HD CCI" },
	{ 0x252602A0, "Miraclebox 6 HD PVR" },
	{ 0x252607A0, "Miraclebox 7 HD PVR" },
	{ 0x252700A0, "Mediastar (Powers) HD 900s" },
	{ 0x252802A0, "Vegasat X1" },
	{ 0x252902A0, "Atemio AM 500/510 HD" },
	{ 0x253000A0, "KIOWA HD 98" },
	{ 0x253003A0, "KIOWA HD 108" },
	{ 0x253513A0, "Starsat 9900 HD" },
	{ 0x253533A0, "Mirage HD 8600 Plus" },
	{ 0x253613A0, "Drake 7500 HD" },
	{ 0x270100A0, "Octagon SF 918G SE+ HD Difference" },
	{ 0x270101A0, "Octagon SF 918CG SE+ HD Difference" },
	{ 0x270110A3, "Octagon SF 908G HD Difference" },
	{ 0x270120A0, "Octagon SF 1008G SE+ HD Intelligence" },
	{ 0x270130A0, "Octagon SF 1008G+ SE+ HD Intelligence" },
	{ 0x270200A0, "HD Box FS-7119" },
	{ 0x270220A0, "HD Box FS-9105 HD+" },
	{ 0x270300A0, "XCruiser XDSR 385 HD" },
	{ 0x270310A0, "XCruiser XDSR 380 HD Plus" },
	{ 0x270330A0, "XCruiser XDSR 420 HD" },
	{ 0x270330A1, "XCruiser XDSR 400 HD NAND" },
	{ 0x270331A1, "XCruiser XDSR 390 HD" },
	{ 0x270400A0, "Optibox Gekko Plus" },
	{ 0x270431A0, "Optibox Anaconda Plus" },
	{ 0x270600A0, "Skyway Nano 2" },
	{ 0x270631A0, "Skyway Classic III" },
	{ 0x270700A0, "Openbox S4 PRO Plus" },
	{ 0x270720A0, "Openbox S6 Plus" },
	{ 0x270730A0, "Openbox S6 PRO Plus" },
	{ 0x271000A0, "Icecrypt S3550 HD CCI" },
	{ 0x271020A0, "Icecrypt S3600 HD CCI" },
	{ 0x271200A0, "Miraclebox 6 Plus HD PVR" },
	{ 0x271400A0, "Rebox RE-2210HD PVR" },
	{ 0x271420A0, "Rebox RE-4210HD PVR" },
	{ 0x271500A0, "Forever Nano Smart" },
	{ 0x271520A0, "Forever HD 7819 Nano Pro" },
	{ 0x271530A0, "Forever HD 7474 PVR" },
	{ 0x271531A0, "Forever HD 7878 PVR" },
	{ 0x271600A0, "Dreamsky HD4+" },
	{ 0x271620A0, "Dreamsky HD6+" },
	{ 0x272400A0, "Skytec Jobi" },
	{ 0x272420A0, "Skytec Kleio" },
	{ 0x272431A0, "Skytec Nerine" },
	{ 0x272720A0, "Arcon Titan 1012 HDTV" },
	{ 0x272820A0, "Star Track 2014 Pro" },
	{ 0x272830A0, "Star Track 2014 Pro Plus" },
	{ 0x272900A0, "HD Box HB4000 Plus" },
	{ 0x272920A0, "HD Box 6000 Plus" },
	{ 0x272930A0, "HD Box HD6000 X Pro" },
	{ 0x273100A0, "Opticum Actus Mini" },
	{ 0x273120A0, "Opticum Actus Solo" },
	{ 0x29000000, "Fortis DS220" },
	{ 0x29010000, "XCruiser XDSR385HD Avant" },
	{ 0x29020000, "VisionNet Hawk" },
	{ 0x29030000, "Openbox SX4 HD" },
	{ 0x29040000, "Skyway Nano 3" },
	{ 0x29050000, "SuperBox Elite 4+" },
	{ 0x29060000, "Rebox RE-2220HD S-PVR" },
	{ 0x29080000, "Dreamsky HD4X" },
	{ 0x29090000, "Forever HD Nano Smart PVR Cardiff" },
	{ 0x29120000, "Drake 7500HD Mini" },
	{ 0x29130000, "HD Box 4500 plus" },
	{ 0x29140000, "Star Track Grand HD" },
	{ 0x29000100, "Fortis DS260" },
	{ 0x29060100, "Rebox RE-4220HD S-PVR" },
	{ 0x29130100, "HD Box 6500 plus" },
	{ 0x29140100, "Star Track DeLuxe HD" },
	{ 0x29000200, "Fortis DS260N" },
	{ 0x29030200, "Openbox SX6 HD" },
	{ 0x29040200, "Skyway Classic 4" },
	{ 0x29080200, "Dreamsky HD6 Duo" },
	{ 0x29090200, "Forever HD 9898 PVR Cardiff" },
	{ 0x29090300, "Forever HD 3434 PVR Cardiff" },
	{ 0x29120300, "Drake 7500HD-V3" },
	{ 0x29011000, "XCruiser XDSR2600HD Avant" },
	{ 0x29021000, "VisionNet Nova" },
	{ 0x29031000, "Openbox SX4 Base" },
	{ 0x29041000, "Skyway Light 2" },
	{ 0x29051000, "SuperBox Elite TV" },
	{ 0x29091000, "Forever HD 2424 PVR Cardiff" },
	{ 0x29131000, "HD Box 3500 plus" },
	{ 0x29141000, "Star Track SRT 2014 HD Premium" },
	{ 0x29171000, "Dynavision DV6000HDPVR" },
	{ 0x29042000, "Skyway Droid 2" },
	{ 0x29032000, "Openbox SX9 HD Combo" },
	{ 0x2A000000, "Fortis ESS300" },
	{ 0x2A020000, "Rebox RE-8220HD S-PVR" },
	{ 0x2A040000, "Openbox SX9 HD" },
	{ 0x2A050000, "Icecrypt S6600HDPVR" },
	{ 0x2A010100, "XCruiser XDSR420HD Avant" },
	{ 0x2A010101, "XCruiser XDSR400HD Avant" },
	{ 0x2A030101, "Proween STI-820HD Grand" }
};

#endif /* __FUP_H__ */
// vim:ts=4

