@echo off
pscp -v -pw raspberry c:\Users\John\Documents\Code\GIT\DiagROMV2\diagrom.rom pi@172.16.19.201:/home/pi
plink -ssh pi@172.16.19.201 -pw raspberry -batch /home/kick/kicksmash/sw/hostsmash -w /home/pi/diagrom.rom -b 7 -s 3210 -y
plink -ssh pi@172.16.19.201 -pw raspberry -batch /home/kick/kicksmash/sw/hostsmash -t "prom bank current 7"
plink -ssh pi@172.16.19.201 -pw raspberry -batch /home/kick/kicksmash/sw/hostsmash -t "reset amiga"
