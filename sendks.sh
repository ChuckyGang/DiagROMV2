#!/bin/bash

PI_HOST="172.16.19.201"
PI_USER="pi"
PI_PASS="raspberry"
ROM_FILE="$(dirname "$0")/diagrom.rom"
HOSTSMASH="/home/kick/kicksmash/sw/hostsmash"
SSH_OPTS="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"

# Copy ROM to Pi
sshpass -p "$PI_PASS" scp $SSH_OPTS "$ROM_FILE" "${PI_USER}@${PI_HOST}:/home/pi/"

# Write ROM to KickSmash (bank 3)
sshpass -p "$PI_PASS" ssh $SSH_OPTS "${PI_USER}@${PI_HOST}" "$HOSTSMASH -w /home/pi/diagrom.rom -b 3 -s 3210 -y"

# Set current prom bank to 3
sshpass -p "$PI_PASS" ssh $SSH_OPTS "${PI_USER}@${PI_HOST}" "$HOSTSMASH -t 'prom bank current 3'"

# Reset Amiga
sshpass -p "$PI_PASS" ssh $SSH_OPTS "${PI_USER}@${PI_HOST}" "$HOSTSMASH -t 'reset amiga'"
