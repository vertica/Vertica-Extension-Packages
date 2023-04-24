select version();

\set libfile '\''`pwd`'/lib/Encryption.so\'';

CREATE LIBRARY Encryption as :libfile;

CREATE FUNCTION AESEncrypt as language 'C++' name 'AESEncryptFactory' library Encryption;
GRANT EXECUTE ON FUNCTION AESEncrypt(varchar, varchar) TO PUBLIC;

CREATE FUNCTION AESDecrypt as language 'C++' name 'AESDecryptFactory' library Encryption;
GRANT EXECUTE ON FUNCTION AESDecrypt(varchar, varchar) TO PUBLIC;


