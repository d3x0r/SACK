
![MatrixSSL Banner](http://www.matrixssl.org/matrixssl.jpg)

Lightweight Embedded SSL/TLS Implementation

*Official source repository of matrixssl.org*

## Overview
MatrixSSL has been continuously maintained since 2002. It is the first open source small footprint SSL stack. Until recently, releases were tracked on http://freecode.com/projects/matrixssl

MatrixSSL is an embedded SSL and TLS implementation designed for small footprint IoT devices requiring low overhead per connection. The library is less than 50Kb on disk with cipher suites. It includes client and server support through TLS 1.2, mutual authentication, session resumption, and implementations of RSA, ECC, AES, SHA1, SHA-256 and more. The source is well documented and contains portability layers for additional operating systems, cipher suites, and cryptography providers.

## Features
+ < 50KB total footprint with crypto provider
+ SSL 3.0 and TLS 1.0, 1.1 and 1.2 server and client support
+ Included crypto library - RSA, ECC, AES, 3DES, ARC4, SHA1, SHA256, MD5
+ Assembly language optimizations for Intel, ARM and MIPS
+ Session re-keying and cipher renegotiation
+ Full support for session resumption/caching
+ Server Name Indication and Stateless Session Tickets
+ RFC7301 Application Protocol Negotiation
+ Server and client X.509 certificate chain authentication
+ Parsing of X.509 .pem and ASN.1 DER certificate formats
+ PKCS#1.5, PKCS#5 PKCS#8 and PKCS#12 support for key formatting
+ RSASSA-PSS Signature Algorithm support
+ Certificate Revocation List (CRL) support
+ Fully cross platform, portable codebase; minimum use of system calls
+ Pluggable cipher suite interface
+ Pluggable crypto provider interface
+ Pluggable operating system and malloc interface
+ TCP/IP optional
+ Multithreading optional
+ Only a handful of external APIs, all non-blocking
+ Example client and server code included
+ Clean, heavily commented code in portable C
+ User and developer documentation
