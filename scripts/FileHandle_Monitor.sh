#!/bin/sh

UTOPIA_PATH="/etc/utopia/service.d"
	
source $UTOPIA_PATH/log_env_var.sh
source /etc/log_timestamp.sh

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

file_nr="/proc/sys/fs/file-nr"
nr_open="/proc/sys/fs/nr_open"

echo_t "[RDKB_SELHEAL]Output of file-nr $file_nr `cat $file_nr`"
echo_t "[RDKB_SELHEAL]Output of nr_open $nr_open `cat $nr_open`"
