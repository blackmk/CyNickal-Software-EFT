Set-Location "C:\Users\Eddium Ally\Documents\CyNickal-Software-EFT-master"
$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
$project = "CyNickal Software EFT\CyNickal Software EFT.vcxproj"

$env:SolutionDir = "C:\Users\Eddium Ally\Documents\CyNickal-Software-EFT-master\"

& $msbuild $project /p:Configuration=Release /p:Platform=x64 /t:Build
