This folder is for code related to the backend server

#---------------------------------------------------------------
How to run the BE server:
./sample -a <address:port>

Example: ./sample -a 0.0.0.0:50051

#---------------------------------------------------------------
How to run the Master server:
./master <beServerConfigFileName>

Example: ./master beServers.config

NOTE: The BE server config file is in the following format:
<serverGroupNumber>,address:port,heartbeatAddress:port

For our purposes, the address:port and heartbeatAddress:port are the same

Example:
0,0.0.0.0:50051,0.0.0.0:50051
0,0.0.0.0:50052,0.0.0.0:50052
1,0.0.0.0:50053,0.0.0.0:50053
1,0.0.0.0:50054,0.0.0.0:50054

This indicates 2 servers in group 1 and 2 servers in group 2
One server in each group is the primary, all others are replicas

#---------------------------------------------------------------
To write from Thunderbird to bigtable, just run 

```
./SMTP_external_to_bigtables -v server1 server2

```
or 

```
./SMTP_external_to_bigtables -v server1/ server2

```
#---------------------------------------------------------------
To export from bigtable to Thunderbird, just run (only support one replica)
```
./SMTP_external_to_bigtables -v server1 

```
or 

```
./SMTP_external_to_bigtables -v server1/

```