#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "common.h"
#include "types.h"

/*****************************************************
 * Validate command-line args for decode mode
 * argv[2] = stego_image.bmp
 * argv[3] = output secret file (optional)
 *****************************************************/
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    if (argv[2] == NULL)
    {
        printf("ERROR: Missing stego image\n");
        return e_failure;
    }

    /* Check .bmp */
    char *ext = strstr(argv[2], ".bmp");
    if (ext == NULL || strcmp(ext, ".bmp") != 0)
    {
        printf("ERROR: Input stego image must be .bmp\n");
        return e_failure;
    }

    decInfo->stego_image_fname = argv[2];

    /* Output filename */
    if (argv[3] != NULL)
        strcpy(decInfo->output_fname, argv[3]);
    else
        strcpy(decInfo->output_fname, "decoded_secret.txt");

    return e_success;
}

/*****************************************************
 * Open files for decoding
 *****************************************************/
Status open_decode_files(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "r");
    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        return e_failure;
    }

    decInfo->fptr_output = fopen(decInfo->output_fname, "w");
    if (decInfo->fptr_output == NULL)
    {
        perror("fopen");
        return e_failure;
    }

    return e_success;
}

/*****************************************************
 * Decode a single byte from 8 bytes LSB
 *****************************************************/
char decode_byte_from_lsb(char *image_buffer)
{
    char data = 0;
    for (int i = 0; i < 8; i++)
    {
        data = (data << 1) | (image_buffer[i] & 1);
    }
    return data;
}

/*****************************************************
 * Decode 32-bit value from 32 bytes LSB
 *****************************************************/
long decode_size_from_lsb(unsigned char *buffer)
{
    long value = 0;
    for (int i = 0; i < 32; i++)
    {
        value = (value << 1) | (buffer[i] & 1);
    }
    return value;
}

/*****************************************************
 * Decode MAGIC STRING
 *****************************************************/
Status decode_magic_string(DecodeInfo *decInfo)
{
    char buffer[8];
    char magic_read[3];
    magic_read[2] = '\0';

    /* Skip BMP header (54 bytes) */
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);

    for (int i = 0; i < 2; i++)
    {
        fread(buffer, 8, 1, decInfo->fptr_stego_image);
        magic_read[i] = decode_byte_from_lsb(buffer);
    }

    if (strcmp(magic_read, MAGIC_STRING) == 0)
    {
        printf("INFO: Magic string verified\n");
        return e_success;
    }
    else
    {
        printf("ERROR: Magic string mismatch — not a stego image\n");
        return e_failure;
    }
}

/*****************************************************
 * Decode extension size
 *****************************************************/
Status decode_secret_extn_size(DecodeInfo *decInfo, int *extn_size)
{
    unsigned char buffer[32];

    fread(buffer, 32, 1, decInfo->fptr_stego_image);
    *extn_size = decode_size_from_lsb(buffer);

    return e_success;
}

/*****************************************************
 * Decode extension string
 *****************************************************/
Status decode_secret_extn(DecodeInfo *decInfo, char *extn, int extn_size)
{
    unsigned char buffer[8];

    for (int i = 0; i < extn_size; i++)
    {
        fread(buffer, 8, 1, decInfo->fptr_stego_image);
        extn[i] = decode_byte_from_lsb((char *)buffer);
    }
    extn[extn_size] = '\0';

    return e_success;
}

/*****************************************************
 * Decode secret file size
 *****************************************************/
Status decode_secret_file_size(DecodeInfo *decInfo, long *fsize)
{
    unsigned char buffer[32];

    fread(buffer, 32, 1, decInfo->fptr_stego_image);
    *fsize = decode_size_from_lsb(buffer);

    return e_success;
}

/*****************************************************
 * Decode file data and write to output file
 *****************************************************/
Status decode_secret_file_data(DecodeInfo *decInfo, long fsize)
{
    unsigned char buffer[8];
    char ch;

    for (long i = 0; i < fsize; i++)
    {
        fread(buffer, 8, 1, decInfo->fptr_stego_image);
        ch = decode_byte_from_lsb((char *)buffer);
        fwrite(&ch, 1, 1, decInfo->fptr_output);
    }

    return e_success;
}

/*****************************************************
 * MASTER DECODING FUNCTION
 *****************************************************/
Status do_decoding(DecodeInfo *decInfo)
{
    /* 1. Decode MAGIC */
    if (decode_magic_string(decInfo) == e_failure)
        return e_failure;

    /* 2. EXTENSION SIZE */
    int extn_size;
    if (decode_secret_extn_size(decInfo, &extn_size) == e_failure)
        return e_failure;

    /* 3. EXTENSION */
    if (decode_secret_extn(decInfo, decInfo->file_extn, extn_size) == e_failure)
        return e_failure;

    /* 4. FILE SIZE */
    long fsize;
    if (decode_secret_file_size(decInfo, &fsize) == e_failure)
        return e_failure;

    /* 5. FILE DATA */
    if (decode_secret_file_data(decInfo, fsize) == e_failure)
        return e_failure;

    printf("INFO: Decode complete! Output file: %s\n", decInfo->output_fname);

    return e_success;
}
