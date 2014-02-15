#!/bin/bash

inlist=
last=

get_target()
{
	n=$#-1
        eval last=\$$n;
}

start()
{
	n=$#
	tmp=xxzzjtw
        echo $1>$tmp
	for ((i=2;i<n;i++));do eval word=\${$i}; echo $word>>$tmp; done
	eval last=\$$i;
        inlist=$(sort -u $tmp)
}

write_list()
{
	n=$#
	tmp=xxzzjtw
        echo $1>$tmp
	for ((i=2;i<=n;i++));do eval word=\${$i}; echo $word>>$tmp; done
}

read_libs()
{
	# write old list into file...
        #echo START WITH:$inlist
        
        write_list $inlist
        # add new (total) list to file
	ldd $inlist | awk '/=>/ {print $3}'| awk '$0 !~ /\/usr\/lib/ {print $0;}' | awk '$0 !~ /\/lib\// {print $0};' | awk '$0 !~ /\/lib64\// {print $0};'  | awk '$0 !~ /\/usr\/lib64\// {print $0};'| awk '$0 !~ /\(/ {print $0;}' >>$tmp
        # re-evaluate list
        inlist=$(sort -u $tmp)
        
        #echo END WITH:$inlist
        #echo last=$last
}


get_target $*
start $*
#oldlist=$inlist
#until(  );do eval oldlist=$inlinst; read_libs; done;
read_libs;
cp $inlist $last

rm $tmp

