#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
// #include <dos.h>

#include "r3util.h"

#define SEQMAX (800)
#define CTMAX (800)
#define BS (0x08)

int TEST=1;

int PROC_CONT=0;
int PROC_MEMO1=0;
int PROC_MEMO2=0;
int PROC_CLDR=0;
int PROC_NUM=0;
int USE_INI=0;

/*********************************/
/*** CONTACT SECTION VARIABLES ***/
/*********************************/
typedef struct tagCDESC { /* used to maintain & sort contact lists */
  int seq;   /* this sequential entry number 0-based */
  struct tagCDESC *this;        /* This entry in list */
  struct tagCDESC *prev[3][2];  /* Prev entry in list */
  struct tagCDESC *next[3][2];  /* Next entry in list */
  char str[3][8];  /* first 7 bytes each of LAST, FIRST, COMPANY */
} CDESC;

CDESC *cl[3][5]; /* keeps up with start entry of each list */
int nents[3][5]; /* number entries in each list */
int nentsTotal;  /* total number of contacts */
CDESC *seq2cd[CTMAX]; /* converts sequence number to CDESC pointer */
CDESC cdesc; /* temporary CDESC storage */

char lbl[27];  /* holds index labels: #=0, A=1, B=2, Z=26 */
char *subs[5]; /* holds pointers to subset names */
char *flds[48]; /* holds pointers to field names */
int nflds, nsubs;

#define MAXSTR (1024)
char strings[1024];  /* block which holds string data for an entry */
int strings_size;    /* size of data in string block */

int ss=0; /* subset */
int so=0; /* sort order */
int al=0; /* alphabetical letter */

int aln, ali; /* no entries this alphabet letter, i-th entry */

#define MAXHPCF (36)
int nhpcf;
char hpfld[MAXHPCF][64];  /* contents of HP-200 Fields */
char r3fld[28][64];       /* contents of REX3 Fields */
char notes[MAXHPCF][72];  /* holds notes */
int nonotes, sizenotes;
int hp2rex[MAXHPCF];      /* maps hp fields to REX ones */
int l0,l1,l2;             /* size of first three REX fields */

/*********************************/
/***** MEMO SECTION VARIABLES ****/
/*********************************/
#define MAXMEMOS (64)
int imemo, nmemo, nmemo1, nmemo2;
int sizememo[MAXMEMOS];
int linememo[MAXMEMOS];
int titlmemo[MAXMEMOS];
int titlelen;
char title[128];

/**************************************/
/***** CLDR/APPT SECTION VARIABLES ****/
/**************************************/
int iappt, nappt;
char hpfield[256];
int hpstate;
int hpd, hpm, hpy;    /* date from HP APPT entry */
int hphhbeg, hpmmbeg, hpssbeg; /* time from HP APPT entry */
int hphhend, hpmmend, hpssend; /* time from HP APPT entry */
int hpdur, hplead;
char *hpc_title, *hpc_place, *hpc_notes;
/* HP Appt STATE variable bit definitions */
#define HP_APPT 0x80
#define HP_EVENT 0x20


/*** General Purpose Variables */
short BITMASK[16]={0x0001, 0x0002, 0x0004, 0x0008,
                   0x0010, 0x0020, 0x0040, 0x0080,
                   0x0100, 0x0200, 0x0400, 0x0800,
                   0x1000, 0x2000, 0x4000, 0x8000 };

/*** Function Prototypes ***/                   

void t2rexdt( int hh, int mm, int ss, DATETIME *pdt );
void d2rexdt( int y, int m, int d, DATETIME *pdt );
void clrampersand( char *str );
int insCONTlist( CDESC *e, int so, int ss );

void newR3bco1( FILE *frex, BCO1 *pbco1 );
void updR3bco1( FILE *frex, BCO1 *pbco1 );

void bldR3chdr( FILE *frex );
int  newR3chdr( FILE *frex );
void bldR3cent( int seq, FILE *frex );
int  newR3cent( FILE *frex );

int  newR3mhdr( FILE *frex );
int  newR3m1ent( FILE *fptmp, FILE *frex );
int  newR3m2ent( FILE *cdpin, FILE *frex );

int  newR3ahdr( FILE *frex );
int  newR3aent( FILE *frex );

int putR3seq( FILE *frex, long size, void *ptr );
int endR3seq( FILE *frex );

int date2dow( int d, int m, int y );
char DOW[8][4]={"ERR","SUN","MON","TUE","WED","THU","FRI","SAT"};

char* trim_lead( char *cp );
void trim_trail( char *cp );
char* trim_both( char *cp );

int pck( void );

/*** REX-3 Structures ***/

REXBLK blk;  /* memory holder for single 1K REX-3 block */

HDR1K hdr[REX_MAX_BLKS];  /* array of R3-block headers for all 256 blocks */
short hdr2ord[REX_MAX_BLKS]; /* converts physical to ordinal block nos */
TREE tree[REX_MAX_BLKS];  /* array of flat memory information */
long flataddr;  /* used to keep track of flat address */

BCO1 *pbco1, bco1;   /* BCO1 structure */
CONT_HDR *pch, ch;   /* CONTacts header structure */
CONT_ENT *pce, ce;   /* CONTacts entry structure */
MEMO_HDR *pmh, mh;   /* MEMO header structure */
MEMO_ENT *pme, me;   /* MEMO entry structure */
PREF_HDR pref;       /* PREFerences structure */

CLDR_HDR *pah, ah;   /* CLDR header structure */
CLDR_ENT *pae, ae;   /* CLDR entry structure */
CLDR_RPT *par, ar;   /* CLDR repeat parameters structure */

long lineno;

FILE *f123; /* used for debugging only */

char line[2048];

