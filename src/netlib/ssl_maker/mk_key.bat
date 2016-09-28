set OPENSSL_CONF=openssl.cnf

:openssl genrsa -des3 -out server.enc.key 1024

:openssl req -new -key server.enc.key -out server.csr

:openssl rsa -in server.enc.key -out server.key

:set OPENSSL_CONF=c:\general\tools\unix\openssl32\certca\cacnf.cnf
openssl x509 -req -days 36500 -in server.csr -signkey server.key -out server.crt
