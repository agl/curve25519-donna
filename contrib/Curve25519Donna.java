/*
    James Robson
    Public domain.
*/

public class Curve25519Donna {

    final protected static char[] hexArray = "0123456789ABCDEF".toCharArray();

    public static String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        int v;
        for ( int j = 0; j < bytes.length; j++ ) {
            v = bytes[j] & 0xFF;
            hexChars[j * 2] = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0F];
        }
        return new String(hexChars);
    }

    public native byte[] curve25519Donna(byte[] a, byte[] b);
    public native byte[] makePrivate(byte[] secret);
    public native byte[] getPublic(byte[] privkey);
    public native byte[] makeSharedSecret(byte[] privkey, byte[] theirPubKey);
    public native void helowrld();

    // Uncomment if your Java is 32-bit:
    //static { System.loadLibrary("Curve25519Donna"); }

    // Otherwise, load this 64-bit .jnilib:
    static { System.loadLibrary("Curve25519Donna_64"); }

    /*
        To give the old tires a kick (OSX):
        java -cp `pwd` Curve25519Donna
    */
    public static void main (String[] args) {

        Curve25519Donna c = new Curve25519Donna();

        // These should be 32 bytes long
        byte[] user1Secret = "abcdefghijklmnopqrstuvwxyz123456".getBytes();
        byte[] user2Secret = "654321zyxwvutsrqponmlkjihgfedcba".getBytes();


        // You can use the curve function directly...

        //byte[] o = c.curve25519Donna(a, b);
        //System.out.println("o = " + bytesToHex(o));


        // ... but it's not really necessary. Just use the following
        // convenience methods:

        byte[] privKey = c.makePrivate(user1Secret);
        byte[] pubKey = c.getPublic(privKey);

        byte[] privKey2 = c.makePrivate(user2Secret);
        byte[] pubKey2 = c.getPublic(privKey2);

        System.out.println("'user1' privKey = " + bytesToHex(privKey));
        System.out.println("'user1' pubKey = " + bytesToHex(pubKey));
        System.out.println("===================================================");

        System.out.println("'user2' privKey = " + bytesToHex(privKey2));
        System.out.println("'user2' pubKey = " + bytesToHex(pubKey2));
        System.out.println("===================================================");


        byte[] ss1 = c.makeSharedSecret(privKey, pubKey2);
        System.out.println("'user1' computes shared secret: " + bytesToHex(ss1));

        byte[] ss2 = c.makeSharedSecret(privKey2, pubKey);
        System.out.println("'user2' computes shared secret: " + bytesToHex(ss2));

    }
}