int main( int argc, char *argv[] ) {

  int i,j,k,len;
  int ilast;
  int iret;
  int first;
  short prev, next, this, size;
  FILE *cdpin, *mapin, *frex, *mdfin, *cdain;
  FILE *cdnin, *fpini, *fptmp;
  char fldname[64], fldtype[64];
  char catname[64];
  char chlbl, chent;
  int ibs, nbs;
  int ifld, rfld;
  int state=0;
  char *cp, *cpe, *cpb;
  int entrline;
  int seqno;
  CDESC *cde, *dbe;
  int so, ss, ssi;
  char fncdp[128], fnmap[128], fnout[128], fbase[128], fnmdf[128];
  char fncda[128], fncdn[128], fnini[128], fntmp[128];

  if(argc<3) {
printf("HP200LX to REX3 Data Convertor\n");
printf("Version 0.94 Beta  23 Mar 2001\n");
printf("(c)2001 by R. Christopher Lott\n\n");
printf("Usage: hp2rex {fbase} -[ICMNA]\n");
printf("where: -[ICMNA] options (single argument) as follows:\n");
printf("       I  Use the filename overrides specified in .INI file\n");
printf("       C  process and store CONT data\n");
printf("       M  process and store MEMO data from text files\n");
printf("       N  process and store MEMO data from Notetaker\n");
printf("       A  process and store APPT data (TBD)\n");
printf("filenames as follows:\n");
printf("CONT : fbase.CDP is CDF Phonebook data file from 200LX PIM\n");
printf("       fbase.MAP is MAP data file prepared per instr.\n");
printf("       fbase.BIN is REX3 output file\n");
printf("MEMO : fbase.MDF is file of text filenames for MEMO\n");
printf("       fbase.CDN is CDF Note data file from 200LX PIM\n");
printf("APPT : fbase.CDA is CDF Appt data file from 200LX PIM (TBD)\n");
printf("OR.... fbase.INI names each file, if different basenames are desired\n");
    exit(0);
  }

/*** this establishes default filenames ***/
  strncpy(fbase,argv[1],128);
  for(i=0;i<strlen(fbase);i++) fbase[i]=toupper(fbase[i]);
  strncpy(fncdp,fbase,128);
  strncpy(fncda,fbase,128);
  strncpy(fncdn,fbase,128);
  strncpy(fnmap,fbase,128);
  strncpy(fnout,fbase,128);
  strncpy(fnmdf,fbase,128);
  strncpy(fnini,fbase,128);
  strcat(fncdp, ".CDP");
  strcat(fncda, ".CDA");
  strcat(fncdn, ".CDN");
  strcat(fnmap, ".MAP");
  strcat(fnout, ".BIN");
  strcat(fnmdf, ".MDF");
  strcat(fnini, ".INI");

  frex=fopen(fnout, "w+b");
  if(!frex) {
    fprintf(stderr,"Error Opening REX Output File: %s\n", fnout );
    exit(99);
  }

  for(i=0;i<strlen(argv[2]);i++) argv[2][i]=toupper(argv[2][i]);

  if(strchr(argv[2],'C')) { PROC_CONT=1; PROC_NUM++; }
  else                      PROC_CONT=0;
  if(strchr(argv[2],'M')) { PROC_MEMO1=1; PROC_NUM++; }
  else                      PROC_MEMO1=0;
  if(strchr(argv[2],'N')) { PROC_MEMO2=1; PROC_NUM++; }
  else                      PROC_MEMO2=0;
  if(strchr(argv[2],'A')) { PROC_CLDR=1; PROC_NUM++; }
  else                      PROC_CLDR=0;
  if(strchr(argv[2],'I')) { USE_INI=1; }
  else                      USE_INI=0;

  /*** all or some filenames may be changed in the .ini file ***/
if(USE_INI) {
  fpini=fopen(fnini, "r");
  if(!fpini) {
    fprintf(stderr,"Error Opening INI File: %s\n", fnini );
    exit(99);
  }

  while( fgets(line,2048,fpini) ) {
    cp=strchr(line,';');
    if(cp) *cp=0; /* handle comments */
    if(strlen(line) < 5) continue;
    for(i=0;i<strlen(line);i++) line[i]=toupper(line[i]);
    if( 0==strncmp(line, "CDP", 3) ) sscanf( line+3, "%s", fncdp );
    if( 0==strncmp(line, "CDA", 3) ) sscanf( line+3, "%s", fncda );
    if( 0==strncmp(line, "CDN", 3) ) sscanf( line+3, "%s", fncdn );
    if( 0==strncmp(line, "MAP", 3) ) sscanf( line+3, "%s", fnmap );
    if( 0==strncmp(line, "BIN", 3) ) sscanf( line+3, "%s", fnout );
    if( 0==strncmp(line, "MDF", 3) ) sscanf( line+3, "%s", fnmdf );
  }

} /* end processing .INI file */

  if(!PROC_NUM) {
    fprintf(stderr,"INI file: %s\n", fnini );
    fprintf(stderr,"CDP file: %s\n", fncdp );
    fprintf(stderr,"CDA file: %s\n", fncda );
    fprintf(stderr,"CDN file: %s\n", fncdn );
    fprintf(stderr,"MAP file: %s\n", fnmap );
    fprintf(stderr,"BIN file: %s\n", fnout );
    fprintf(stderr,"MDF file: %s\n", fnmdf );
    fprintf(stderr,"Nothing Specified to Process\n");
    exit(99);
  }

  pbco1 = &bco1;
  pce=&ce;
  pch=&ch;
  pme=&me;
  pmh=&mh;

  fprintf(stderr,"HP200LX to REX3 Data Convertor\n");
  fprintf(stderr,"Version 0.94 Beta  23 Mar 2001\n");
  fprintf(stderr,"(c)2001 by R. Christopher Lott\n");


  bco1.id[0]='B'; bco1.id[1]='C'; bco1.id[2]='O'; bco1.id[3]='1';
  for(i=0;i<16;i++) bco1.unk[i]=0;
  for(i=0;i<8;i++) bco1.did[i]=0;
  bco1.last_sync_date.year = 1999;
  bco1.last_sync_date.month = 11;
  bco1.last_sync_date.day = 27;
  bco1.last_sync_date.dow = 0;
  bco1.last_sync_date.hour = 8;
  bco1.last_sync_date.minute = 30;
  bco1.last_sync_date.second = 55;

  bco1.sync_range_begin.year = 1998;
  bco1.sync_range_begin.month = 1;
  bco1.sync_range_begin.day = 7;
  bco1.sync_range_begin.dow = 0;
  bco1.sync_range_begin.hour = 8;
  bco1.sync_range_begin.minute = 30;
  bco1.sync_range_begin.second = 55;

  bco1.sync_range_end.year = 1999;
  bco1.sync_range_end.month = 12;
  bco1.sync_range_end.day = 31;
  bco1.sync_range_end.dow = 0;
  bco1.sync_range_end.hour = 8;
  bco1.sync_range_end.minute = 30;
  bco1.sync_range_end.second = 55;

  bco1.PREF_flat_ofs = 0;
  bco1.ZONE_flat_ofs = 0;
  bco1.TODO_flat_ofs = 0;
  bco1.MEMO_flat_ofs = 0;
  bco1.CONT_flat_ofs = 0;
  bco1.CLDR_flat_ofs = 0;

  flataddr=0;
  newR3bco1( frex, pbco1 );

if(PROC_CONT) {

  cdpin=fopen(fncdp, "rb");
  if(!cdpin) {
    fprintf(stderr,"Error Opening CDP Input File: %s\n", fncdp );
    exit(99);
  }

  mapin=fopen(fnmap, "rb");
  if(!mapin) {
    fprintf(stderr,"Error Opening MAP Input File: %s\n", fnmap );
    exit(99);
  }

  bco1.CONT_flat_ofs = flataddr;

  flds[0] ="Last Name";
  flds[1] ="First Name";
  flds[2] ="Company";
  flds[3] ="Title";
  flds[4] ="Work Address";
  flds[5] ="Work-Addr2";
  flds[6] ="Work-City";
  flds[7] ="Work-State";
  flds[8] ="Work-Zip";
  flds[9] ="Work-Country";
  flds[10]="Home-Address";
  flds[11]="Home-Addr2";
  flds[12]="Home-City";
  flds[13]="Home-State";
  flds[14]="Home-Zip";
  flds[15]="Home-Country";

  flds[16]="Work Phone";   /* these are the "phone" fields */
  flds[17]="Home Phone";   /* dispno refers to which of these to show in */
  flds[18]="E-mail";       /* the main list - dispno0=flds[16], etc... */
  flds[19]="Work Fax";
  flds[20]="Home Fax";     /* dispflag, after MSB stripped off, tells how */
  flds[21]="Cell Phone";   /* many "phone" entries there are this contact */
  flds[22]="Car Phone";
  flds[23]="Pager";
  flds[24]="Other";
  flds[25]="Main Phone";
  flds[26]="Web";

  flds[27]="NOTES";        /* dispflag MSB (0x80&flagnum) means NOTES exist */

  for(i=0;i<3;i++) for(j=0;j<5;j++) nents[i][j]=0;
  nentsTotal=0;

  lbl[0]='#'; /* preload label array */
  for(i=0;i<26;i++) lbl[i+1]='A'+i;

  state=0;
  nhpcf=0;
  while( fgets(line,2048,mapin) ) {
    if(state==0) {
      if(4==sscanf(line,"%d %d %s %s", &ifld, &rfld, fldname, fldtype )) {
        if(nhpcf>=MAXHPCF) {
          fprintf(stderr,"Too Many Fields in HP Phonebook File\n");
          exit(99);
        }
        clrampersand( fldname );
        /* printf("%d->%d %s / %s\n", ifld, rfld, fldname, fldtype ); */
        ifld--;
        hp2rex[ifld]=rfld;
        nhpcf++;
      }
      if(strstr(line,"Categories") ) state=1;
    } else {
      if(1==sscanf(line,"%s", catname )) {
        /* printf("CAT: %s\n", catname ); */
        state++;
        if(state>5) break;
      }
    }
  }
  fclose(mapin);

  /* load up SUBSET names */
  nsubs=1;
  subs[0]="ALL";
  subs[1]=0;
  subs[2]=0;
  subs[3]=0;
  subs[4]=0;

/* first pass through contacts file - build indices */
  fprintf(stderr, "CONT Pass 1: Building Index Lists...processing entry ");
  lineno=0;
  seqno=-1;
  while( fgets(line,2048,cdpin) ) {
    cpb=line;
    lineno++;
    if(cpb[0]=='D') {
      seqno++;
      if(SEQMAX && (seqno>=SEQMAX)) break;
      entrline=0;
      cpb+=2;
    } else if(cpb[0]=='N') {
      entrline++;
      cpb+=2;
    } else if(cpb[0]!='"') {
      fprintf(stderr,"Error reading CDF file format at Line %ld\n", lineno );
      exit(99);
    }

    if(entrline==0) { /* DATA line */
      cpb++; /* skip over first quote mark */
      for(i=0;i<nhpcf;i++) {
        hpfld[i][0]=0;
        if(i<(nhpcf-1)) cpe=strstr(cpb,"\",\"");
        else {
          cpe=line+strlen(line)-1;
          if(cpe) {
            while((*cpe=='"') || (*cpe=='\r') || (*cpe=='\n')) {
              if(cpe==cpb) break;
              cpe--;
            }
          }
        }
        if(!cpe) {
          fprintf(stderr,"\n");
          fprintf(stderr,"i: %d\n", i );
          fprintf(stderr,"CPB: %s\n", cpb );
          fprintf(stderr,"CPE: %s\n", cpe );
          fprintf(stderr,"FILE: %s, LINENO: %ld\n", fncdp, lineno );
          fprintf(stderr,"ERROR: no quote-comma-quote found: %d\n", 1000+i );
          exit(99);
        }
        for(j=0,cp=cpb;cp<cpe;cp++,j++) hpfld[i][j] = *cp;
        hpfld[i][j]=0;
        strncpy(r3fld[hp2rex[i]], hpfld[i], 64);
        if(i<(nhpcf-1)) cpb=cpe+3;
        /*****
        if(seqno<10) {
          printf("<%d:%d><%s>\n", seqno, hp2rex[i], r3fld[hp2rex[i]] );
        }
        *****/
      }
      fprintf(stderr, "#%03d%n", seqno, &nbs);
      for(ibs=0;ibs<nbs;ibs++) fprintf(stderr,"%c", BS);
    } else { /* NOTE data lines */
      if(1) continue; /* no NOTES action first pass */
    }

    if(entrline==0) {
      cde=calloc(1,sizeof(CDESC));
      if(!cde) {
        fprintf(stderr,"\nError Allocating Memory for Contacts\n"); 
        fprintf(stderr,"Try transferring a smaller number of entries\n"); 
        exit(99);
      }

      cde->this = cde;
      cde->seq = seqno;   /* sequential entry number 0-based */
      nentsTotal++;
      seq2cd[seqno]=cde;
      for(i=0;i<3;i++) { /* save the first part of each sort order string */
        for(j=0;j<7;j++) {
          cde->str[i][j]=r3fld[i][j];
          if(r3fld[i][j]==0) break;
        }
        cde->str[i][j]=0;
      }
      /* make blank entries contain data, as follows: */
      /* if no name, use company if given, else UNKNOWN */
      /* if no company, use name if given, else UNKNOWN */
      /* if no category, use UNKNOWN */
      l0=strlen(cde->str[0]) > 0;
      l1=strlen(cde->str[1]) > 0;
      l2=strlen(cde->str[2]) > 0;
      if(!l0) {
        if(l2) strncpy(cde->str[0], cde->str[2], 8);
        else   strncpy(cde->str[0], "UNKNOWN", 8);
      }
      if(!l1) strncpy(cde->str[1], "UNKNOWN", 8);
      if(!l2) {
        if(l0) strncpy(cde->str[2], cde->str[0], 8);
        else   strncpy(cde->str[2], "UNKNOWN", 8);
      }

      so=0; /* only sort sort order=0 for now */
      ss=0; /* only sort subset=0 for now */
      /* for(so=0;so<3;so++) */
      for(so=2;so>=0;so--) {
        insCONTlist( cde, so, ss );
      }

    }
  } /* end while - pass 1  */
  if(seqno<SEQMAX) {
    fprintf(stderr, "#%03d%n", seqno, &nbs);
    for(ibs=0;ibs<nbs;ibs++) fprintf(stderr,"%c", BS);
  }
  fprintf(stderr, "\n");


#if 0
/* debug printout by seqno */
  for(i=0;i<nentsTotal;i++) {
    dbe=seq2cd[i]; /* beginning of list */
    printf("%d ", dbe->seq );
    for(j=0;j<3;j++) printf("%-8s ", dbe->str[j] );
    printf("\n");
  }

  if(1) exit(0);
#endif

#if 0
/* debug printout by list navigation */
  so=0;
  ss=0;
  for(so=0;so<3;so++) {
    printf("List Navigation, so=%d\n", so);
    dbe=cl[so][ss]; /* beginning of list */
    while(dbe) {
      printf("%03d ", dbe->seq );
      for(j=0;j<3;j++) printf("%-8s ", dbe->str[j] );
      /****
      printf("%03d  ", dbe->seq );
      printf("%p   ", dbe );
      ****/
      printf("\n");
      dbe=dbe->next[so][0];
    }
  }
#endif


/* make alphabetical indices */
/* initialize index arrays, all -1's */
  for(i=0;i<3;i++) {  /* loop over all sort orders */
  for(j=0;j<5;j++) {  /* loop over all subsets */
  for(k=0;k<27;k++) { /* loop over all alphabet entries */
    pch->ix[i][j][k][0]=(short)-1;  /* COL 0 value */
    pch->ix[i][j][k][1]=(short)-1;  /* COL 1 value */
  } } }

for(so=0;so<3;so++) {
  ss=0;
  ssi=ss>0?1:0;

  k=0; /* label array index */
  chlbl=lbl[k];

  dbe=cl[so][ss]; /* beginning of list */
  chent=dbe->str[so][0]; /* first char of entry's string */
  seqno=0;  /* this is seqno within this list */
  do {
    do {
      chlbl=lbl[k];
      pch->ix[so][ss][k][0]=dbe->seq;
      pch->ix[so][ss][k][1]=seqno;
      k++;
    } while( (chlbl<chent) && (k<27) );
    if(k>=27) break;
    do { /* skip entries until next entry letter */
      dbe=dbe->next[so][ssi];
      seqno++;
      if(!dbe) break;
      chent=dbe->str[so][0];
    } while(chent==chlbl);
  } while(dbe);
  while(k<27) {
    pch->ix[so][ss][k][0]=pch->ix[so][ss][k-1][0];
    pch->ix[so][ss][k][1]=pch->ix[so][ss][k-1][1];
    k++;
  }


} /* end loop over all sort orders (so) */

#if 0
/* debug printout of indices */
  printf("ss=%d\n", ss );
  ss=0;
  for(k=0;k<=26;k++) {
    printf("%d %c: ", k, lbl[k] );
    for(so=0;so<3;so++) {
      seqno=pch->ix[so][ss][k][0];
      dbe=seq2cd[seqno];
      if(1) {
        printf("%03d,%03d<%-8s> ", pch->ix[so][ss][k][0], pch->ix[so][ss][k][1],
                                 dbe->str[so] );
      } else {
        printf("%03d,%03d   ", pch->ix[so][ss][k][0], pch->ix[so][ss][k][1]);
      }
    }
    printf("\n");
  }
#endif


  bldR3chdr( frex ); /* generate REX Contacts Header */
  fflush(frex);


/* second pass through contacts file - build indices */
  fprintf(stderr, "CONT Pass 2: Generating REX3 File...processing entry ");
  rewind(cdpin);
  lineno=0;
  seqno=-1;
  while( fgets(line,2048,cdpin) ) {
    cpb=line;
    lineno++;
    if(cpb[0]=='D') {
      if(seqno >= 0) {
        /* now, store entry into REX3 format */
        cdesc = *(seq2cd[seqno]); /* get contact descriptor */
        bldR3cent( seqno, frex );
        fflush(frex);
        fprintf(stderr, "#%03d%n", seqno, &nbs);
        for(ibs=0;ibs<nbs;ibs++) fprintf(stderr,"%c", BS);
      }
      seqno++;
      nonotes=0;
      sizenotes=0;
      if(SEQMAX && (seqno>=SEQMAX)) break;
      entrline=0;
      cpb+=2;
    } else if(cpb[0]=='N') {
      entrline++;
      cpb+=2;
    } else if(cpb[0]!='"') {
      fprintf(stderr,"Error reading CDP file format at Line %ld\n", lineno );
      exit(99);
    }

    if(entrline==0) { /* DATA line */
      cpb++; /* skip over first quote mark */
      for(i=0;i<nhpcf;i++) {
        hpfld[i][0]=0;
        if(i<(nhpcf-1)) cpe=strstr(cpb,"\",\"");
        else {
          cpe=line+strlen(line)-1;
          if(cpe) {
            while((*cpe=='"') || (*cpe=='\r') || (*cpe=='\n')) {
              if(cpe==cpb) break;
              cpe--;
            }
          }
        }
        if(!cpe) {
          fprintf(stderr,"\n");
          fprintf(stderr,"i: %d\n", i );
          fprintf(stderr,"CPB: %s\n", cpb );
          fprintf(stderr,"CPE: %s\n", cpe );
          fprintf(stderr,"FILE: %s, LINENO: %ld\n", fncdp, lineno );
          fprintf(stderr,"ERROR: no quote-comma-quote found: %d\n", 1000+i );
          exit(99);
        }
        for(j=0,cp=cpb;cp<cpe;cp++,j++) hpfld[i][j] = *cp;
        hpfld[i][j]=0;
        strncpy(r3fld[hp2rex[i]], hpfld[i], 64);
        if(i<(nhpcf-1)) cpb=cpe+3;
        /****
        if(seqno<10) {
          printf("<%d:%d><%s>\n", seqno, i, hpfld[i] );
        }
        ****/
      }

    } else { /* NOTE data lines */
      if( (entrline<=16)&&(strlen(cpb)) ) {
        i=entrline-1;
        nonotes++;
        strncpy(notes[i],cpb,64);
        /* strcat(notes[i], "\r\n"); */
        sizenotes += strlen(notes[i]);
      }
    }
  }
  if(seqno < SEQMAX) {
    /* now, store entry into REX3 format */
    cdesc = *(seq2cd[seqno]); /* get contact descriptor */
    bldR3cent( seqno, frex );
    fflush(frex);
    fprintf(stderr, "#%03d%n", seqno, &nbs);
    for(ibs=0;ibs<nbs;ibs++) fprintf(stderr,"%c", BS);
  }
  fprintf(stderr, "\n");

} /* end PROC_CONT section */


if(PROC_MEMO1 || PROC_MEMO2) {

/*** first, determine size of memo section ***/

  nmemo=0; nmemo1=0; nmemo2=0;

if(PROC_MEMO1) { /* determine size of MDF style MEMOs */
  mdfin=fopen(fnmdf, "rb");
  if(!mdfin) {
    fprintf(stderr,"Error Opening MDF Input File: %s\n", fnmdf );
    exit(99);
  }

  while( fgets(line,2048,mdfin) ) {
    sscanf(line,"%s%n", fntmp, &i );
    fptmp=fopen(fntmp,"rb");
    if(!fptmp) {
      fprintf(stderr,"Error Opening Memo #%d File: %s\n", nmemo, fntmp );
      exit(99);
    }

    cp=line+i; /* point to potential title */
    cp=trim_both( cp ); /* remove any leading/trailing white space */
    len=strlen(cp);
    if(len) { /* title IS specified on MDF line */
      strncpy( title, cp, 128 );
      titlmemo[nmemo]=1;
      linememo[nmemo]=0;
    } else { /* title first line of the file */
      if(!fgets(line,2048,fptmp)) {
        fprintf(stderr,"Error Reading Memo #%d File: %s at first line (title)\n", nmemo, fntmp );
        exit(99);
      }
      cp=trim_both(line);
      strncpy( title, cp, 128 );
      titlmemo[nmemo]=0;
      linememo[nmemo]=1;
    }
    titlelen=strlen(cp)+1;
    /* includes header, trailing size and memo null */
    sizememo[nmemo]=sizeof(MEMO_ENT)+3; 
    sizememo[nmemo]+=titlelen; /* now include title length */
    while( fgets(line,2048,fptmp) ) {
      len=strlen(line);
      sizememo[nmemo]+=len;
      linememo[nmemo]++;
    }
    if( ((linememo[nmemo]==0) && (titlmemo[nmemo]==1)) ||
        ((linememo[nmemo]==1) && (titlmemo[nmemo]==0)) ) {
      fprintf(stderr,"Error Reading Memo #%d File: %s, it is empty!\n", nmemo, fntmp );
      exit(99);
    }
    fclose(fptmp);
    nmemo++;
    nmemo1++;
  }

  if(!nmemo1) {
    fprintf(stderr,"No Memo Files to Process, Text File Type\n" );
    exit(99);
  }

  rewind(mdfin);
}

if(PROC_MEMO2) { /* determine size of Notetaker style MEMOs */
  cdnin=fopen(fncdn, "rb");
  if(!cdnin) {
    fprintf(stderr,"Error Opening CDN Input File: %s\n", fncdn );
    exit(99);
  }

  nmemo2=0;
  first=1;
  lineno=0;
  while( fgets(line,2048,cdnin) ) {
    lineno++;
    cpb=line;
    if(cpb[0]=='D') {
      if(!first) { nmemo2++; nmemo++; }
      else first=0;
      cpb+=3; /* skip to and over first quote mark */
      cpe=strstr(cpb,"\",\""); /* find trailing quote mark */
      *cpe=0; /* end title string */
      strncpy( title, cpb, 128 );
      titlelen=strlen(title)+1;
      /* includes header, trailing size and memo null */
      sizememo[nmemo]=sizeof(MEMO_ENT)+3; 
      sizememo[nmemo]+=titlelen; /* now include title length */
      linememo[nmemo]=0;
    } else if(cpb[0]=='N') {
      cpb+=2;
      len=strlen(cpb);
      sizememo[nmemo]+=len;
      linememo[nmemo]++;
    } else {
      fprintf(stderr,"\nError reading CDN file, Pass 1,  at Line %ld\n", lineno );
      exit(99);
    }

  }

  if(!first) { nmemo2++; nmemo++; } /* count last memo entry */

  if(!nmemo2) {
    fprintf(stderr,"No Memo Files to Process, Notetaker Type\n" );
    exit(99);
  }
  rewind(cdnin);
} /* end PROC2 size determination */

/*** Make MEMO header encompassing both types of memos ***/

  bco1.MEMO_flat_ofs = flataddr;

  mh.id[0]='M'; mh.id[1]='E'; mh.id[2]='M'; mh.id[3]='O';
  mh.unk1 = 1;
  mh.nents = nmemo;
  pmh = &mh;

  newR3mhdr( frex );

if(PROC_MEMO1) {
  fprintf(stderr, "MEMO1: processing file#/line#: ");
  for(i=0;i<8;i++) me.unk1[i]=0;
  pme=&me;
  for(imemo=0;imemo<nmemo1;imemo++) {
    me.size=sizememo[imemo];
    fgets(line,2048,mdfin);
    sscanf(line,"%s%n", fntmp, &i );
    fptmp=fopen(fntmp,"rb");
    if(!fptmp) {
      fprintf(stderr,"Error Opening Memo File: %s\n", fntmp );
      exit(99);
    }
    cp=line+i; /* point to potential title */
    cp=trim_both( cp ); /* remove any leading/trailing white space */
    len=strlen(cp);
    if(len) { /* title IS specified on MDF line */
      strncpy( title, cp, 128 );
      titlmemo[nmemo]=1;
      linememo[nmemo]=0;
    } else { /* title first line of the file */
      if(!fgets(line,2048,fptmp)) {
        fprintf(stderr,"Error Reading Memo #%d File: %s at first line (title)\n", nmemo, fntmp );
        exit(99);
      }
      cp=trim_both(line);
      strncpy( title, cp, 128 );
      titlmemo[nmemo]=0;
      linememo[nmemo]=1;
    }
    titlelen=strlen(cp)+1;
    newR3m1ent( fptmp, frex );
    fclose(fptmp);
  }

  fprintf(stderr,"\n");

} /* end PROC_MEMO1 section */

if(PROC_MEMO2) {

  fprintf(stderr, "MEMO2: processing note#/line#: ");
  for(i=0;i<8;i++) me.unk1[i]=0;
  pme=&me;
  lineno=0;
  for(imemo=nmemo1;imemo<nmemo;imemo++) {
    me.size=sizememo[imemo];
    fgets(line,2048,cdnin);
    lineno++;
    cpb=line;
    if(cpb[0]!='D') {
      fprintf(stderr,"\nError reading CDN file, Pass 2, at Line %ld\n", lineno );
      exit(99);
    }
    cpb+=3; /* skip to and over first quote mark */
    cpe=strstr(cpb,"\",\""); /* find trailing quote mark */
    *cpe=0; /* end title string */
    strncpy( title, cpb, 128 );
    titlelen=strlen(title)+1;
    newR3m2ent( cdnin, frex );
  }

  fclose(cdnin);
  fprintf(stderr,"\n");

} /* end PROC_MEMO2 section */

} /* end PROC_MEMO section (both MEMO1 and MEMO2 types) */

if(PROC_CLDR) {

  cdain=fopen(fncda, "rb");
  if(!cdain) {
    fprintf(stderr,"Error Opening ADF Input File: %s\n", fncda );
    exit(99);
  }
  /* first pass, just determine how many appt entries are there */
  nappt=0;
  while( fgets(line,2048,cdain) ) nappt++;
  rewind(cdain);

  bco1.CLDR_flat_ofs = flataddr;

  ah.id[0]='C'; ah.id[1]='L'; ah.id[2]='D'; ah.id[3]='R';
  ah.unk1 = 1;
  ah.nents = nappt;
  for(i=0;i<14;i++) ah.unk2[i]=0;
  pah = &ah;

  newR3ahdr( frex );
  fprintf(stderr, "CLDR:  processing appt#: ");
  pae=&ae;
  lineno=0;
  seqno=-1;
  for(iappt=0;iappt<nappt;iappt++) {
    ae.size=0; /* to be filled in later */
    fgets(line,2048,cdain);
    seqno++;
    lineno++;
    len=strlen(line);
    for(i=0;i<MAXSTR;i++) strings[i]=0;
    strings_size=0;
    cpb=line+1; /* skip over first quote mark */
    i=0;
    ilast=3;
    while(1) {
      cpe=strstr(cpb,"\",\"");
      if(!cpe) {
        cpe=line+strlen(line)-1;
        if(cpe) {
          while((*cpe=='"') || (*cpe=='\r') || (*cpe=='\n')) {
            if(cpe==cpb) break;
            cpe--;
          }
        }
      }
      if(!cpe) {
        fprintf(stderr,"\n");
        fprintf(stderr,"i: %d\n", i );
        fprintf(stderr,"CPB: %s\n", cpb );
        fprintf(stderr,"CPE: %s\n", cpe );
        fprintf(stderr,"FILE: %s, LINENO: %ld\n", fncdp, lineno );
        fprintf(stderr,"ERROR: no quote-comma-quote found: %d\n", 1000+i );
        exit(99);
      }
      for(j=0,cp=cpb;cp<cpe;cp++,j++) hpfield[j] = *cp;
      hpfield[j]=0;

      if(i==0) { /* DATE field */
        if(3!=sscanf(hpfield,"%d.%d.%d", &hpd, &hpm, &hpy )) {
          fprintf(stderr,"Error: lineno %ld,reading date: %s\n", 
              lineno, hpfield);
          exit(99);
        }
        d2rexdt(hpy, hpm, hpd, &(pae->beg) );
        pae->end = pae->beg; /* duplicate beg date */
      } else if(i==1) { /* Description string */
        len=strlen(hpfield);
        len++;
        cp=strings;
        hpc_title = cp;
        for(j=0;j<len;j++) cp[j]=hpfield[j];
        strings_size=len;
      } else if(i==2) { /* Place string */
        len=strlen(hpfield);
        len++;
        cp=strings+strings_size;
        hpc_place = cp;
        for(j=0;j<len;j++) cp[j]=hpfield[j];
        strings_size+=len;
      } else if(i==3) { /* HP State Field */
        if(1!=sscanf(hpfield,"%d", &hpstate )) {
          fprintf(stderr,"Error: lineno %ld,reading state: %s\n", 
              lineno, hpfield);
          exit(99);
        }
        if( hpstate & HP_APPT ) ilast=7;
      } else if( (i==4) && (hpstate&HP_APPT) ) { /* Beg Time */
        if(2!=sscanf(hpfield,"%2d:%2d", &hphhbeg, &hpmmbeg )) {
          fprintf(stderr,"Error: lineno %ld, reading Beg Time: %s\n", 
              lineno, hpfield);
          exit(99);
        }
        hpssbeg=0;
        t2rexdt(hphhbeg, hpmmbeg, hpssbeg, &(pae->beg) );
      } else if( (i==5) && (hpstate&HP_APPT) ) { /* End Time */
        if(2!=sscanf(hpfield,"%2d:%2d", &hphhend, &hpmmend )) {
          fprintf(stderr,"Error: lineno %ld, reading End Time: %s\n", 
              lineno, hpfield);
          exit(99);
        }
        hpssend=0;
        t2rexdt(hphhend, hpmmend, hpssend, &(pae->end) );
      } else if( (i==6) && (hpstate&HP_APPT) ) { /* Duration Days */
        if(1!=sscanf(hpfield,"%d", &hpdur )) {
          fprintf(stderr,"Error: lineno %ld, reading Duration: %s\n", 
              lineno, hpfield);
          exit(99);
        }
      } else if( (i==7) && (hpstate&HP_APPT) ) { /* Appt Lead Time */
        if(1!=sscanf(hpfield,"%d", &hplead )) {
          fprintf(stderr,"Error: lineno %ld, reading Lead Time: %s\n", 
              lineno, hpfield);
          exit(99);
        }
      }

      if(i!=ilast) {
        i++;
        cpb=cpe+3;
      } else break;
    }

    cp=strings+strings_size;
    *cp=0; /* null-terminated Notes field */
    strings_size++;

    ae.size=sizeof(CLDR_ENT)+strings_size+2;
    ae.type=0x00;
    ae.seqno=seqno;
    ae.unk1=0x01;
    ae.rpt=0x00;
    for(i=0;i<4;i++) ae.unk2[i]=0;
    ae.alarmb4=0;
    newR3aent( frex );

    fprintf(stderr, "#%03d%n", seqno, &nbs);
    for(ibs=0;ibs<nbs;ibs++) fprintf(stderr,"%c", BS);

  }

  fclose(cdain);
  fprintf(stderr,"\n");

} /* end PROC_CLDR section */

  endR3seq( frex );
  fprintf(stderr,"\nREX3 File %s Successfully Generated\n", fnout);

  exit(0);

}

