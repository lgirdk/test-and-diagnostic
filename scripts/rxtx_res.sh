#reset and stop the count 
#!/bin/sh
ebtables -F;
rm /tmp/eblist;
rm /tmp/rxtx_enabled;
