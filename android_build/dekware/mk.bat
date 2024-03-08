
set ANDROID_SDK=C:\general\android\android-sdk

set path=%path%;%ANDROID_SDK%\build-tools\27.0.3

. aapt package -f -m -J <input> -M AndroidManifest.xml -S <output> -I 
android.jar
2. javac *.java and output to obj
3. dx --dex --output=classes.dex obj
4. aapt package -f -m -F app.unaligned.apk -M AndroidManifest.xml -S 
<output> -I android.jar
5. apksigner sign --ks debug.keystore --ks-pass "blabla"
6. zipalign -f 4 app.unaligned.apk app.apk
