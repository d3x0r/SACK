cd typecore
git pull
call mk.bat
git commit -m "Sync sack updates" .
git push
cd ..


cd netcore
git pull
call mk.bat
git commit -m "Sync sack updates" .
git push
cd ..


cd filesys
git pull
call mk.bat
git commit -m "Sync sack updates" .
git push
cd ..



cd fullcore
git pull
call mk.bat
git commit -m "Sync sack updates" .
git push
cd ..

