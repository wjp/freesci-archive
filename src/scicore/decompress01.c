/***************************************************************************
 decompress01.c Copyright (C) 1999 The FreeSCI project


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

***************************************************************************/
/* Reads data from a resource file and stores the result in memory */


#include <sciresource.h>

/***************************************************************************
* The following code was originally created by Carl Muckenhoupt for his
* SCI decoder. It has been ported to the FreeSCI environment by Sergey Lapin.
***************************************************************************/

/* TODO: Clean up, re-organize, improve speed-wise */

struct tokenlist {
	guint8 data;
	gint16 next;
} tokens[0x1004];

static gint8 stak[0x1014], lastchar;
static gint16 stakptr;
static guint16 numbits, bitstring, lastbits, decryptstart;
static gint16 curtoken, endtoken;


guint32 gbits(int numbits,  guint8 * data, int dlen);

void decryptinit3()
{
	int i;
	lastchar = lastbits = bitstring = stakptr = 0;
	numbits = 9;
	curtoken = 0x102;
	endtoken = 0x1ff;
	decryptstart = 0;
	gbits(0,0,0);
	for(i=0;i<0x1004;i++) {
		tokens[i].next=0;
		tokens[i].data=0;
	}
}

int decrypt3(guint8 *dest, guint8 *src, int length, int complength)
{
	static gint16 token;
	while(length != 0) {

		switch (decryptstart) {
		case 0:
		case 1:
			bitstring = gbits(numbits, src, complength);
			if (bitstring == 0x101) { /* found end-of-data signal */
				decryptstart = 4;
				return 0;
			}
			if (decryptstart == 0) { /* first char */
				decryptstart = 1;
				lastbits = bitstring;
				*(dest++) = lastchar = (bitstring & 0xff);
				if (--length != 0) continue;
				return 0;
			}
			if (bitstring == 0x100) { /* start-over signal */
				numbits = 9;
				endtoken = 0x1ff;
				curtoken = 0x102;
				decryptstart = 0;
				continue;
			}
			token = bitstring;
			if (token >= curtoken) { /* index past current point */
				token = lastbits;
				stak[stakptr++] = lastchar;
			}
			while ((token > 0xff)&&(token < 0x1004)) { /* follow links back in data */
				stak[stakptr++] = tokens[token].data;
				token = tokens[token].next;
			}
			lastchar = stak[stakptr++] = token & 0xff;
		case 2:
			while (stakptr > 0) { /* put stack in buffer */
				*(dest++) = stak[--stakptr];
				length--;
				if (length == 0) {
					decryptstart = 2;
					return 0;
				}
			}
			decryptstart = 1;
			if (curtoken <= endtoken) { /* put token into record */
				tokens[curtoken].data = lastchar;
				tokens[curtoken].next = lastbits;
				curtoken++;
				if (curtoken == endtoken && numbits != 12) {
					numbits++;
					endtoken <<= 1;
					endtoken++;
				}
			}
			lastbits = bitstring;
			continue; /* When are "break" and "continue" synonymous? */
		case 4:
			return 0;
		}
	}
	return 0;     /* [DJ] shut up compiler warning */
}


guint32 gbits(int numbits,  guint8 * data, int dlen)
{
	int place; /* indicates location within byte */
	guint32 bitstring;
	static guint32 whichbit=0;
	int i;

	if(numbits==0) {whichbit=0; return 0;}
  
	place = whichbit >> 3;
	bitstring=0;
	for(i=(numbits>>3)+1;i>=0;i--)
		{
			if (i+place < dlen)
				bitstring |=data[place+i] << (8*(2-i));
		}
	/*  bitstring = data[place+2] | (long)(data[place+1])<<8
	    | (long)(data[place])<<16;*/
	bitstring >>= 24-(whichbit & 7)-numbits;
	bitstring &= (0xffffffff >> (32-numbits));
	/* Okay, so this could be made faster with a table lookup.
	   It doesn't matter. It's fast enough as it is. */
	whichbit += numbits;
	return bitstring;
}

/***************************************************************************
* Carl Muckenhoupt's code ends here
***************************************************************************/