/***** CONTACT FUNCTIONS ******/

void bldR3cent( int seq, FILE *frex ) {
  int i,j,k;
  int len;
  int lenfn, lenln, lenco;
  int so, ss, ssi;
  int nextseq, prevseq;
  short size;
  char *str;
  static long addr=0L;
  char line[256];

  size=0;

  /*****
  printf("saving REX3 entry, seqno=%d\n", seq );
  *****/

  pce->size = 0; /* temporary ...will update later */
  pce->subset=1; /* hard-code subset for now */
  pce->number=seq; /* just use sequence number */
  pce->unk2=1; /* always 0x0001 ???  */
  for(i=0;i<4;i++) {
    pce->unk3[i]=0; /* force to zeros */
  }

  /* first, copy over the Bank 1 entries to string memory block */

  for(i=0;i<MAXSTR;i++) strings[i]=0;
  strings_size=0;
  str=strings;
  pce->bitfield=0;
  pce->bk1cnt=0;
  for(i=0;i<16;i++) {
    len=strlen(r3fld[i]);
    if(len==0) continue; /* only process strings w/content */
    len++; /* account for trailing NULL */
    pce->bitfield |= BITMASK[i];
    for(j=0;j<len;j++) str[j]=r3fld[i][j];
    strings_size += len;
/*  fprintf(stderr,"->%02ds%02i%s\n", seq, i, str ); */
    str += len;
    pce->bk1cnt+=len;
  }
/*fprintf(stderr,"Press ENTER to continue\n");*/
/*gets(line);*/

  lenln=strlen( r3fld[0] );
  lenfn=strlen( r3fld[1] );
  lenco=strlen( r3fld[2] );

  if(lenfn==0) pce->f3off[0]=-1;
  else {
    pce->f3off[0]=lenln;
    if(lenln) pce->f3off[0]++; /* acct for trailing null */
  }

  if(lenco==0) pce->f3off[1]=-1;
  else {
    pce->f3off[1]=lenln+lenfn;
    if(lenln) pce->f3off[1]++; /* acct for trailing null */
    if(lenfn) pce->f3off[1]++; /* acct for trailing null */
  }

  pce->dispnum = 0xff;
  pce->flagnum = 0x00;
  for(i=0;i<11;i++) {
    k=i+16;
    len=strlen(r3fld[k]);
    if(len==0) continue;
    len++; /* account for trailing NULL */
    pce->flagnum++;
    if(pce->dispnum == 0xff ) pce->dispnum = (uchar)i; /* specify first avail number */
    str[0]=len+2;
    str[1]=i;
    str+=2;
    for(j=0;j<len;j++) str[j]=r3fld[k][j];
    str+=len;
    strings_size+=len+2;
  }

  if(nonotes) {
    pce->flagnum |= 0x80;
    for(i=0;i<nonotes;i++) {
      len=strlen(notes[i]);
      for(j=0;j<len;j++) str[j]=notes[i][j];
      str+=len;
    }
    str[0]=0; /* null terminate */
    strings_size += sizenotes+1;
  }

  size = sizeof(CONT_ENT) + strings_size;
  size += 2; /* acct for trailing size value after strings */
  pce->size = size;
  /****
  printf("writing %d %d %d bytes\n", size, pce->size, ce.size );
  *****/

  ss=0;
  ssi=ss>0?1:0;
  for(so=0;so<3;so++) {
    if( cdesc.prev[so][ssi] ) pce->prev[so][ssi] = (cdesc.prev[so][ssi])->seq;
    else                       pce->prev[so][ssi] = -1;
    if( cdesc.next[so][ssi] ) pce->next[so][ssi] = (cdesc.next[so][ssi])->seq;
    else                       pce->next[so][ssi] = -1;
  }

  newR3cent( frex );
  flataddr += size;

}

