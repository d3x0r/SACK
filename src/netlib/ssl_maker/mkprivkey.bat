set OPENSSL_CONF=openssl.cnf

openssl genrsa -des3 -out privkey.enc.key 4096

openssl rsa -in privkey.enc.key -out myprivkey.key -outform DER
openssl rsa -in privkey.enc.key -out myprivkey.pem -outform PEM

openssl req -new -key privkey.enc.key -out server.csr
openssl x509 -req -days 36500 -in server.csr -signkey myprivkey.pem -out server.crt -outform der