int decompress01(resource_t *result, int resh)
{
	guint16 compressedLength;
	guint16 compressionMethod;
	guint8 *buffer;

	if (read(resh, &(result->id),2) != 2)
		return SCI_ERROR_IO_ERROR;

#ifdef WORDS_BIGENDIAN
	result->id = GUINT16_SWAP_LE_BE_CONSTANT(result->id);
#endif

	result->number = result->id & 0x07ff;
	result->type = result->id >> 11;

	if ((result->number > 999) || (result->type > sci_invalid_resource))
		return SCI_ERROR_DECOMPRESSION_INSANE;

	if ((read(resh, &compressedLength, 2) != 2) ||
	    (read(resh, &(result->length), 2) != 2) ||
	    (read(resh, &compressionMethod, 2) != 2))
		return SCI_ERROR_IO_ERROR;

#ifdef WORDS_BIGENDIAN
	compressedLength = GUINT16_SWAP_LE_BE_CONSTANT(compressedLength);
	result->length = GUINT16_SWAP_LE_BE_CONSTANT(result->length);
	compressionMethod = GUINT16_SWAP_LE_BE_CONSTANT(compressionMethod);
#endif

	/*  if ((result->length < 0) || (compressedLength < 0))
	    return SCI_ERROR_DECOMPRESSION_INSANE; */
	/* This return will never happen in SCI0 or SCI1 (does it have any use?) */

	if ((result->length > SCI_MAX_RESOURCE_SIZE) ||
	    (compressedLength > SCI_MAX_RESOURCE_SIZE))
		return SCI_ERROR_RESOURCE_TOO_BIG;

	if (compressedLength > 4)
		compressedLength -= 4;
	else { /* Object has size zero (e.g. view.000 in sq3) (does this really exist?) */
		result->data = 0;
		result->status = SCI_STATUS_NOMALLOC;
		return SCI_ERROR_EMPTY_OBJECT;
	}

	buffer = malloc(compressedLength);
	result->data = malloc(result->length);

	if (read(resh, buffer, compressedLength) != compressedLength) {
		free(result->data);
		free(buffer);
		return SCI_ERROR_IO_ERROR;
	};


#ifdef _SCI_DECOMPRESS_DEBUG
	fprintf(stderr, "Resource %s.%03hi encrypted with method SCI01/%hi at %.2f%%"
		" ratio\n",
		Resource_Types[result->type], result->number, compressionMethod,
		(result->length == 0)? -1.0 : 
		(100.0 * compressedLength / result->length));
	fprintf(stderr, "  compressedLength = 0x%hx, actualLength=0x%hx\n",
		compressedLength, result->length);
#endif

	switch(compressionMethod) {

	case 0: /* no compression */
		if (result->length != compressedLength) {
			free(result->data);
			result->data = NULL;
			result->status = SCI_STATUS_NOMALLOC;
			free(buffer);
			return SCI_ERROR_DECOMPRESSION_OVERFLOW;
		}
		memcpy(result->data, buffer, compressedLength);
		result->status = SCI_STATUS_ALLOCATED;
		break;

	case 1: /* LZW */
		if (decrypt2(result->data, buffer, result->length, compressedLength)) {
			free(result->data);
			result->data = 0; /* So that we know that it didn't work */
			result->status = SCI_STATUS_NOMALLOC;
			free(buffer);
			return SCI_ERROR_DECOMPRESSION_OVERFLOW;
		}
		result->status = SCI_STATUS_ALLOCATED;
		break;

	case 2: /* ??? */
		decryptinit3();
		if (decrypt3(result->data, buffer, result->length, compressedLength)) {
			free(result->data);
			result->data = 0; /* So that we know that it didn't work */
			result->status = SCI_STATUS_NOMALLOC;
			free(buffer);
			return SCI_ERROR_DECOMPRESSION_OVERFLOW;
		}
		result->status = SCI_STATUS_ALLOCATED;
		break;

	case 3: /* Some sort of Huffman encoding */
		if (decrypt2(result->data, buffer, result->length, compressedLength)) {
			free(result->data);
			result->data = 0; /* So that we know that it didn't work */
			result->status = SCI_STATUS_NOMALLOC;
			free(buffer);
			return SCI_ERROR_DECOMPRESSION_OVERFLOW;
		}
		result->status = SCI_STATUS_ALLOCATED;
		break;

	default:
		fprintf(stderr,"Resource %s.%03hi: Compression method SCI1/%hi not "
			"supported!\n", Resource_Types[result->type], result->number,
			compressionMethod);
		free(result->data);
		result->data = 0; /* So that we know that it didn't work */
		result->status = SCI_STATUS_NOMALLOC;
		free(buffer);
		return SCI_ERROR_UNKNOWN_COMPRESSION;
	}

	free(buffer);
	return 0;
}

