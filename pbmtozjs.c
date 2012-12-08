/*

  pbmtozjs.c Version 0.01 28.08.2002
 
  This program converts pbm images (produced by ghostscript) to
  Zenographics ZJ-stream format. This utility makes possible to
  use the HP LaserJet 1000 printer under Linux or other operating
  systems. This program uses Markus Kuhn's jbig compression library, 
  which can be obtained from "http://www.cl.cam.ac.uk/~mgk25/jbigkit/".
  Information about the ZJS format can be found at Zenographics's
  website "ddk.zeno.com".

  Compiling: 
     1) install jbigkit 
     2) copy jbig.h and libjbig.a into the directory of pbmtozjs
     3) run the command: gcc -O2 -W -o pbmtozjs pbmtozjs.c libjbig.a

  This preliminary version recieves the PBM file on the standard input
  and writes the ZJS strem onto the standard output. For example if
  you intend to use it with ghostscript and LaserJet 1000 then try the
  following

  gs -q -r600 -g4736x6817 -sDEVICE=pbmraw -sOutputFile=- -dNOPAUSE -dBATCH \
  <psfile> | pbmtozjs | lpr
  
  where <psfile> is the a postscript file.

  Note that this program is machine dependent. It works well on x86
  machines.
  
  Copying is permitted under GNU's General Public License (GPL)

  Copyright (c) Robert Szalai

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "jbig.h"

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;

/* from zjrca.h */
typedef enum {
  ZJT_START_DOC, ZJT_END_DOC,
  ZJT_START_PAGE, ZJT_END_PAGE,
  ZJT_JBIG_BIH,                     /*  Bi-level Image Header */
  ZJT_JBIG_BID,                     /*  Bi-level Image Data blocks */
  ZJT_END_JBIG,
  ZJT_SIGNATURE,
  ZJT_RAW_IMAGE,                    /*  full uncompressed plane follows */
  ZJT_START_PLANE, ZJT_END_PLANE
} ZJ_TYPE;

typedef struct _ZJ_HEADER {
  DWORD size;           /*  total record size, includes sizeof(ZJ_HEADER) */
  DWORD type;           /*  ZJ_TYPE */
  DWORD items;          /*  use varies by Type, e.g. item count */
  WORD reserved;        /*  later to be sumcheck or CRC */
  WORD signature;       /*  'ZZ' */
} ZJ_HEADER;

typedef enum {
  /* for START_DOC */
  ZJI_PAGECOUNT,          /* 0 number of ZJT_START_PAGE / ZJT_END_PAGE 
			     pairs, if known */
  ZJI_DMCOLLATE,          /* 1 from DEVMODE */
  ZJI_DMDUPLEX,           /* 2 from DEVMODE */
  
  /* for START_PAGE */
  ZJI_DMPAPER,            /* 3 from DEVMODE */
  ZJI_DMCOPIES,           /* 4 from DEVMODE */
  ZJI_DMDEFAULTSOURCE,    /* 5 from DEVMODE */
  ZJI_DMMEDIATYPE,        /* 6 from DEVMODE */
  ZJI_NBIE,               /* 7 number of Bi-level Image Entities, */
                          /*  e.g. 1 for monochrome, 4 for color */
  ZJI_RESOLUTION_X, ZJI_RESOLUTION_Y,     /* 8,9 dots per inch */
  ZJI_OFFSET_X, ZJI_OFFSET_Y,             /* 10,11 upper left corner */
  ZJI_RASTER_X, ZJI_RASTER_Y,             /* 12,13 raster dimensions */
  
  ZJI_COLLATE,            /* 14 asks for collated copies */
  ZJI_QUANTITY,           /* 15 copy count */

  ZJI_VIDEO_BPP,          /* 16 video bits per pixel */
  ZJI_VIDEO_X, ZJI_VIDEO_Y,  /* 17, 18 video dimensions 
				(if different than raster) */
  ZJI_INTERLACE,          /* 19 0 or 1 */
  ZJI_PLANE,              /* 20 enum PLANE */
  ZJI_PALETTE,            /* 21 translation table (dimensions in item 
			     type) */

  ZJI_PAD = 99,           /* bogus item type for padding stream */

  /* 0x8000-0x80FF : Item tags for QMS specific features. */ 
  ZJI_QMS_FINEMODE = 0x8000,      /* for 668, 671 */
  ZJI_QMS_OUTBIN                  /* for 671 output bin */
  /* 0x8100-0x81FF : Item tags for Minolta specific features. */
  /* 0x8200-0x82FF : Item tags for the next OEM specific features. */
} ZJ_ITEM;

typedef enum {
  ZJIT_UINT32=1,          /* unsigned integer */
  ZJIT_INT32,             /* signed integer */
  ZJIT_STRING,            /* byte string, NUL-terminated, DWORD-aligned */
  ZJIT_BYTELUT            /* DWORD count followed by that many byte
			     entries */
} ZJ_ITEM_TYPE;

