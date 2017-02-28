#!/bin/bash
pash=`pwd`
HOME=`dirname $pash`

#delete lib
cd $HOME/prepose/src/temp/lib
rm -rf *.a


# -------- first compile api static lib --------
#common
cd $HOME/prepose/src/api/common
make

#dcsutil
cd $HOME/prepose/src/api/dcsutil
bash make_shell

#iso8583
cd $HOME/prepose/src/api/iso8583
make

#dcs
cd $HOME/prepose/src/api/dcs
bash make_shell

#fold
cd $HOME/prepose/src/api/fold
bash make_fold_shell

#timer
cd $HOME/prepose/src/api/timer
make

#security
cd $HOME/prepose/src/api/security
make

#memcache
cd $HOME/prepose/src/api/memcache
make

#timeout_control
cd $HOME/prepose/src/api/timeout_control
make

# --------  compile server  --------

#foldSvr
cd $HOME/prepose/src/srv/foldSvr
make

#monitor
cd $HOME/prepose/src/srv/monitor
make

#tcp_long
cd $HOME/prepose/src/srv/tcp/tcp_long
make

#tcp_server_short
cd $HOME/prepose/src/srv/tcp/tcp_server_short
make

#short-tcp-client
cd $HOME/prepose/src/srv/tcp/tcp_client_short
make

#tcp_client_short_syn
cd $HOME/prepose/src/srv/tcp/tcp_client_short_syn
make

#http_client_socket
cd $HOME/prepose/src/srv/http/http_client_socket
make

#http_server_httpd
cd $HOME/prepose/src/srv/http/http_server_httpd
make

#http_server_socket
cd $HOME/prepose/src/srv/http/http_server_socket
make

#http_client_socket_syn
cd $HOME/prepose/src/srv/http/http_client_socket_syn
make

#http_client_curl
cd $HOME/prepose/src/srv/http/http_client_curl
make

#http_client_curl_syn
cd $HOME/prepose/src/srv/http/http_client_curl_syn
make

#security
cd $HOME/prepose/src/srv/security
make

#memcache
cd $HOME/prepose/src/srv/memcache
make

#timer
cd $HOME/prepose/src/srv/timer
make

#timeout_control
cd $HOME/prepose/src/srv/timeout_control
make

#src
cd $HOME/prepose/src/srv/termSys
make


#dcxmonisv
#cd $HOME/prepose/run/bin
#./dcxmonisv
cd $HOME/prepose/src/temp/
cp -Rf ./bin/*  $HOME/prepose/run/bin/
cp -Rf ./bin/*  /var/xpep/run/bin/

 
