-------------------------------
INTRODUCTION
-------------------------------

This package contains encryption functions for Vertica.

To that end, my contribution consists of a pair of AES
encryption/decryption functions. This enables field-level encryption
within Vertica using the industry standard AES algorithm.

The remainder of this README will mostly document the AES functions.
I recommend ammending it if/when other functionality is added.

-------------------------------
WHY?
-------------------------------

This has several advantages over encrypting the
entire underlying block device:

1. The data is more protected. Only keyholders have access to the 
   encrypted data. DBAs and SysAdmins don't have to be keyholders.
2. Since most data that one would encrypt is typically high-cardinality
   anyway, this should have a minimal impact on compression and
   performance. Please remember 'minimal' means 'the least amount
   possible for what you get'. It does not mean 'almost no overhead'.
   Encryption still takes some overhead. Nothing is free.
3. Speaking of performance, this will perform exponentially better
   than encrypting the entire block device that stores the Vertica
   data files. Not nearly as much data is being encrypted. Among 
   other things, encrypting the entire device has the side effect
   of performing a decrypt/encrypt during tuple mover operations and
   during backup.
4. Allows the use of different keys for different columns or even
   different fields.

The AES functions themselves were shamelessly taken from MySQL.
I did this for two reasons. The first is that this should allow
Vertica to be compatible with data that was encrypted in a MySQL
database (though I haven't tried this). The second reason is that
is was easy, and the functions are freely available. 

-------------------------------
ROADMAP
-------------------------------

I plan to further extend this package to include:

1. DES encryption/decryption
2. Differing key lengths of AES in the form of AES256Encrypt, etc...
3. base64 encoding/decoding of strings

No promises on when.

One thing that might be nice within Vertica is to be able to
put ACLs on virtual tables. This would allow an administrator to
embed a key inside a call to AESDecrypt that's contained within a 
view without compromising the key itself. You can obviously do
this today, but the key would be exposed with a simple
'select ..from views'.

-------------------------------
INSTALLING / UNINSTALLING
-------------------------------

You can install by running the SQL statements in
 src/ddl/install.sql 
or, to uninstall,
 src/ddl/uninstall.sql
Note that the SQL statements assume that you have copied this package to a 
node in	your cluster and are running them from there.

Alternatively, assuming you can run vsql, just do:

$ make install
$ make uninstall

-------------------------------
BUILDING
-------------------------------

To build from source:

$ make

By default, there's a set key-length of 128 bits. This isn't the
most secure AES, but it's the fastest. You can change:
#DEFINE AES_KEY_LENGTH 128
in src/third-party/include/my_aes.h to 192 or 256 bits if you're
inclined. I kept the 128 bit default to be the same as MySQL for
compatibility.


-------------------------------
USAGE
-------------------------------
 
Usage is very simple, after installation you have two functions available:

 AESEncrypt(input,passphrase) - Returns an encrypted binary string 
 AESDecrypt(input,passphrase) - Returns the decrypted string 

To use them, just pass them some data and a passphrase. Both of the inputs
must be strings. For example:

 select AESEncrypt('Secret string','password');

Please see a few more example in the examples/samples.sql

It should also encrypt binary data stored in varchars. Specifically, this
means that strings containing nulls are not truncated and should work
properly.

-------------------------------
PERFORMANCE
-------------------------------

I don't know. It seems fast enough. I can do:
UPDATE t1 set field=AESEncrypt(field,'password');
where 'field' is a VARCHAR(12) across 6803932 rows in about 30 seconds, but
that's with really slow disks (or disk, rather.. there's only one). Another
data point:

4-5 seconds from warmed up cache:
\o /dev/null
SELECT field FROM t1;

13-14 seconds:
\o /dev/null
SELECT AESDecrypt(field,'passphrease') from t1;

But I think that's only pegging one core on a VM. :) If someone gets
some numbers from a real cluster, I'd love to see them and add them here.

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt
