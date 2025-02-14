@echo off
pscp -v -pw raspberry c:\Users\John\Documents\Code\GIT\DiagROMV2\diagrom.rom pi@172.16.19.201:/home/pi
plink -ssh pi@172.16.19.201 -pw raspberry -batch kick/kicksmash32/sw/hostsmash -w /home/pi/diagrom.rom -b 1 -s 3210 -y
plink -ssh pi@172.16.19.201 -pw raspberry -batch kick/kicksmash32/sw/hostsmash -t "prom bank current 1"
plink -ssh pi@172.16.19.201 -pw raspberry -batch kick/kicksmash32/sw/hostsmash -t "reset amiga"
