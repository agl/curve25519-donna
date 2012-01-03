#! /usr/bin/python

import unittest

from curve25519 import Private, Public
from hashlib import sha1, sha256
from binascii import hexlify

class Basic(unittest.TestCase):
    def test_basic(self):
        secret1 = b"abcdefghijklmnopqrstuvwxyz123456"
        self.assertEqual(len(secret1), 32)

        secret2 = b"654321zyxwvutsrqponmlkjihgfedcba"
        self.assertEqual(len(secret2), 32)
        priv1 = Private(secret=secret1)
        pub1 = priv1.get_public()
        priv2 = Private(secret=secret2)
        pub2 = priv2.get_public()
        shared12 = priv1.get_shared_key(pub2)
        e = b"b0818125eab42a8ac1af5e8b9b9c15ed2605c2bbe9675de89e5e6e7f442b9598"
        self.assertEqual(hexlify(shared12), e)
        shared21 = priv2.get_shared_key(pub1)
        self.assertEqual(shared12, shared21)

        pub2a = Public(pub2.serialize())
        shared12a = priv1.get_shared_key(pub2a)
        self.assertEqual(hexlify(shared12a), e)

    def test_errors(self):
        priv1 = Private()
        self.assertRaises(ValueError, priv1.get_shared_key, priv1)

    def test_seed(self):
        # use 32-byte secret
        self.assertRaises(TypeError, Private, secret=123)
        self.assertRaises(TypeError, Private, secret=b"too short")
        secret1 = b"abcdefghijklmnopqrstuvwxyz123456"
        assert len(secret1) == 32
        priv1 = Private(secret=secret1)
        priv1a = Private(secret=secret1)
        priv1b = Private(priv1.serialize())
        self.assertEqual(priv1.serialize(), priv1a.serialize())
        self.assertEqual(priv1.serialize(), priv1b.serialize())
        e = b"6062636465666768696a6b6c6d6e6f707172737475767778797a313233343576"
        self.assertEqual(hexlify(priv1.serialize()), e)

        # the private key is a clamped form of the secret, so they won't
        # quite be the same
        p = Private(secret=b"\x00"*32)
        self.assertEqual(hexlify(p.serialize()), b"00"*31+b"40")
        p = Private(secret=b"\xff"*32)
        self.assertEqual(hexlify(p.serialize()), b"f8"+b"ff"*30+b"7f")

        # use arbitrary-length seed
        self.assertRaises(TypeError, Private, seed=123)
        priv1 = Private(seed=b"abc")
        priv1a = Private(seed=b"abc")
        priv1b = Private(priv1.serialize())
        self.assertEqual(priv1.serialize(), priv1a.serialize())
        self.assertEqual(priv1.serialize(), priv1b.serialize())
        self.assertRaises(AssertionError, Private, seed=b"abc", secret=b"no")

        priv1 = Private(seed=b"abc")
        priv1a = Private(priv1.serialize())
        self.assertEqual(priv1.serialize(), priv1a.serialize())
        self.assertRaises(AssertionError, Private, seed=b"abc", secret=b"no")

        # use built-in os.urandom
        priv2 = Private()
        priv2a = Private(priv2.private)
        self.assertEqual(priv2.serialize(), priv2a.serialize())

        # attempt to use both secret= and seed=, not allowed
        self.assertRaises(AssertionError, Private, seed=b"abc", secret=b"no")

    def test_hashfunc(self):
        priv1 = Private(seed=b"abc")
        priv2 = Private(seed=b"def")
        shared_sha256 = priv1.get_shared_key(priv2.get_public())
        e = b"da959ffe77ebeb4757fe5ba310e28ede425ae0d0ff5ec9c884e2d08f311cf5e5"
        self.assertEqual(hexlify(shared_sha256), e)

        # confirm the hash function remains what we think it is
        def myhash(shared_key):
            return sha256(b"curve25519-shared:"+shared_key).digest()
        shared_myhash = priv1.get_shared_key(priv2.get_public(), myhash)
        self.assertEqual(hexlify(shared_myhash), e)

        def hexhash(shared_key):
            return sha1(shared_key).hexdigest().encode()
        shared_hexhash = priv1.get_shared_key(priv2.get_public(), hexhash)
        self.assertEqual(shared_hexhash,
                             b"80eec98222c8edc4324fb9477a3c775ce7c6c93a")


if __name__ == "__main__":
    unittest.main()

