#! /bin/ksh

# function to print usage message to user

# function to list all processes in the system
# syntax: list_all_proc
#
list_all_proc()
{
cat - <<__END
dcxhstsv
dcxhstcp
foldSvr
TransRcv
HTServer
secuSrv3
SMSRcv
MoniPosp
shortTcp
shortClient
httpClient
httpServer
synTcp
synHttp
timerProc
PinEncrySrv
memSrv
lhttpC
lsynHttpC
XPETransRcv
TransferRcv
timeOutRcv
__END
}


# function to signal a process.
# syntax: signal_proc procName signo
#
signal_proc_bak()
{
    if [ $# -lt 2 ]; then
        return 0
    fi

    procList=`ps -ef | grep "$1"`
    if [ "x$procList" = "x" ]; then
        return 0
    fi

    IFS="\n"

    echo $procList | while read Line
    do
        tmpLine=`echo $Line | sed -e 's/grep /MARK_ /g'`
        if [ "$Line" != "$tmpLine" ]; then
            continue
        fi

        ttyName=`echo $Line | awk '{print $6}' | sed -e 's/[ \?]//g'`
        if [ x"$ttyName" != "x" ]; then
            continue
        fi

        pid=`echo $Line | awk '{print $2}'`
        kill -$signo  $pid  2>/dev/null
    done

    return 0
}

# function to signal a process.
# syntax: signal_proc procName signo
#
signal_proc()
{
    if [ $# -lt 2 ]; then
        return 0
    fi

    procList=`ps -ef | grep "$1"`
    if [ "x$procList" = "x" ]; then
        return 0
    fi

    IFS="\n"

    echo $procList | while read Line
    do
        tmpLine=`echo $Line | sed -e 's/grep /MARK_ /g'`

        if [ "$Line" != "$tmpLine" ]; then
            continue
        fi

        pid=`echo $Line | awk '{print $2}'`
	#echo "$1   $2  kill  $pid..."
        #kill   $pid  2>/dev/null
        kill  -$2 $pid  2>/dev/null
    done

    return 0
}



#cleanup
# function to remove all ipc objects
# syntax: cleanup_ipcobj
#
cleanup_ipcobj()
{
    # remove shared memory
    #
    echo "INFO : cleanup shared memories..."

    #shmList=`ipcs -mo | grep "$LOGNAME"`
    shmList=`ipcs -m| grep "$LOGNAME"`
    if [ "x$shmList" != "x" ]; then
        IFS="\n"
        echo $shmList | while read Line
        do
            tmpLine=`echo $Line | sed -e 's/[a-zA-Z:\n]//g'`
            if [ "x$tmpLine" = "x" ]; then
                continue
            fi
            id=`echo $Line | awk '{print $2}'`
            ipcrm -m  $id  2>/dev/null
        done
    fi

    # remove semaphore
    #
    echo "INFO : cleanup semaphores..."

    #semList=`ipcs -so | grep "$LOGNAME"`
    semList=`ipcs -s | grep "$LOGNAME"`
    if [ "x$semList" != "x" ]; then
        IFS="\n"
        echo $semList | while read Line
        do
            tmpLine=`echo $Line | sed -e 's/[a-zA-Z:\n]//g'`
            if [ "x$tmpLine" = "x" ]; then
                continue
            fi
            id=`echo $Line | awk '{print $2}'`
            ipcrm -s  $id  2>/dev/null
        done
    fi

    # remove message queue
    #
    echo "INFO : cleanup message queues..."

    #queList=`ipcs -qo | grep "$LOGNAME"`
    queList=`ipcs -q | grep "$LOGNAME"`
    if [ "x$queList" != "x" ]; then
        IFS="\n"
        echo $queList | while read Line
        do
            tmpLine=`echo $Line | sed -e 's/[a-zA-Z:]//g'`
            if [ "x$tmpLine" = "x" ]; then
                continue
            fi
            id=`echo $Line | awk '{print $2}'`
            ipcrm -q  $id  2>/dev/null
        done
    fi
}


# function to startup the whole system
# systax: sartup
#
onshutdown()
{
    echo "INFO : system is shutting down, please wait..."

    # first round we send a SIGTERM signal to each process
    #
    list_all_proc | while read proc
    do
        echo "INFO : bring down $proc..."
        signal_proc "$proc" 15
         #signal_proc "$proc" 10
    done

    sleep 2

    # second round we send a SIGKILL signal to each process
    # to ascertain all processes do be killed
    #
    list_all_proc | while read proc
    do
        signal_proc "$proc" 9
    done

    # cleanup all kernel resources
    #
    cleanup_ipcobj

    echo "INFO : system shut down completed."
}

############################################################################
#                       main function goes here
#############################################################################

# cleanup all kernel resources
#
onshutdown

#cd $EXECHOME/run/log
#rm -rf ./*

unlink /tmp/SECSRV2
unlink /tmp/TRANSRCV
unlink /tmp/POS_COM
unlink /tmp/TM04
unlink /tmp/HSTSRV
unlink /tmp/TM03
unlink /tmp/HTSERVER
unlink /tmp/foldSvr
unlink /tmp/PEX_COM
unlink /tmp/fold_response_pipe_file
unlink /tmp/H3C_COM
unlink /tmp/SMSRCV

rm -rf /tmp/*