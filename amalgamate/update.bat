cd typecore
call mk.bat
git commit -m "Sync sack updates" .
git push
cd ..


cd netcore
call mk.bat
git commit -m "Sync sack updates" .
git push
cd ..


cd filesys
call mk.bat
git commit -m "Sync sack updates" .
git push
cd ..



cd fullcore
call mk.bat
git commit -m "Sync sack updates" .
git push
cd ..

