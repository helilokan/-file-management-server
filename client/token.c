#include "../l8w8jwt/include/l8w8jwt/encode.h"
#include "../l8w8jwt/include/l8w8jwt/decode.h"
#include "worker.h"
#include <func.h>

int encode(char *key, char *user_name, char *token)
{
    char* jwt;
    size_t jwt_length;

    struct l8w8jwt_encoding_params params;
    l8w8jwt_encoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;

    params.sub = user_name;
    // params.iss = "Black Mesa";
    //params.aud = "Administrator";

    params.iat = time(NULL);
    params.exp = time(NULL) + 600; /* Set to expire after 10 minutes (600 seconds). */

    params.secret_key = (unsigned char*)key;
    params.secret_key_length = strlen(params.secret_key);

    params.out = &jwt;
    params.out_length = &jwt_length;

    int r = l8w8jwt_encode(&params);

    // printf("strlen(jwd) = %lu\n", strlen(jwt));
    // printf("\n l8w8jwt example HS512 token: %s \n", r == L8W8JWT_SUCCESS ? jwt : " (encoding failure) ");
    if(r != L8W8JWT_SUCCESS)
    {
        printf("encoding failure\n");
        return EXIT_FAILURE;
    }
    strcpy(token, jwt);
    /* Always free the output jwt string! */
    l8w8jwt_free(jwt);

    return 0;
}