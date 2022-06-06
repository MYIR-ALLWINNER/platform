#!/bin/sh
#
# Start the preinit
#

add_preinit_to_inittab(){
	if [ -e ${TARGET_DIR}/etc/inittab ]; then
		#insert preinit
		grep "::sysinit:/etc/preinit" ${TARGET_DIR}/etc/inittab >/dev/null
		if [ $? -eq 0 ]; then
			echo "preinit is already in inittab!"
		else
			echo "preinit is not in inittab, add it!"
			sed -i '/Startup the system/a ::sysinit:/etc/preinit' ${TARGET_DIR}/etc/inittab
		fi

		#commented ttyS0, insert /bin/sh
		grep "#ttyS0::respawn:" ${TARGET_DIR}/etc/inittab >/dev/null
		if [ $? -eq 0 ]; then
			echo "ttyS0 is already commented!"
		else
			echo "ttyS0 is not commented, commented it!"
			sed -i 's/ttyS0/#&/' ${TARGET_DIR}/etc/inittab

			grep "::respawn:-/bin/sh" ${TARGET_DIR}/etc/inittab >/dev/null
			if [ $? -eq 0 ]; then
				echo "/bin/sh is already exist!"
			else
				echo "insert ::respawn:-/bin/sh"
				sed -i '/GENERIC_SERIAL/a ::respawn:-/bin/sh' ${TARGET_DIR}/etc/inittab
			fi
		fi
	fi
}

add_preinit_to_inittab