#!/bin/sh

count=0
while [ ! -e /dev/by-name/extend ] && [ ! -e /dev/by-name/recovery ] && [ $count -lt 10 ]
do
	let count+=1
	sleep 1
done

count=0
while [ ! -e /dev/mnt/UDISK ] && [ $count -lt 10 ]
do
       let count+=1
       sleep 1
done

if [ -e /dev/by-name/extend ];then
    /sbin/aw_upgrade_process.sh 1>/dev/console 2>/dev/console
else
    /sbin/aw_upgrade_normal.sh 1>/dev/console 2>/dev/console

fi
