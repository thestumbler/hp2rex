gdbio file.pdb /n >file.cdp
gdbio file.ndb /n >file.cdn
adbdump -c -i an -q2 file.adb >file.cda
hp2rex file -cmn
rexwr file.bin