typedef struct _ZJ_ITEM_HEADER {
  DWORD size;        /*  total record size, includes
			 sizeof(ZJ_ITEM_HEADER) */
  WORD  item;        /*  ZJ_ITEM */
  BYTE  type;        /*  ZJ_ITEM_TYPE */
  BYTE  param;       /*  general use */
} ZJ_ITEM_HEADER;

typedef struct _ZJ_ITEM_UINT32 {
  ZJ_ITEM_HEADER header;
  DWORD value;
} ZJ_ITEM_UINT32;

typedef struct _ZJ_ITEM_INT32 {
  ZJ_ITEM_HEADER header;
  long value;
} ZJ_ITEM_INT32;

typedef struct _BIE_CHAIN{
  unsigned char *data;
  size_t len;
  struct _BIE_CHAIN *next;
} BIE_CHAIN;

typedef union _SWAP_LONG{
    char byte[sizeof(long)];
    unsigned long dword;
} SWAP_LONG;

typedef union _SWAP_SHORT{
    char byte[sizeof(short)];
    unsigned short word;
} SWAP_SHORT;

static unsigned long long2dword(unsigned long dword)
{
  SWAP_LONG swap;

  swap.byte[3] = (( SWAP_LONG )dword).byte[0]; 
  swap.byte[2] = (( SWAP_LONG )dword).byte[1];
  swap.byte[1] = (( SWAP_LONG )dword).byte[2]; 
  swap.byte[0] = (( SWAP_LONG )dword).byte[3];
  return swap.dword;
}

static unsigned short short2word(unsigned short word)
{
  SWAP_SHORT swap;

  swap.byte[1] = (( SWAP_SHORT )word).byte[0]; 
  swap.byte[0] = (( SWAP_SHORT )word).byte[1];
  return swap.word;
}

static void chunk_write( unsigned long type, 
			 unsigned long items,
			 unsigned long size,
			 FILE *file )
{
  ZJ_HEADER chunk;
  
  chunk.type = long2dword( type );
  chunk.items = long2dword( items );
  chunk.size = long2dword( sizeof(ZJ_HEADER) + size );
  chunk.reserved = 0;
  chunk.signature = 0x5a5a;
  fwrite( &chunk, 1, sizeof(ZJ_HEADER), file);
}

static void item_uint32_write( unsigned short item,
			       unsigned long value,
			       FILE *file )
{
  ZJ_ITEM_UINT32 item_uint32;
  
  item_uint32.header.size = long2dword( sizeof(ZJ_ITEM_UINT32) );
  item_uint32.header.item = short2word( item );
  item_uint32.header.type = (ZJ_ITEM_TYPE) ZJIT_UINT32;
  item_uint32.header.param = 0;
  item_uint32.value = long2dword( value );
  fwrite( &item_uint32, 1, sizeof(ZJ_ITEM_UINT32), file);
}

static unsigned long getint(FILE *f)
{
  int c;
  unsigned long i;

  while ((c = getc(f)) != EOF && !isdigit(c))
    if (c == '#')
      while ((c = getc(f)) != EOF && !(c == 13 || c == 10)) ;
  if (c != EOF) {
    ungetc(c, f);
    fscanf(f, "%lu", &i);
  }

  return i;
}

