all:
	gcc -Wall fcgi.c frozen.c credis.c -o fcgi.fcgi -lfcgi -lpthread
	sudo rm /home/vagrant/fcgiws/echo.fcgi
	sudo cp fcgi.fcgi /home/vagrant/fcgiws/echo.fcgi
	sudo service lighttpd restart