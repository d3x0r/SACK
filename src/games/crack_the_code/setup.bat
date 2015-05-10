@echo off
mysql -u user --password=password < ..\..\..\setup.sql

echo PLEASE CREATE/CONFIGURE 'MySQL' system DSN
echo  server is either computer's address, but must be the same on both systems
echo  username is 'user'
echo  password is 'password'
echo  select the database in the drop list 'eagle_mtn'

if NOT EXIST c:\windows\syswow64 goto IS32BIT

c:\windows\syswow64\odbcad32

goto done

:IS32BIT
c:\windows\system32\odbcad32

:done
