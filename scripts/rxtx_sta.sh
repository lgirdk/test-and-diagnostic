#start the traffic count
#!/bin/sh
touch /tmp/rxtx_enabled
/usr/ccsp/tad/rxtx_res.sh
/usr/ccsp/tad/rxtx_cur.sh

