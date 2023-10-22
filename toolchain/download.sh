#!/bin/sh
[ -f "$BIN.sig" ] && gpg --verify $BIN.sig $BIN && exit 0
[ -f "$BIN.sig" ] || wget "$URL/$BIN.sig"
[ -f "$BIN" ] && gpg --verify $BIN.sig $BIN && exit 0
wget "$URL/$BIN"
gpg --verify $BIN.sig $BIN && exit 0
rm $BIN
echo "Signature failed"
exit 1
