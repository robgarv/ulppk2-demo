# AM_CFLAGS = -g -fPIC @demolibs_inc@ -I/usr/local/include/ulppk -DULPPK_DEBUG
AM_CFLAGS = -g -fPIC @demolibs_inc@ -I/usr/local/include/ulppk 
AM_LDFLAGS = @demolibs_libflags@ -ldl -lutil -lulppk -ldemolibs -pthread

dist_bin_SCRIPTS = create-demo-server-files.sh 

# noinst_PROGRAMS = pty pt1 test_echo

bin_PROGRAMS = demoserver demosocketclient demosocketserver
demoserver_SOURCES = demoserver.c
demosocketclient_SOURCES = demosocketclient.c
demosocketserver_SOURCES = demosocketserver.c

install-exec-hook:
	mkdir -p /var/ulppk2-demo/data
	mkdir -p /var/ulppk2-demo/log
	mkdir -p /var/ulppk2-demo/core
	mkdir -p /var/ulppk2-demo/memfiles
	find /var/ulppk2-demo -type d -exec chmod 777 {} \;
	cp demoserver.ini /usr/local/etc
	cp demosocketclient.ini /usr/local/etc
	cp demosocketserver.ini /usr/local/etc
	chmod 666 /usr/local/etc/demoserver.ini
	chmod 666 /usr/local/etc/demosocketclient.ini
	chmod 666 /usr/local/etc/demosocketserver.ini


