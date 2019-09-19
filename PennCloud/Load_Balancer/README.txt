This folder is for code related to the load balancer

How to run the load balancer:
	./loadBalancer [-p <port>] <loadBalancerConfigFileName>
	
Example:
	./loadBalancer -p 8000 loadBalancer.config
	
NOTE: Load Balancer config file format:

0.0.0.0,8888,0.0.0.0:50070
0.0.0.0,8889,0.0.0.0:50071

There is one line for each FE server
The first address is the address & port the browser connects on
The second address is the address & port the FE server will listen for heartbeat messages

The load balancer listens on port 8000 (or the port specified from the command line)

When the user tries to navigate to a page on the load balancer, it will be redirected to one of the FE servers in a round robin fashion