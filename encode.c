#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include "common.h"

/*===========================================================
 * FUNCTION NAME : check_operation_type
 * PURPOSE       : Reads argv[1] to decide whether user wants
 *                  ENCODING (-e) or DECODING (-d)
 * RETURN        : e_encode, e_decode or e_unsupported
 *===========================================================*/
OperationType check_operation_type(char *argv[])
{
    /* Check if user typed -e for encoding */
    if (strcmp(argv[1], "-e") == 0)
    {
        return e_encode;
    }
    /* Check if user typed -d for decoding */
    else if (strcmp(argv[1], "-d") == 0)
    {
        return e_decode;
    }
    /* If user typed anything else */
    else
    {
        return e_unsupported;
    }
}

/*===========================================================
 * FUNCTION NAME : read_and_validate_encode_args
 * PURPOSE       : Validates command line arguments for encoding
 * EXPECTED ARGS : 
 *      argv[2] = source BMP file
 *      argv[3] = secret .txt file
 *      argv[4] = optional output BMP (stego image)
 *===========================================================*/
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    /* Check mandatory args present */
    if (argv[2] == NULL || argv[3] == NULL)
    {
        printf("ERROR: Missing required files\n");
        printf("Usage: %s -e <input.bmp> <secret.txt> [output_stego.bmp]\n", argv[0]);
        return e_failure;
    }

    /* Validate Source BMP image – it must contain .bmp extension */
    char *ext = strstr(argv[2], ".bmp");
    if (ext == NULL || strcmp(ext, ".bmp") != 0)
    {
        printf("ERROR: Source file must have .bmp extension\n");
        return e_failure;
    }
    encInfo->src_image_fname = argv[2];

    /* Validate secret file – here we only allow .txt for simplicity */
    ext = strstr(argv[3], ".txt");
    if (ext == NULL || strcmp(ext, ".txt") != 0)
    {
        printf("ERROR: Secret file must have .txt extension\n");
        return e_failure;
    }
    encInfo->secret_fname = argv[3];

    /* Extract the extension from secret file name (ex: ".txt") */
    {
        char *dot = strrchr(argv[3], '.');
        if (dot == NULL)
        {
            printf("ERROR: Secret file has no extension\n");
            return e_failure;
        }
        /* Store extension inside structure */
        strncpy(encInfo->extn_secret_file, dot, MAX_FILE_SUFFIX - 1);
        encInfo->extn_secret_file[MAX_FILE_SUFFIX - 1] = '\0';
    }

    /* Handle output file name */
    if (argv[4] != NULL)    /* If user supplied output file name */
    {
        ext = strstr(argv[4], ".bmp");
        if (ext == NULL || strcmp(ext, ".bmp") != 0)
        {
            printf("ERROR: Output file must have .bmp extension\n");
            return e_failure;
        }
        encInfo->stego_image_fname = argv[4];
    }
    else    /* If user did not specify output name */
    {
        encInfo->stego_image_fname = "stego.bmp";  /* Default */
        printf("INFO: Output file not provided. Using default: stego.bmp\n");
    }

    return e_success;
}

/*===========================================================
 * FUNCTION NAME : open_files
 * PURPOSE       : Open 3 files required during encoding:
 *                  1. Source BMP image (read mode)
 *                  2. Secret text file (read mode)
 *                  3. Stego(BMP) output image (write mode)
 *===========================================================*/
Status open_files(EncodeInfo *encInfo)
{
    /* Open source image for reading */
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open source BMP %s\n", encInfo->src_image_fname);
        return e_failure;
    }

    /* Open secret file for reading */
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open secret file %s\n", encInfo->secret_fname);
        return e_failure;
    }

    /* Open output Stego BMP for writing (this stores hidden content) */
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open stego image %s\n", encInfo->stego_image_fname);
        return e_failure;
    }

    return e_success;
}

/*===========================================================
 * FUNCTION NAME : get_image_size_for_bmp
 * PURPOSE       : Reads width and height from BMP header
 *                 starting at byte 18 and 22.
 * RETURN VALUE  : Capacity in bytes (width * height * 3)
 ===========================================================*/
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;

    /* Move to byte 18 where width is stored inside BMP */
    fseek(fptr_image, 18, SEEK_SET);

    /* Read width & height (each 4 bytes) */
    fread(&width, sizeof(int), 1, fptr_image);
    fread(&height, sizeof(int), 1, fptr_image);

    /* Restore pointer back to start of file */
    fseek(fptr_image, 0, SEEK_SET);

    /* Total bytes = pixels * 3 (R,G,B for 24-bit image) */
    return width * height * 3;
}

/*===========================================================
 * FUNCTION NAME : get_file_size
 * PURPOSE       : Get number of bytes inside secret file
 ===========================================================*/
uint get_file_size(FILE *fptr)
{
    long size;

    fseek(fptr, 0, SEEK_END);
    size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    return (uint)size;
}

/*===========================================================
 * FUNCTION NAME : check_capacity
 * PURPOSE       : Make sure source BMP has enough capacity
 *                 to store:
 *                   - magic string
 *                   - extension size
 *                   - extension name
 *                   - secret file size
 *                   - entire secret data
 ===========================================================*/
Status check_capacity(EncodeInfo *encInfo)
{
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);

    int magic_len = strlen(MAGIC_STRING);
    int extn_len  = strlen(encInfo->extn_secret_file);

    /* Total required bits */
    uint required_bits =
            (magic_len +      /* magic string */
             4 +              /* extension size (int) */
             extn_len +       /* extension chars */
             4 +              /* secret file size (int) */
             encInfo->size_secret_file) * 8; /* data bytes */

    /* Compare capacity vs requirement */
    if (encInfo->image_capacity < required_bits)
    {
        printf("ERROR: Image is too small to store secret data\n");
        return e_failure;
    }

    printf("INFO: Image capacity is sufficient\n");
    return e_success;
}

/*===========================================================
 * FUNCTION NAME : copy_bmp_header
 * PURPOSE       : First 54 bytes of BMP must be copied
 *                 exactly (not modified)
 ===========================================================*/
Status copy_bmp_header(FILE *fptr_src, FILE *fptr_dest)
{
    unsigned char header[54];

    fseek(fptr_src, 0, SEEK_SET);
    fread(header, 1, 54, fptr_src);
    fwrite(header, 1, 54, fptr_dest);

    return e_success;
}

/*===========================================================
 * Encode functions:
 *   - magic string
 *   - extension size
 *   - extension characters
 *   - file size
 *   - file data bytes
 -----------------------------------------------------------*/

/* Encodes a single character using 8 bytes (LSB method) */
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for (int i = 0; i < 8; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((data >> (7 - i)) & 1);
    }
    return e_success;
}

/* Encodes a 32-bit numeric value using LSB in 32 bytes */
Status encode_size_to_lsb(long value, unsigned char *buffer)
{
    for (int i = 0; i < 32; i++)
    {
        buffer[i] = (buffer[i] & 0xFE) | ((value >> (31 - i)) & 1);
    }
    return e_success;
}

/* Encode magic string */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    unsigned char buffer[8];
    int len = strlen(magic_string);

    for (int i = 0; i < len; i++)
    {
        fread(buffer, 8, 1, encInfo->fptr_src_image);
        encode_byte_to_lsb(magic_string[i], (char *)buffer);
        fwrite(buffer, 8, 1, encInfo->fptr_stego_image);
    }

    return e_success;
}

/* Encode extension size (stored as integer value) */
Status encode_secret_file_extn_size(int extn_size, EncodeInfo *encInfo)
{
    unsigned char buffer[32];

    fread(buffer, 32, 1, encInfo->fptr_src_image);
    encode_size_to_lsb(extn_size, buffer);
    fwrite(buffer, 32, 1, encInfo->fptr_stego_image);

    return e_success;
}

/* Encode extension characters (e.g., ".txt") */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    unsigned char buffer[8];
    int len = strlen(file_extn);

    for (int i = 0; i < len; i++)
    {
        fread(buffer, 8, 1, encInfo->fptr_src_image);
        encode_byte_to_lsb(file_extn[i], (char *)buffer);
        fwrite(buffer, 8, 1, encInfo->fptr_stego_image);
    }

    return e_success;
}

/* Encode secret file size */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    unsigned char buffer[32];

    fread(buffer, 32, 1, encInfo->fptr_src_image);
    encode_size_to_lsb(file_size, buffer);
    fwrite(buffer, 32, 1, encInfo->fptr_stego_image);

    return e_success;
}

/* Encode entire secret file content byte-by-byte */
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    unsigned char buffer[8];
    char ch;

    /* Read secret file character-by-character */
    while (fread(&ch, 1, 1, encInfo->fptr_secret))
    {
        fread(buffer, 8, 1, encInfo->fptr_src_image);
        encode_byte_to_lsb(ch, (char *)buffer);
        fwrite(buffer, 8, 1, encInfo->fptr_stego_image);
    }

    return e_success;
}

/* Write all leftover image bytes without encoding */
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fptr_src)) > 0)
    {
        fwrite(buffer, 1, bytes_read, fptr_dest);
    }

    return e_success;
}

/*===========================================================
 * Top-level encoding:
 * Performs the whole encoding pipeline step-by-step
 ===========================================================*/
Status do_encoding(EncodeInfo *encInfo)
{
    /* Open required input/output files */
    if (open_files(encInfo) == e_failure)
        return e_failure;

    /* Check if image is large enough */
    if (check_capacity(encInfo) == e_failure)
        return e_failure;

    /* Copy BMP header unmodified */
    printf("INFO: Copying BMP header...\n");
    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
        return e_failure;

    /* Encode magic string */
    printf("INFO: Encoding Magic String...\n");
    if (encode_magic_string(MAGIC_STRING, encInfo) == e_failure)
        return e_failure;

    int ext_size = strlen(encInfo->extn_secret_file);

    /* Encode extension metadata */
    printf("INFO: Encoding Secret File Extension Size...\n");
    if (encode_secret_file_extn_size(ext_size, encInfo) == e_failure)
        return e_failure;

    printf("INFO: Encoding Secret File Extension...\n");
    if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_failure)
        return e_failure;

    /* Encode size of secret text */
    printf("INFO: Encoding Secret File Size...\n");
    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)
        return e_failure;

    /* Encode the actual file data */
    printf("INFO: Encoding Secret File Data...\n");
    if (encode_secret_file_data(encInfo) == e_failure)
        return e_failure;

    /* Copy remaining pixels */
    printf("INFO: Copying Remaining Image Data...\n");
    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
        return e_failure;

    printf("INFO: Encoding completed successfully.\n");
    return e_success;
}
