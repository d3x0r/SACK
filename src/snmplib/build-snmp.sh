#!/bin/sh

case `uname` in
 IRIX) 
       ABI=32; export ABI
       (smake wash mf all install 2>&1) | tee log.`sys`.$ABI
       ABI=n32; export ABI
       (smake wash mf all install 2>&1) | tee log.`sys`.$ABI
       ;;
  *) 
     (smake wash mf all install 2>&1) | tee log.`sys` 
     ;;
esac

exit 0
