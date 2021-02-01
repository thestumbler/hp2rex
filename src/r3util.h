typedef unsigned short ushort;
typedef unsigned char uchar;

#define REX_BLK_SIZE    1024
#define REX_MAX_BLKS    256

#define HEX_FOOD (0x0DF0)
#define HEX_DEAD (0xADDE)

typedef struct {
  ushort sig;
  short prev;
  short next;
  short size;
} HDR1K;

typedef union {
  HDR1K hdr;
  char  buf[REX_BLK_SIZE];
} REXBLK;

typedef struct {
  short this;
  short prev;
  short next;
  short size;
  long beg;
  long end;
} TREE;

#define DOW_MON 1
#define DOW_TUE 2
#define DOW_WED 3
#define DOW_THU 4
#define DOW_FRI 5
#define DOW_SAT 6
#define DOW_SUN 7

typedef struct {
	unsigned short	year;
	unsigned char	month;
	unsigned char	day;
	unsigned char	dow;
	unsigned char	hour;
	unsigned char	minute;
	unsigned char	second;
} DATETIME;

typedef struct {
	char	id[4];	/* 'BCO1' */
	char	unk[16];
	DATETIME	last_sync_date;
	char	did[8];
	DATETIME	sync_range_begin;
	DATETIME	sync_range_end;
	unsigned long	PREF_flat_ofs;
	unsigned long	ZONE_flat_ofs;
	unsigned long	TODO_flat_ofs;
	unsigned long	MEMO_flat_ofs;
	unsigned long	CONT_flat_ofs;
	unsigned long	CLDR_flat_ofs;
} BCO1;

typedef struct {
  char	id[4];	/* 'CLDR' */
  uchar	unk1;
  ushort nents;
  uchar unk2[14];
} CLDR_HDR;

typedef struct {
  ushort size;
  uchar  type;  /* | 0x20 means alarm */
  ushort seqno;
  uchar  unk1; /* usually 0x01 */
  uchar  rpt;  /* repeat flag - 0x80 */
  DATETIME beg;
  DATETIME end;
  uchar unk2[4];
  ushort alarmb4;
} CLDR_ENT;

typedef struct {
  uchar unk1[4];
  uchar rptmos; /* repeat every x months */
  uchar unk2[15];
  uchar rptdow;  /* DOW for weekly/monthly by day */
  uchar rptdate; /* DOW for monthly by date/1st/last */
  uchar unk3;
  uchar rptfl;   /* first/last of month 0x80/00 */
} CLDR_RPT;

/**** each entry:
 *  place, null terminated
 *  title, null terminated,
 *  notices, null terminated
 *  at the end, same entry size as before???
*****/

typedef struct {
  short ent1;   /* offset to first entry */
  char id[4];   /* always CONT */
  char unk1;    /* seems to be 0x01 always */
  short nentTotal; /* number of total contact entries */
  short nent[5];   /* number of contact entries each subset */
  short unk2[3];   /* 0 or 512, then 2, then 4 */
  short beg[3][5]; /* [0=last,1=first,2=comp][subseu:0-4] */
  short ix[3][5][27][2]; /* [0=last,1=first,2=comp][subset:0-4] */
  char unk5[2]; /* something fills here before data labels begin */
  uchar numbk2; /* number bank 2 attributes */
  short sslen; /* length of all 5 subset names in bytes */
} CONT_HDR;

typedef struct {
  short size;
  char subset; /* subset this belongs to 1-4 */
  short number; /* increases usually by 1, sometimes skips */
  short unk2; /* always 0x0001 ???  */
  uchar unk3[4]; /* often 8f-8e-87-27 */
  uchar dispnum;
  uchar flagnum;
  short bk1cnt;  /* bank 1 byte count */
  short bitfield;
  short prev[3][2];  /* sort order, all/subset */
  short next[3][2];  /* sort order, all/subset */
  short f3off[2];
} CONT_ENT;

#define CONT_HCTRY   (0x8000)
#define CONT_HZIP    (0x4000)
#define CONT_HSTATE  (0x2000)
#define CONT_HCITY   (0x1000)
#define CONT_HADDR2  (0x0800)
#define CONT_HADDR   (0x0400)
#define CONT_WCTRY   (0x0200)
#define CONT_WZIP    (0x0100)

#define CONT_WSTATE  (0x0080)
#define CONT_WCITY   (0x0040)
#define CONT_WADDR2  (0x0020)
#define CONT_WADDR   (0x0010)
#define CONT_TITLE   (0x0008)
#define CONT_COMP    (0x0004)
#define CONT_FIRST   (0x0002)
#define CONT_LAST    (0x0001)



typedef struct {
  char id[4];
  uchar unk1;
  ushort nent;
} TODO_HDR;


typedef struct {
  ushort size;
  uchar unk1[7];
  uchar checked;
  uchar iscall;
  DATETIME due;
} TODO_ENT;

/*****
 *after that,
   title, null terminated,
   at the end, entry size as at the beginning 
****/

typedef struct {
  char id[4];
  uchar unk1;
  ushort nents;
} MEMO_HDR;


typedef struct {
  ushort size;
  uchar unk1[8];
  uchar lentitle;
  ushort lenmemo;
} MEMO_ENT;