int newR3ahdr( FILE *frex ) {
  ushort size;
  char *cp;
  int i,j,k,len;

  /* save header to file */
  size=sizeof(CLDR_HDR);
  putR3seq( frex, (long)size, (void*)pah );
  flataddr += size;
  return 0;
}

int newR3aent( FILE *frex ) {
  ushort size;
  char *cp;
  int i,j,k,len;

  /* save entry to file */
  size=sizeof(CLDR_ENT);
  putR3seq( frex, (long)size, (void*)pae );
  flataddr += size;

  /* save strings to file */
  putR3seq( frex, (long)strings_size, (void*)strings );
  flataddr += strings_size;

  /* save trailing size to file */
  size =pae->size;
  putR3seq( frex, (long)2, (void*)&size );
  flataddr += 2;
  return 0;
}

int newR3mhdr( FILE *frex ) {
  ushort size;
  char *cp;
  int i,j,k,len;

  size = sizeof(MEMO_HDR);
  /* save entry header to file */
  putR3seq( frex, (long)size, (void*)pmh );
  flataddr += size;
  return 0;
}

int newR3m1ent( FILE *fptmp, FILE *frex ) {

  ushort size;
  char *cp;
  int i,j,k,len,lno;
  int mnlen, lnlen;

  memset(line,0,256);

  fprintf(stderr, "%02d/%n", imemo+1, &mnlen );

  me.lentitle=titlelen;
  me.lenmemo=me.size-titlelen-sizeof(MEMO_ENT)-2;;
  /* save entry header to file */
  size = sizeof(MEMO_ENT);
  putR3seq( frex, (long)size, (void*)pme );
  flataddr += size;

  if(titlmemo[imemo]==0) lno=1;
  else lno=0;

  /* write title to rex file */
  putR3seq( frex, (long)titlelen, (void*)title );
  flataddr += titlelen;

  lineno=0;
  while( fgets(line,2048,fptmp) ) {
    len=strlen(line);
    lineno++;
    if(lno==linememo[imemo]-1) line[len++]=0x00; /* add trailing null */
    putR3seq( frex, (long)len, (void*)line );
    flataddr += len;
    lno++;
    fprintf(stderr, "%03d%n", lno, &lnlen );
    for(i=0;i<lnlen;i++) fprintf(stderr,"%c", BS);
    // delay((unsigned)25); // TODO need modern delay
  }

  size=me.size;
  putR3seq( frex, (long)2, (void*)(&size) );
  flataddr += 2;

  for(i=0;i<mnlen;i++) fprintf(stderr,"%c", BS);

  return 0;
}

