#include "Encryption.h"
#include "mbedtls/md.h"

// must match frontend
#define UID_SALT "smartlock-uid-v1"

String computeHMAC(const String& message, const String& secret) {
    byte hmacResult[32];

    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1); // 1 = use HMAC
    mbedtls_md_hmac_starts(&ctx,
        (const unsigned char*)secret.c_str(), secret.length());
    mbedtls_md_hmac_update(&ctx,
        (const unsigned char*)message.c_str(), message.length());
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);

    String result;
    result.reserve(64);
    for (int i = 0; i < 32; i++) {
        if (hmacResult[i] < 0x10) result += "0";
        result += String(hmacResult[i], HEX);
    }
    return result;
}

String hashUID(const String& uid) {
    // concatenate salt + uid before hashing
    String saltedUid = UID_SALT + uid;

    byte hashResult[32];

    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0); // 0 = plain hash, no HMAC
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx,
        (const unsigned char*)saltedUid.c_str(), saltedUid.length());
    mbedtls_md_finish(&ctx, hashResult);
    mbedtls_md_free(&ctx);

    String result;
    result.reserve(64);
    for (int i = 0; i < 32; i++) {
        if (hashResult[i] < 0x10) result += "0";
        result += String(hashResult[i], HEX);
    }
    return result;
}