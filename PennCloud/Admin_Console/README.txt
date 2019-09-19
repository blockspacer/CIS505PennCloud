This folder is for code related to the Admin Console

Admin Console:

Done:
1. Track server status via heartbeat messages
2. Run a server on port 7000. Handle HTTP GET request for http://localhost:7000/admin.html

TODO:
1. Add ability to shut down a server from the Admin Console page
2. Display the contents of the BigTable
3. Make the pages look better

To run the Admin Console:
	./adminConsole <feServersConfigFileName> <beServersConfigFileName>
	
Example:
	./adminConsole feServers.config beServers.config
	

	NOTE: The BE server config file is in the following format:
<serverGroupNumber>,address:port,heartbeatAddress:port

For the FE server file, the address:port is for the port the browser connects on and heartbeatAddress:port is the port for the heartbeat messages
For the BE server file, the address:port and heartbeatAddress:port are the same

Example:
0,0.0.0.0:50051,0.0.0.0:50051
0,0.0.0.0:50052,0.0.0.0:50052
1,0.0.0.0:50053,0.0.0.0:50053
1,0.0.0.0:50054,0.0.0.0:50054

This indicates 2 servers in group 1 and 2 servers in group 2
One server in each group is the primary, all others are replicas