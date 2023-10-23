#!/bin/bash

genpwd() {
  makepasswd -c 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz023456789`~!@#$%^&*()-_=+.,/|[]{};:<>?\'
}

genkey() {
  out="$1"
  if [ $# -ge 2 -a "$2" == y ]; then
    pwd=$(printf -- '-aes256 -pass pass:%s' "$(genpwd)")
  else
    pwd=''
  fi
  openssl genpkey -algorithm RSA -outform PEM -out "$out" -pkeyopt rsa_keygen_bits:2048 $pwd >& /dev/null
  echo $pwd | sed 's/.*pass://'
}

gencsr() {
  cakey="$1"
  caroot="$2"
  if [ $# -ge 3 ]; then
    pwd=$(printf -- '-passin pass:%s' "$3")
  else
    pwd=''
  fi
  openssl req -x509 -new -nodes -extensions v3_ca -key "$cakey" -days 3654 -out "$caroot" -sha512 $pwd -subj '/CN=XX/'
}

if [ -d ca ]; then
  cd ca
fi

capwd=$(genkey cakey.pem n)

gencsr cakey.pem caroot.pem "$capwd"

printf '%s\n' "$capwd" > capwd.txt
chmod go-rw capwd.txt
