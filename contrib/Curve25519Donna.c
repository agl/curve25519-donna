/*
    James Robson
    Public domain.
*/

#include "Curve25519Donna.h"
#include <stdio.h>
#include <stdlib.h>

extern void curve25519_donna(unsigned char *output, const unsigned char *a,
                             const unsigned char *b);

unsigned char*
as_unsigned_char_array(JNIEnv* env, jbyteArray array, int* len);

jbyteArray as_byte_array(JNIEnv* env, unsigned char* buf, int len);


jbyteArray as_byte_array(JNIEnv* env, unsigned char* buf, int len) {
    jbyteArray array = (*env)->NewByteArray(env, len);
    (*env)->SetByteArrayRegion(env, array, 0, len, (jbyte*)buf);

    //int i;
    //for (i = 0;i < len;++i) printf("%02x",(unsigned int) buf[i]); printf(" ");
    //printf("\n");

    return array;
}

unsigned char*
as_unsigned_char_array(JNIEnv* env, jbyteArray array, int* len) {

    *len = (*env)->GetArrayLength(env, array);
    unsigned char* buf = (unsigned char*)calloc(*len+1, sizeof(char));
    (*env)->GetByteArrayRegion (env, array, 0, *len, (jbyte*)buf);
    return buf;

}

JNIEXPORT jbyteArray JNICALL Java_Curve25519Donna_curve25519Donna
  (JNIEnv *env, jobject obj, jbyteArray a, jbyteArray b) {

    unsigned char o[32] = {0};
    int l1, l2;
    unsigned char* a1 = as_unsigned_char_array(env, a, &l1);
    unsigned char* b1 = as_unsigned_char_array(env, b, &l2);

    if ( !(l1 == 32 && l2 == 32) ) {
        fprintf(stderr, "Error, must be length 32");
        return NULL;
    }


    curve25519_donna(o, (const unsigned char*)a1, (const unsigned char*)b1);

    free(a1);
    free(b1);

    return as_byte_array(env, (unsigned char*)o, 32);
}

JNIEXPORT jbyteArray JNICALL Java_Curve25519Donna_makePrivate
  (JNIEnv *env, jobject obj, jbyteArray secret) {

    int len;
    unsigned char* k = as_unsigned_char_array(env, secret, &len);

    if (len != 32) {
        fprintf(stderr, "Error, must be length 32");        
        return NULL;
    }

    k[0] &= 248;
    k[31] &= 127;
    k[31] |= 64;
    return as_byte_array(env, k, 32);
}

JNIEXPORT jbyteArray JNICALL Java_Curve25519Donna_getPublic
  (JNIEnv *env, jobject obj, jbyteArray privkey) {

    int len;
    unsigned char* private = as_unsigned_char_array(env, privkey, &len);

    if (len != 32) {
        fprintf(stderr, "Error, must be length 32");        
        return NULL;
    }

    unsigned char pubkey[32];
    unsigned char basepoint[32] = {9};

    curve25519_donna(pubkey, private, basepoint);
    return as_byte_array(env, (unsigned char*)pubkey, 32);
}

JNIEXPORT jbyteArray JNICALL Java_Curve25519Donna_makeSharedSecret
  (JNIEnv *env, jobject obj, jbyteArray privkey, jbyteArray their_pubkey) {

    unsigned char shared_secret[32];

    int l1, l2;
    unsigned char* private = as_unsigned_char_array(env, privkey, &l1);
    unsigned char* pubkey = as_unsigned_char_array(env, their_pubkey, &l2);

    if ( !(l1 == 32 && l2 == 32) ) {
        fprintf(stderr, "Error, must be length 32");
        return NULL;
    }

    curve25519_donna(shared_secret, private, pubkey);
    return as_byte_array(env, (unsigned char*)shared_secret, 32);
}

JNIEXPORT void JNICALL Java_Curve25519Donna_helowrld
  (JNIEnv *env, jobject obj) {
    printf("helowrld\n");
}
