The main idea of project - create new protocol to replace TCP 
in networks with high loss, jitter and other negative phenomena.

Main "engine" of protocol is physical channel.

![Channel structure](documentation/images/Channels.drawio.png)


One connection may include many physical channels.
Every channel may be launched on different ports or
on different interfaces.
That structure supports using LTE and WI-FI or some 
modems at the same time.

Your application will operate with logical channels.
Logical channel may work in "TCP-like" mode or in 
"Block operating" mode.

In "TCP-like" mode logic channel will pretend TCP socket with
"send" and "recv" methods. Connection will automatically slice
your data to blocks and share it among physical channels.

In "Block operating" mode logic channel receives blocks from 
application and sends it with free or reserved physical 
channel.






