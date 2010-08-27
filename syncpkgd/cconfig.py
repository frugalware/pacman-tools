class config:
	server_url = "http://localhost:1873"
	server_user = "user"
	server_pass = "pass" # change this

	# wait X seconds if no package to build
	sleep = 300

	# if the load is higher than this value, don't start building a new package
	#throttle = 1.00

	# never try to build these packages, just fail
	#blacklist = ['foo', 'bar']

# vim: ft=python
