#!/bin/bash

/usr/bin/openssl genrsa -out privkey.pem 2048
/usr/bin/openssl req -new -x509 -key privkey.pem -out cacert.pem -days 1825 -config cacert.config
chmod go-rwx *.pem
