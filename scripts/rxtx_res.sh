#reset and stop the count 
#!/bin/sh
#ebtables -F;
traffic_count -F;
rm /tmp/eblist;
rm /tmp/rxtx_enabled;