int newR3m2ent( FILE *cdnin, FILE *frex ) {

  ushort size;
  char *cp, *cpb, *cpe;
  int i,j,k,len,lno;
  int mnlen, lnlen;

  memset(line,0,2048);

  fprintf(stderr, "%02d/%n", imemo+1, &mnlen );

  me.lentitle=titlelen;
  me.lenmemo=me.size-titlelen-sizeof(MEMO_ENT)-2;;
  /* save entry header to file */
  size = sizeof(MEMO_ENT);
  putR3seq( frex, (long)sizeof(MEMO_ENT), (void*)pme );
  flataddr += size;

  /* write title to rex file */
  putR3seq( frex, (long)titlelen, (void*)title );
  flataddr += titlelen;

  lineno=0;
  for(lno=0;lno<linememo[imemo];lno++) {
    fgets(line,2048,cdnin);
    lineno++;
    cpb=line;
    if(cpb[0]!='N') {
      fprintf(stderr,"\nError reading CDN file format at Line %ld\n", lineno );
      exit(99);
    }
    cpb+=2;
    len=strlen(cpb);
    if(lno==linememo[imemo]-1) cpb[len++]=0x00; /* add trailing null */
    putR3seq( frex, (long)len, (void*)cpb );
    flataddr += len;
    fprintf(stderr, "%03d%n", lno, &lnlen );
    for(i=0;i<lnlen;i++) fprintf(stderr,"%c", BS);
    // delay((unsigned)25); // TODO need modern delay
  }

  size=me.size;
  putR3seq( frex, (long)2, (void*)(&size) );
  flataddr += 2;

  for(i=0;i<mnlen;i++) fprintf(stderr,"%c", BS);

  return 0;
}

