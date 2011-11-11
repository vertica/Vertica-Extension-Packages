select version();

\set libfile '\''`pwd`'/build/Encryption.so\'';

CREATE LIBRARY Encryption as :libfile;
CREATE FUNCTION AESEncrypt as language 'C++' name 'AESEncryptFactory' library Encryption;
CREATE FUNCTION AESDecrypt as language 'C++' name 'AESDecryptFactory' library Encryption;


