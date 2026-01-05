#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"
#include "common.h"

int main(int argc, char *argv[])
{
    /* Check minimum number of arguments */
    if (argc < 3)
    {
        printf("Usage (encode): %s -e <input.bmp> <secret.txt> [output_stego.bmp]\n", argv[0]);
        printf("Usage (decode): %s -d <stego.bmp> [output_secret.txt]\n", argv[0]);
        return 1;
    }

    OperationType op_type = check_operation_type(argv);

    /* ============ ENCODE SECTION ============ */
    if (op_type == e_encode)
    {
        printf("INFO: Selected operation: ENCODE\n");
        
        EncodeInfo encInfo;

        if (read_and_validate_encode_args(argv, &encInfo) == e_failure)
        {
            printf("ERROR: Read and validate encode arguments failed\n");
            return 1;
        }

        if (do_encoding(&encInfo) == e_success)
        {
            printf("INFO: Encoding completed successfully!\n");
            return 0;
        }
        else
        {
            printf("ERROR: Encoding failed\n");
            return 1;
        }
    }

    /* ============ DECODE SECTION ============ */
    else if (op_type == e_decode)
    {
        printf("INFO: Selected operation: DECODE\n");

        DecodeInfo decInfo;

        if (read_and_validate_decode_args(argv, &decInfo) == e_failure)
        {
            printf("ERROR: Decode validation failed\n");
            return 1;
        }

        if (open_decode_files(&decInfo) == e_failure)
        {
            printf("ERROR: Opening decode files failed\n");
            return 1;
        }

        if (do_decoding(&decInfo) == e_success)
        {
            printf("INFO: Decoding completed successfully!\n");
            return 0;
        }
        else
        {
            printf("ERROR: Decoding failed\n");
            return 1;
        }
    }

    /* ============ UNSUPPORTED ============ */
    else
    {
        printf("ERROR: Unsupported operation. Use -e or -d\n");
        return 1;
    }
}
