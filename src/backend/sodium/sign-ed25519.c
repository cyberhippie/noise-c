/*
 * Copyright (C) 2016 Southern Storm Software, Pty Ltd.
 * Copyright (C) 2016 Topology LP.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "internal.h"
#include <sodium.h>

typedef struct
{
    struct NoiseSignState_s parent;
    union {
        struct {
            uint8_t private_key[32];
            uint8_t public_key[32];
        };
        uint8_t skpk[crypto_sign_ed25519_SECRETKEYBYTES]; // sodium uses this format
    };

} NoiseEd25519State;

static void noise_ed25519_generate_keypair(NoiseSignState *state)
{
    NoiseEd25519State *st = (NoiseEd25519State *)state;
    uint8_t temp_pk[crypto_sign_ed25519_PUBLICKEYBYTES];
    crypto_sign_ed25519_keypair(temp_pk, st->skpk);
}

static int noise_ed25519_validate_keypair
        (const NoiseSignState *state, const uint8_t *private_key,
         const uint8_t *public_key)
{
    /* Check that the public key actually corresponds to the private key */
    uint8_t temp[crypto_sign_ed25519_PUBLICKEYBYTES];
    uint8_t temp_sk[crypto_sign_ed25519_SECRETKEYBYTES];
    int equal;
    crypto_sign_ed25519_seed_keypair(temp, temp_sk, private_key);
    equal = noise_is_equal(temp, public_key, sizeof(temp));
    return NOISE_ERROR_INVALID_PUBLIC_KEY & (equal - 1);
}

static int noise_ed25519_validate_public_key
        (const NoiseSignState *state, const uint8_t *public_key)
{
    /* Nothing to do here yet */
    return NOISE_ERROR_NONE;
}

static int noise_ed25519_derive_public_key
        (const NoiseSignState *state, const uint8_t *private_key,
         uint8_t *public_key)
{
    uint8_t temp_sk[crypto_sign_ed25519_SECRETKEYBYTES];
    crypto_sign_ed25519_seed_keypair(public_key, temp_sk, private_key);
    return NOISE_ERROR_NONE;
}

static int noise_ed25519_sign
        (const NoiseSignState *state, const uint8_t *message,
         size_t message_len, uint8_t *signature)
{
    const NoiseEd25519State *st = (const NoiseEd25519State *)state;
    crypto_sign_ed25519_detached(signature, NULL, message, message_len, st->skpk);
    return NOISE_ERROR_NONE;
}

static int noise_ed25519_verify
        (const NoiseSignState *state, const uint8_t *message,
         size_t message_len, const uint8_t *signature)
{
    const NoiseEd25519State *st = (const NoiseEd25519State *)state;
    int result = crypto_sign_ed25519_verify_detached(signature,
            message, message_len, st->public_key);
    return result ? NOISE_ERROR_INVALID_SIGNATURE : NOISE_ERROR_NONE;
}

NoiseSignState *noise_ed25519_new(void)
{
    NoiseEd25519State *state = noise_new(NoiseEd25519State);
    if (!state)
        return 0;
    state->parent.sign_id = NOISE_SIGN_ED25519;
    state->parent.private_key_len = 32;
    state->parent.public_key_len = 32;
    state->parent.signature_len = 64;
    state->parent.private_key = state->private_key;
    state->parent.public_key = state->public_key;
    state->parent.generate_keypair = noise_ed25519_generate_keypair;
    state->parent.validate_keypair = noise_ed25519_validate_keypair;
    state->parent.validate_public_key = noise_ed25519_validate_public_key;
    state->parent.derive_public_key = noise_ed25519_derive_public_key;
    state->parent.sign = noise_ed25519_sign;
    state->parent.verify = noise_ed25519_verify;
    return &(state->parent);
}
