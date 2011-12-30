from . import _curve25519
from hashlib import sha256
import os

# the curve25519 functions are really simple, and could be used without an
# OOP layer, but it's a bit too easy to accidentally swap the private and
# public keys that way.

def _hash_shared(shared):
    return sha256(b"curve25519-shared:"+shared).digest()

class Private:
    def __init__(self, secret=None, seed=None):
        if secret is None:
            if seed is None:
                secret = os.urandom(32)
            else:
                secret = sha256(b"curve25519-private:"+seed).digest()
        else:
            assert seed is None, "provide secret, seed, or neither, not both"
        if not isinstance(secret, bytes) or len(secret) != 32:
            raise TypeError("secret= must be 32-byte string")
        self.private = _curve25519.make_private(secret)

    def serialize(self):
        return self.private

    def get_public(self):
        return Public(_curve25519.make_public(self.private))

    def get_shared_key(self, public, hashfunc=None):
        if not isinstance(public, Public):
            raise ValueError("'public' must be an instance of Public")
        if hashfunc is None:
            hashfunc = _hash_shared
        shared = _curve25519.make_shared(self.private, public.public)
        return hashfunc(shared)

class Public:
    def __init__(self, public):
        assert isinstance(public, bytes)
        assert len(public) == 32
        self.public = public

    def serialize(self):
        return self.public