int newR3cent( FILE *frex ) {
  CONT_ENT next;
  char str[4096];
  short size;
  char *cp;
  int i;

  memset(str,0,4096);
  size=ce.size;
  *((CONT_ENT *)str) = ce; /* copy it from global storage */

  cp=str+sizeof(CONT_ENT); /* point to beginning of entry strings */ 
  for(i=0;i<strings_size;i++) *(cp++) = strings[i];
  *((short*)cp)=size;
  putR3seq( frex, (long)size, (void*)str );

#if 0
  for(i=0;i<size;i++) {
    if( (i%16==0) && i )printf("\n");
    printf("%02x ", (uchar)str[i] );
  }
  printf("\n");
#endif
  return 0;
}

void clrampersand( char *str ) {
  char *ap;
  ap=strchr(str,'&');
  if(!ap) return;
  while(ap[1]) {
    ap[0]=ap[1];
    ap++;
  }
  ap[0]=0;
}

#define DBUG 0

int insCONTlist( CDESC *e, int so, int ss ) {
  int i;
  int ssi; /* subset index */
  int found, compare;
  CDESC *p, *n, *l, *dbe;

  ssi=ss>0?1:0;

#if DBUG==1
  printf("\ninsCONTlist: so=%d, ss=%d, ssi=%d, nents=%d, %s\n", 
         so, ss, ssi, nents[so][ss], e->str[0] );
#endif

  if(nents[so][ss]==0) { /* first entry this subset */
#if DBUG==1
    printf("@INIT\n");
#endif
    e->prev[so][ssi] = (CDESC*)NULL;
    e->next[so][ssi] = (CDESC*)NULL;
    nents[so][ss]++;
    cl[so][ss]=e;
    return 0;
  }

  found=0;
  l=cl[so][ss]; /* start at first of list */

  for(i=0;i<nents[so][ss];i++) { /* loop over all entries this subset */
#if DBUG==1
    printf(".%d: %s vs %s\n", i, e->str[so], l->str[so] );
#endif
    compare=strncmp(e->str[so], l->str[so], 7 );
    if(compare<0) {
      found=1;
      break; /* entry point in list found */
    } 
    if(i!=(nents[so][ss]-1)) l=l->next[so][ssi];
  }
#if DBUG==1
  printf("i=%d@-%d:%s-%d:%s\n", i, e->seq, e->str[so], l->seq, l->str[so] );
#endif
  if(found) { /* insert this entry onto the list right before l */
    p=l->prev[so][ssi]; /* save this for easier notation */
    e->prev[so][ssi]=p; /* update this entry prev-pointer */
    e->next[so][ssi]=l; /* update this entry next-pointer */
    l->prev[so][ssi]=e; /* update next entry's prev pointer */
    if(p) { /*only update prev entry's "next" entry if not beg of list (NULL) */
      p->next[so][ssi]=e; /* update prev-entries' next-pointer */
    } else { /* if beg of list, update starting array */
      cl[so][ss] = e;
    }
  } else { /* insert it after l */
    l->next[so][ssi]=e;
    e->prev[so][ssi]=l;
    e->next[so][ssi]=(CDESC*)NULL;;
  }

#if DBUG==1
  dbe=cl[so][ss];
  do {
    printf("%d,", dbe->seq );
    dbe=dbe->next[so][ssi];
  } while(dbe);
  printf("\n");
#endif
 
  nents[so][ss]++;
  return 0;
}

