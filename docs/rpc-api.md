RPC API Documentation
---------------------

This file contains descriptions of objects and methods that are available over
RPC interface. 

TODO: complete this documentation. 

This interface is from the begining meant to seamlessly replace backend
interface for JUCI webgui. Therefore, method names and general structure of the
interface is for now kept compatible with the old RPC interface for JUCI.
However it would be very benefitial for this interface to be refactored with
time into a more generic and powerful rpc interface for OpenWRT.  

UCI 
---

This is central interface to all configuration options on the system. Current
implementation forwards these calls directly to corresponding ubus calls. 


```javascript 
{
	"uci": {
		"add": [],
		"revert": [],
		"commit": [],
		"rollback": [],
		"set": [],
		"apply": [],
		"configs": [],
		"delete": [],
		"get": []
	}
}
```

DDNS
----

```javascript
{
	"juci.ddns": {
		"providers": []
	}
}
```

`providers`

This method returns list of supported DDNS providers as reported in
`/usr/lib/ddns/services` file. 

Response: 

```javascript
{
	"result": [ ... ]
}
```

DHCP Server Interface
---------------------

Access DHCP lease information. 

```javascript
{
	"juci.dhcp": {
		"ipv6leases": [],
		"ipv4leases": []
	}
}
```

`ipv6leases`: return ipv4 dhcp leases

Response: 

```javascript
{
	"leases": [{
		device: "device name (such as , 
		duid = "ipv6 duid", 
		iaid = "ipv6 interface id", 
		hostname = "client hostname", 
		leasetime = "lease time remaining", 
		id = "id of the lease", 
		length = "length?", 
		ip6addr = "ipv6 address assigned to client"
	}]
}
```

`ipv4leases`: return list of ipv4 leases

Response: 

```javascript
{
	"<device mac>": {
		leasetime: "time left for this lease", 
		ipaddr: "ip address assigned to the client", 
		hostname: "hostname of the client", 
		macaddr: "mac address of the device"
	}
}
```

Network Diagnostics
-------------------

These functions allow you to do pings or traceroutes (simple diagnostics). 

```javascript
{	
	"juci.diagnostics": {
		"traceroute6": [],
		"ping": [],
		"ping6": [],
		"traceroute": []
	}
}
```

Dropbear SSH Interface
----------------------

```javascript
{
	"juci.dropbear": {
		"remove_public_key": [],
		"get_public_keys": [],
		"add_public_key": []
	}
}
```

Layer2 Ethernet Devices (ip)
----------------------------

```javascript
{
	"juci.ethernet": {
		"adapters": []
	}
}
```

Firewall
--------

Various helper functions for firewall configuration. 

```javascript
{
	"juci.firewall.dmz": {
		"excluded_ports": []
	}
}
```

IPTV
----

```javascript
{
	"juci.iptv": {
		"igmptable": []
	}
}
```

Logs
----

```javascript
{
	"juci.logs": {
		"download": []
	}
}
```

MAC-address identification
--------------------------

```javascript
{
	"juci.macdb": {
		"lookup": []
	}
}
```

MiniDLNA
--------

```javascript
{
	"juci.minidlna": {
		"autocomplete": [],
		"folder_tree": [],
		"status": []
	}
}
```

Modem Management
----------------

```javascript
{
	"juci.modems": {
		"list": [],
		"list4g": []
	}
}
```

Network (TCP/UDP)
-----------------

```javascript
{
	"juci.network": {
		"load": [],
		"clients": [],
		"protocols": [],
		"nat_table": [],
		"services": [],
		"nameservers": []
	}
}
```

Status: 

```javascript
{
	"juci.network.status": {
		"arp": [],
		"ipv4routes": [],
		"ipv6neigh": [],
		"ipv6routes": []
	}
}
```

OpenWRT Network Interface Status

```javascript
{
	"network.interface": {
		"dump": []
	}
}
```

Realtime Network Traffic: 

```javascript
{
	"juci.rtgraphs": {
		"get": []
	}
}
```

Samba
-----

```javascript
{
	"juci.samba": {
		"folder_tree": [],
		"autocomplete": []
	}
}
```

Switch Configuration
--------------------

```javascript
{
	"juci.swconfig": {
		"status": []
	}
}
```

System Information
------------------

```javascript
{
	"juci.system": {
		"info": [],
		"defaultreset": [],
		"filesystems": [],
		"log": [],
		"reboot": []
	}
}
```

OpenWRT System Info

```javascript
{
	"system": {
		"info": [],
		"board": []
	}
}
```

Configuration

```javascript
{
	"juci.system.conf": {
		"clean": [],
		"backup": [],
		"features": [],
		"restore": []
	}
}
```

Processes

```javascript
{
	"juci.system.process": {
		"list": []
	}
}
```

Services

```javascript
{
	"juci.system.service": {
		"disable": [],
		"list": [],
		"stop": [],
		"start": [],
		"status": [],
		"enable": [],
		"reload": []
	}
}
```

Time

```javascript
{
	"juci.system.time": {
		"zonelist": [],
		"timediff": [],
		"set": [],
		"get": []
	}
}
```

Upgrade

```javascript
{
	"juci.system.upgrade": {
		"test": [],
		"check": [],
		"clean": [],
		"online": [],
		"start": []
	}
}
```

Users

```javascript
{
	"juci.system.user": {
		"setpassword": [],
		"listusers": []
	}
}
```

User Interface
--------------

```javascript
{
	"juci.ui": {
		"menu": []
	}
}
```

UPnP
----

```javascript
{
	"juci.upnpd": {
		"ports": []
	}
}
```

USB
---

```javascript
{
	"juci.usb": {
		"list": []
	}
}
```

Session And Access
------------------

```javascript
{
	"session": {
		"access": []
	}
}
```

File Uploads
------------

JUCI file uploader is currently very simple and supports writing blocks to
files that user is allowed to write to over RPC. The benefit of using RPC file
upload is that it works very well without any extra protocol support regardless
of where the client is running (it can even be behind a websocket proxy and
uploading will work anyway). The biggest disadvantage is that this simple
approach is currently a lot slower than uploading a file directly over HTTP. 

User permissions can be set as `juci file <filename> w`. If this permission is
granted for currently logged in user then file uploads for this user will be
allowed. 

```javascript
{
	"file": {
		"write" [ filename:STRING, seek:INT, length:INT, data64:STRING ]
	}
}
```

`write` - writes a file fragment to specified file. 

A base64 encoded binary fragment of length `length` is written at position
`seek` inside file specified by `filename`. If file does not exist then it is
created. If seek position is past the end of the file then file size is
extended automatically to fit that position and extra bytes are filled with 0s. 
