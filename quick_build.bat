@echo off
setlocal
cd /d "C:\Users\Eddium Ally\Documents\CyNickal-Software-EFT-master"
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "CyNickal Software EFT\CyNickal Software EFT.vcxproj" /p:Configuration=Release /p:Platform=x64 "/p:SolutionDir=C:\Users\Eddium Ally\Documents\CyNickal-Software-EFT-master\\" /m /v:minimal
