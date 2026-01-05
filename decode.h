#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h"

#define MAX_SECRET_EXT 10

typedef struct _DecodeInfo
{
    /* Stego Image Info */
    char *stego_image_fname;
    FILE *fptr_stego_image;

    /* Output Secret File Info */
    char output_fname[50];
    FILE *fptr_output;

    /* Decoded metadata */
    char magic_string[3];
    char file_extn[MAX_SECRET_EXT];
    long file_size;

} DecodeInfo;

/***************** FUNCTION PROTOTYPES *****************/

/* Validate args for decoding */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

/* Open stego and output files */
Status open_decode_files(DecodeInfo *decInfo);

/* Perform decoding */
Status do_decoding(DecodeInfo *decInfo);

/* Decode functions (bit extraction) */
Status decode_magic_string(DecodeInfo *decInfo);
Status decode_secret_extn_size(DecodeInfo *decInfo, int *extn_size);
Status decode_secret_extn(DecodeInfo *decInfo, char *extn, int extn_size);
Status decode_secret_file_size(DecodeInfo *decInfo, long *fsize);
Status decode_secret_file_data(DecodeInfo *decInfo, long fsize);

/* Helper LSB decoders */
char decode_byte_from_lsb(char *image_buffer);
long decode_size_from_lsb(unsigned char *buffer);

#endif