void bldR3chdr( FILE *frex ) {
  int i,j,k, len;
  char *str;

#if 0
typedef struct {
  short ent1;   /* offset to first entry */
  char id[4];   /* always CONT */
  char unk1;    /* seems to be 0x01 always */
  short nentTotal; /* number of total contact entries */
  short nent[5];   /* number of contact entries each subset */
  short unk2[3];   /* 0 or 512, then 2, then 4 */
  short beg[3][5]; /* [0=last,1=first,2=comp][subseu:0-4] */
  short ix[3][5][27][2]; /* [0=last,1=first,2=comp][subseu:0-4] */
  char unk5[5]; /* something fills here before data labels begin */
} CONT_HDR;
#endif

  pch->ent1=0; /* will be updated later on */
  pch->id[0]='C';
  pch->id[1]='O';
  pch->id[2]='N';
  pch->id[3]='T';
  pch->unk1=0x01; /* unknown */

  pch->nentTotal=nentsTotal;
  for(i=0;i<5;i++) pch->nent[i]=nents[0][i];

  pch->unk2[0]=0; pch->unk2[1]=2; pch->unk2[2]=4; 

  for(i=0;i<3;i++) {
    for(j=0;j<5;j++) {
      pch->beg[i][j] = cl[i][j]->seq;
    }
  }
  /* note - pch->ix array already filled */
  for(i=0;i<2;i++) pch->unk5[i]=0;

/* generate category names and field name overrides */
  for(i=0;i<MAXSTR;i++) strings[i]=0;
  str=strings;
  strings_size=0;

/* subset names */
  pch->sslen=0;
  for(i=0;i<5;i++) {
    len=strlen(subs[i]);
    if(len) strncpy(str,subs[i],64);
    str[len]=0;
    str+=len+1;
    strings_size+=len+1;
    pch->sslen+=len+1;
  }
  pch->numbk2 = 0x0c;

/* field name overrides */
  for(i=0;i<28;i++) {
    len=strlen(flds[i]);
    if(len) strcpy(str,flds[i]);
    str[len]=0;
    str+=len+1;
    strings_size+=len+1;
  }

  pch->ent1=sizeof(CONT_HDR) + strings_size;

  newR3chdr( frex );
  flataddr += pch->ent1;

}

int newR3chdr( FILE *frex ) {
  int i;
  int size;
  char *cp;

  size=sizeof(CONT_HDR);
  cp = (char *)pch;
  putR3seq( frex, (long)size, (void*)cp );

  size=strings_size;
  cp = strings;
  /******
  printf("saveCONThdr: sizeof strings: %d\n", size );
  for(i=0;i<size;i++) {
    printf("%02x ", cp[i] );
    if((i%16)==15) printf("\n");
  }
  *******/

  putR3seq( frex, (long)size, (void*)cp );

  return 0;
}

void newR3bco1( FILE *frex, BCO1 *pbco1 ) {
  int i;
  int size;
  char *cp;

  size=sizeof(BCO1);
  cp = (char *)pbco1;
  putR3seq( frex, (long)size, (void*)cp );
  flataddr += size;

}

/* this func only called at the very end */
void updR3bco1( FILE *frex, BCO1 *pbco1 ) {
  int i, bc;
  int size;
  long pos0, pos;
  char *cp;

  pos0=ftell(frex);
  pos=0L;
  fseek(frex,pos,SEEK_SET);
  bc=fread(blk.buf, 1, REX_BLK_SIZE, frex);
  if(bc != REX_BLK_SIZE) {
    fprintf(stderr,"updR3bco1: Error Reading REX File\n");
    exit(99);
  }

/* copy over updated bco1 header */
  *( (BCO1*)(blk.buf+sizeof(HDR1K)) ) = *pbco1;  
  size=sizeof(BCO1);

  pos=0;
  fseek(frex,pos,SEEK_SET);
  bc=fwrite(blk.buf, 1, REX_BLK_SIZE, frex);
  if(bc != REX_BLK_SIZE) {
    fprintf(stderr,"updR3bco1: Error Writing REX File\n");
    exit(99);
  }
  fseek(frex,pos0,SEEK_SET);

}

int pck( void ) {
  if(pce == &ce) return 0;
  printf("ERROR: pce/ce mismatch: pce=%p, &ce=%p\n", pce, &ce );
  return 1;
}


/* this func writes sequential rex data starting at zero offset */
/* used to create brand new rex files.  you must call endR3seq func */
/* to finish out the file */

int putR3seq( FILE *frex, long size, void *ptr ) {
  static int first=1;
  static int ibk=0;
  static long addr=0;
  static char *prex3;
  static int blksize;
  long i,j, addrend, addrbeg;
  char *pbuf;
  int bc;

  pbuf = (char *) ptr;

  if(size <= 0) return 0;

  /******
  printf("putR3seq: writing %x bytes at %lx address\n", size, addr );
  printf("size of HDR1K=%d\n", sizeof(HDR1K) );
  *******/

  if(first) {
    for(j=sizeof(HDR1K);j<REX_BLK_SIZE;j++) blk.buf[j]=0xaa;
    for(i=0;i<REX_MAX_BLKS;i++) {
      tree[i].this=-1;
      tree[i].prev=-1;
      tree[i].next=-1;
      tree[i].beg=-1;
      tree[i].end=-1;
    }
    blk.hdr.sig = HEX_FOOD;
    blk.hdr.prev=-1;
    blk.hdr.next=1;
    blk.hdr.size=0;
    prex3=blk.buf + sizeof(HDR1K);
    ibk=0;
    blksize=0;
    first=0;
    tree[0].beg=0L;
  }

  for(i=0;i<size;i++) {
    *prex3 = *pbuf;
    blksize++;
    if( blksize == REX_BLK_SIZE - sizeof(HDR1K) ) {
      prex3=blk.buf + sizeof(HDR1K);
      blk.hdr.size=blksize;
      bc=fwrite( blk.buf, 1, REX_BLK_SIZE, frex );
      if(bc != REX_BLK_SIZE) {
        fprintf(stderr,"SEQREXBYTES: Error writing frex, bc=%d\n", bc);
        exit(99);;
      }
      for(j=sizeof(HDR1K);j<REX_BLK_SIZE;j++) blk.buf[j]=0xaa;
      tree[ibk].this=ibk;
      tree[ibk].prev=blk.hdr.prev;
      tree[ibk].next=blk.hdr.next;
      tree[ibk].size=blk.hdr.size;
      tree[ibk].end=addr;
      blksize=0;
      blk.hdr.prev++;
      blk.hdr.next++;
      ibk++;
      tree[ibk].beg=addr+1;
      tree[ibk].size=0;
    } else prex3++;
    pbuf++;
    addr++;
  } /* end loop over all bytes */

  tree[ibk].size=blksize; /* update latest size this block */
  return size;

}

int endR3seq( FILE *frex ) {
  int i, ibk;
  int bc, size;

  /* find last block */
  for(i=0;i<REX_MAX_BLKS;i++) if(tree[i].this<0) break;
  if(i>=REX_MAX_BLKS) {
    fprintf(stderr,"Error endR3seq, too many blocks\n");
    exit(99);
  }
  ibk=i; /* this is the last partial block */
  if(tree[ibk].size) { /* only write if really a partial block w/data */
    tree[ibk].next=-1;
    blk.hdr.next=-1;
    blk.hdr.size=tree[ibk].size;
    /* write last partial block */
    bc=fwrite( blk.buf, 1, REX_BLK_SIZE, frex );
    if(bc != REX_BLK_SIZE) {
      fprintf(stderr,"endR3seq: Error writing frex, bc=%d\n", bc);
      exit(99);
    }
    ibk++;
  }
   
  /* write all remaining empty blocks */
  for(i=sizeof(HDR1K);i<REX_BLK_SIZE;i++) blk.buf[i]=0xff;
  blk.hdr.sig=HEX_DEAD;
  blk.hdr.size=0;
  for(i=ibk;i<REX_MAX_BLKS;i++) {
    bc=fwrite( blk.buf, 1, REX_BLK_SIZE, frex );
    if(bc != REX_BLK_SIZE) {
      fprintf(stderr,"endR3seq: Error writing frex, bc=%d\n", bc);
      exit(99);
    }
  }

  updR3bco1(frex, pbco1);
  return 1;
}

void dateprint( FILE *fp, DATETIME *dt ) {

  fprintf(fp, "%04d-%02d-%02d %02d:%02d:%02d UNK: %02d\n",
      dt->year, dt->month, dt->day, dt->hour, 
      dt->minute, dt->second, dt->dow);
}

void d2rexdt( int y, int m, int d, DATETIME *pdt ) {
  if(y<150) y += 1900;
  if(y>9999) y=9999;
  pdt->year = y;

  pdt->month = m;
  pdt->day = d;

  pdt->dow = date2dow(d,m,y);
}

void t2rexdt( int hh, int mm, int ss, DATETIME *pdt ) {
  pdt->hour = hh;
  pdt->minute = mm;
  pdt->second = ss;
}


int date2dow( int d, int m, int y ) {
  int datetab[12]={3,127,240,364,484,608,728,852,976,1096,1220,1340};
  long dy,dm,dd, dh,dt;
  int dow;
  dy=y-1964;
  dm=m-1;
  dd=d;
  if((dy<0)||(dy>=136)) return 0;
  dh=dy*1461+datetab[dm];
  dh=dh/4+dd+3;
  dt=dh/7;
  dh=dh-dt*7;
  if((dh<0)||(dh>6)) return 0;
  dow=dh-1;
  if(dow<0) dow=6;
  dow++;
  return dow;
}

char* trim_lead( char *cp ) {
  int i,len;
  char *rp;

  len=strlen(cp);
  if(len==0) return cp;
  rp=cp;

  for(i=0;i<len;i++) {
    if(isspace(cp[i])) rp++;
    else break;
  }
  return rp;
}

void trim_trail( char *cp ) {
  int i,len;

  len=strlen(cp);
  if(len==0) return;

  for(i=len-1;i>=0;i--) {
    if(isspace(cp[i])) cp[i]=0;
    else break;
  }
  return;
}

char* trim_both( char *cp ) {
  int i,len;
  char *rp;

  len=strlen(cp);
  if(len==0) return cp;
  rp=cp;
/* trim trailing first */
  for(i=len-1;i>=0;i--) {
    if(isspace(cp[i])) cp[i]=0;
    else break;
  }
/* now trim any leading */
  for(i=0;i<len;i++) {
    if(isspace(cp[i])) rp++;
    else break;
  }
  return rp;
}