int write_page( BIE_CHAIN **root, FILE *file)
{
  BIE_CHAIN *current = *root, *next, *temp;
  int i, len, pad_len;
  unsigned long xx,yy;

  /* error handling */
  if( current == NULL ){ 
    fprintf(stderr,"There is no JBIG!\n"); 
    return -1; 
  }
  if( current->next == NULL ){ 
    fprintf(stderr,"There is no or wrong JBIG header!\n"); 
    return -1;
  }
  if( current->len != 20 ){
    fprintf(stderr,"wrong BIH length\n"); 
    return -1; 
  }

  /* computing data length */
  temp = current->next; len = 0;
  while( temp->next != NULL ){
    len += temp->len;
    temp = temp->next;
  }
  pad_len = 16*((len+15)/16)-len;

  /* startpage, jbig_bih, jbig_bid, jbig_end, endpage */
  xx = (((long) current->data[ 4] << 24) | ((long) current->data[ 5] <<
					    16) |
	((long) current->data[ 6] <<  8) | (long) current->data[ 7]);
  yy = (((long) current->data[ 8] << 24) | ((long) current->data[ 9] <<
					    16) |
	((long) current->data[10] <<  8) | (long) current->data[11]);

  chunk_write( (ZJ_TYPE)ZJT_START_PAGE , 13, 13*sizeof(ZJ_ITEM_UINT32),
	       file );
  item_uint32_write( 23,                            0,   file );
  item_uint32_write( 22,                            1,   file );
  item_uint32_write( (ZJ_ITEM) ZJI_VIDEO_Y,         yy,  file );
  item_uint32_write( (ZJ_ITEM) ZJI_VIDEO_X,         xx,  file );
  item_uint32_write( (ZJ_ITEM) ZJI_VIDEO_BPP,       1,   file );
  item_uint32_write( (ZJ_ITEM) ZJI_RASTER_Y,        yy,  file );
  item_uint32_write( (ZJ_ITEM) ZJI_RASTER_X,        xx,  file );
  item_uint32_write( (ZJ_ITEM) ZJI_NBIE,            1,   file );
  item_uint32_write( (ZJ_ITEM) ZJI_RESOLUTION_Y,    600, file );
  item_uint32_write( (ZJ_ITEM) ZJI_RESOLUTION_X,    600, file );
  item_uint32_write( (ZJ_ITEM) ZJI_DMDEFAULTSOURCE, 7,   file );
  item_uint32_write( (ZJ_ITEM) ZJI_DMCOPIES,        1,   file );
  item_uint32_write( (ZJ_ITEM) ZJI_DMPAPER,         9,   file );

  chunk_write( (ZJ_TYPE) ZJT_JBIG_BIH, 0, current->len, file );
  fwrite( current->data, 1, current->len, (FILE *) file);
  
  next = current->next;
  free( current->data ); free( current );
  *root = NULL;
  current = next;
  
  chunk_write( (ZJ_TYPE) ZJT_JBIG_BID, 0, len + pad_len, file );  
  while( (next = current->next) != NULL ){
    fwrite( current->data, 1, current->len, (FILE *) file);
    if( current->data != NULL ){ 
      free( current->data );
      current->data = NULL;
    }else{ 
      fprintf(stderr,"Trying to free up a nullpointer\n"); exit(-1); 
    }    
    free( current );
    current = next;
  }
  for(i = 0; i < pad_len; i++ ) putc( 0, (FILE *) file );

  chunk_write( (ZJ_TYPE) ZJT_END_JBIG, 0, 0, file );
  chunk_write( (ZJ_TYPE) ZJT_END_PAGE, 0, 0, file );
  return 0;
}

void output_jbig(unsigned char *start, size_t len, void *file)
{
  BIE_CHAIN *current, **root = ( BIE_CHAIN **)file;

  if( (*root) == NULL ){
    (*root) = malloc( sizeof( BIE_CHAIN ) );
    if( (*root) == NULL ){ fprintf(stderr,"No memory\n"); exit(-1); }
    (*root)->data=NULL; (*root)->next=NULL; (*root)->len=0;
  }
  current = *root;  
  while( current->next != NULL ) current = current->next;
  current->data = malloc( len );
  if( current->data == NULL ){ fprintf(stderr,"No memory\n"); exit(-1); }
  current->next = malloc( sizeof( BIE_CHAIN ) );
  if( current->next == NULL ){ fprintf(stderr,"No memory\n"); exit(-1); }

  current->next->data=NULL; current->next->next=NULL; current->next->len=0;
  memcpy( current->data, start, len ); 
  current->len = len;
}

int main()
{
  FILE *input=stdin,*output=stdout;
  BIE_CHAIN *chain = NULL;
  unsigned long header=0x5a4a5a4a;
  int xx, yy, bpl;
  char c;
  unsigned char *buffer, *bitmaps[1];
  struct jbg_enc_state se; 

  /* start document */
  fwrite( &header, 1, sizeof(long), output );
  chunk_write( (ZJ_TYPE)ZJT_START_DOC , 2, 2*sizeof(ZJ_ITEM_UINT32),
	       output );
  item_uint32_write( (ZJ_ITEM)ZJI_PAGECOUNT, 0, output );
  item_uint32_write( (ZJ_ITEM)ZJI_DMDUPLEX,  1, output );

  /* reading the input file */
  do{
    /* the type of the input file must be pbmraw */
    if( getc( input ) == 'P'){
      if( getc( input ) != '4' ){
	fprintf(stderr,"wrong file type\n");
	return 0;
      }
    }else{
      fprintf(stderr,"wrong file type\n"); 
      break; 
    }
    do{ c = getc( input ); }while( (c != '\n')&&(c != EOF) );

    xx = getint( input );
    yy = getint( input );
    do{ c = getc( input ); }while( (c != '\n')&&(c != EOF) );

    bpl = (xx + 7) / 8;
    buffer = malloc( bpl * yy );
    fread( buffer, bpl * yy, 1, input );
    
    *bitmaps = buffer;
    jbg_enc_init( &se, xx, yy, 1, bitmaps,
		  output_jbig, &chain );     /* initialize encoder */
    jbg_enc_options( &se, JBG_ILEAVE | JBG_SMID, 
		     JBG_LRLTWO | JBG_TPDON | JBG_TPBON | JBG_DPON,
		     128, 16, 0);            /* setting up options */
    jbg_enc_out(&se);                        /* encode image */
    jbg_enc_free(&se);                       /* release allocated
						resources */
    write_page( &chain, output );            /* writing a page */
    free( buffer );
    c = getc( input ); ungetc( c, input );
  }while( c != EOF );
  /* enddoc */
  chunk_write( (ZJ_TYPE) ZJT_END_DOC , 0, 0, output );
  return 0;
}