typedef struct {
  char id[4];
  uchar unk1[9];
  uchar dis2412; /* 0=12h, 1=24h */
  uchar disdate; /* 0="Jan 1,1997", 1="1 Jan,1997", 2="Wed 1/1/97" */
  uchar almsound; /* 0=off, 1=short, 2=long */
  uchar unk2;
  uchar unk3;
  uchar keyclick; /* 0=off, 1=on */
  uchar shutoff; /* 0=1m, 1=3m, 2=5m */
  uchar pwchk; /* 0=off, 1=30min, 2=60min, 3=always */
  uchar pw[5]; /* 1=HOME,2=VIEW,3=SEL,4=UP,5=DOWN */
  DATETIME unk4;
  uchar unk5[12];
} PREF_HDR;
/*****
 *  after that, follows null terminated:
 *  if found, return to..... line terminator 0D 0A
*****/

typedef struct {
  char id[4];
  uchar unk1;
  uchar unk2;
  uchar unk3;
  short gmtoff;
  short winteroff;
  short summeroff;
  uchar dst_emon; /* DST ending time */
  uchar dst_eday; /* if zero, use next entry */
  uchar dst_edow;
  uchar dst_ehr;
  uchar dst_bmon; /* DST beging time */
  uchar dst_bday; /* if zero, use next entry */
  uchar dst_bdow;
  uchar dst_bhr;
  char locname[32];
} ZONE_HDR;


int getrexblk( FILE *frex, int blkno, char *buf );
int putrexblk( FILE *frex, int blkno, int ofs, int size, char *buf );
int putrexbytes( FILE *frex, long addr, long size, void *ptr );
int saveCONTentry( FILE *frex, long flataddr );
int mktree( FILE *fprex, FILE *frexout );
int loadCONThdr( FILE *frex );
int dispCONTentries( FILE *frex );
long getCONTflat( FILE *frex, int num );
int loadCONTentry( FILE *frex, long flataddr );
int traverseCONTentries( FILE *frex, FILE *frexout );
void dateprint( FILE *fp, DATETIME *dt );
void printBCO1( FILE *fp, BCO1 *bco1 );
void printPREF( FILE *fp, PREF_HDR *pref );
void printCLDRHDR( FILE *fp, CLDR_HDR *ch );
int getrexbytes( FILE *fprex, long addr, long size, void *ptr );
int getrexstrn( FILE *frex, long addr, long maxsize, char *ptr );
char *getstrbufn( char *buf, int *max, char ** next, char *xtra );
void binaryprint( FILE*fp, char v );
int bitcount( char v );
char *getnextstr( char *buf, char *end );
int istext( char c );
void cls( void );
void flat2phys( long addrflat, int*block, int*offset );

/* Each REX RAM 1K "block" has an 8-byte header as follows:

	BYTE	Signature[2];	"F0 0D" for data block
				"DE AD" for empty/unused
	WORD	prev_blk;	"FF FF" means start of chain
	WORD	next_blk;	"FF FF" means end of chain
	WORD	n_data_bytes;	# "logical" data bytes this block


	The REX appears to think of the memory space as a "flat"
	*logical* memory space.  This means that it uses a logical-
	to-physical 1K block mapping table, *and* that "offsets" 
	to locations within this logical memory space do NOT include
	the 8-byte header.

	It is important to note that when the REX is addressing "flat"
	offsets into the memory space, that it is NOT just as simple
	as dividing the offset by (1024-sizeof(block hdr)) to 
	compute the logical block number and offset into that block.
	Each logical block may have less than (1024-sizeof(block hdr))
	"logical data bytes" in it, so you must look at each block's
	header and subtract its "n_data_bytes" value from the 
	"flat offset" until the "flat offset' value becomes smaller
	than the next block's "n_data_bytes" value.
*/

/*
	Calling "load_rex_mem_file()" will load (and possibly XOR-
	decode) a REX memory image file, generate the logical-to-
	physical mapping tables, and generate a new "flat" memory
	image (no block header info) called "Rex_flat_mem[]".  If
	a "BCO1" segment is found at offset ZERO into "Rex_flat_mem[]",
	the following pointers will also be initialized:

		P_BCO1_hdr	- Ptr to 'BCO1' segment
		P_PREF_hdr	- Ptr to 'PREF' segment
		P_ZONE_hdr	- Ptr to 'ZONE' segment
		P_TODO_hdr	- Ptr to 'TODO' segment
		P_MEMO_hdr	- Ptr to 'MEMO' segment
		P_CONT_hdr**	- Ptr to 'CONT' segment**
		P_CLDR_hdr	- Ptr to 'CLDR' segment

		**NOTE: For some reason, the offsets in the 'BCO1'
		segment point to two (or more) bytes *before* the
		'CONT' string for the 'CONT' segment - I don't
		know what the additional info before the 'CONT' 
		string is.  All other pointers should be pointing
		to the first character of their respective segment
		identifier.

*/
extern unsigned char	*P_BCO1_hdr;
extern unsigned char	*P_PREF_hdr;
extern unsigned char	*P_ZONE_hdr;
extern unsigned char	*P_TODO_hdr;
extern unsigned char	*P_MEMO_hdr;
extern unsigned char	*P_CONT_hdr;
extern unsigned char	*P_CLDR_hdr;

