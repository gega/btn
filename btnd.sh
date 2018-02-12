#!/bin/bash

# server on udp port 4423
# process incoming messages:
# 106:0,1,0 106:1,0,0 
# 106:0,1,0 106:0,0,1 106:0,0,2 106:0,0,3 106:0,0,4 106:0,0,5 106:0,0,6 106:0,0,7 106:1,0,0
# left: 105, right: 106, center: 28, up: 103, down: 108
# format: button:released,pressed,long-pressed
# action: mpc -h 10.0.1.2 volume +5

PORT=4423
HOME="/home/user"
LOGF="$HOME/btnd.log"


if [ "$1" == "-d" ]; then
  CMDLINE="d"
else
  read -t 2 CMDLINE
  if [ ${#CMDLINE} -lt 2 ]; then
    CMDLINE=""
  fi
fi


function cmd_volume()
{
  local MOD=$1
  local PARAM=""
  
  [[ "$MOD" == "dec" ]] && PARAM="-5"
  [[ "$MOD" == "inc" ]] && PARAM="+5"
  if [ "$PARAM" != "" ]; then
    mpc -h 10.0.1.2 volume $PARAM
    echo "==== $(date) command volume $MOD" >>$LOGF
  fi
}


function cmd_playtoggle()
{
  mpc -h 10.0.1.2 toggle
  echo "==== $(date) command toggle" >>$LOGF
}


function cmd_lights()
{
  bridge=$(cat $HOME/.hueplkey |cut -d' ' -f1)
  ping -nqc 2 $bridge >/dev/null
  if [ $? -ne 0 ]; then
    bridge=$(nmap -sP 10.0.1.255/24 > /dev/null; arp -na | grep "at 00:17:88"|tr '()' ','|awk -F, '{print $2}')
    key=$(cat $HOME/.hueplkey |cut -d' ' -f2)
    echo "$bridge $key" >$HOME/.hueplkey
  fi
  ACTION=$1
  if [ "$ACTION" == "false" ]; then
    curl -X PUT http://$bridge/$key/groups/0/action -d '{"on": false}' &>/dev/null
    echo "==== $(date) command lights OFF [$?]" >>$LOGF
  elif [ "$ACTION" == "true" ]; then
    echo "==== $(date) command light ON [unsupported]" >>$LOGF
  else
    echo "---- $(date) invalid lights action" >>$LOGF
  fi
}


function proto()
{
  local CL=$*
  local BTN=$(echo "$CL"|cut -d ":" -f 1)
  local PAR=$(echo "$CL"|cut -d ":" -f 2-)
  local RELEASED=$(echo "$PAR"|cut -d "," -f 1)
  local PRESSED=$(echo "$PAR"|cut -d "," -f 2)
  local LONGPRESS=$(echo "$PAR"|cut -d "," -f 3)

  echo $CL
  echo $BTN
  case "$BTN" in
    105)         # LEFT
      let DOWN=LONGPRESS+RELEASED
      if [ $DOWN -gt 0 ]; then
        cmd_volume dec
      fi
      ;;
    106)         # RIGHT
      let DOWN=LONGPRESS+RELEASED
      if [ $DOWN -gt 0 ]; then
        cmd_volume inc
      fi
      ;;
    28)          # CENTER
      cmd_playtoggle
      ;;
    103)         # UP
      cmd_lights "true"
      ;;
    108)         # DOWN
      cmd_lights "false"
      ;;
    *)
      echo "ERR Invalid command"
      echo "---- $(date) Invalid command '$CMD'" >>$LOGF
      ;;
  esac
}


function main()
{
  while [ true ]; do
    C=$(nc6 -l -u -p $PORT -q 1 -e $1)
  done
}

if [ "$CMDLINE" == "d" ]; then
  echo "btnd server started, listening on port $PORT"
  echo "==== $(date) btnd server started, listening on port $PORT" >>$LOGF
  main "$0"
elif [ "$CMDLINE" == "" ]; then
  echo "---- $(date) missing parameter" >>$LOGF
else
  proto "$CMDLINE"
fi
