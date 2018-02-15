select version();

\set libfile '\''`pwd`'/lib/Encryption.so\'';

CREATE OR REPLACE LIBRARY Encryption as :libfile;
CREATE OR REPLACE FUNCTION AESEncrypt as language 'C++' name 'AESEncryptFactory' library Encryption;
CREATE OR REPLACE FUNCTION AESDecrypt as language 'C++' name 'AESDecryptFactory' library Encryption;


