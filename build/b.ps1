$ErrorActionPreference = "Stop"

# $pythonUrl = "https://www.python.org/ftp/python/3.13.12/python-3.13.12-embed-amd64.zip"
$pythonUrl = "https://github.com/winpython/winpython/releases/download/17.2.20251222post1/WinPython64-3.15.0.0dot_post1.zip"
$pythonUrl = "https://github.com/winpython/winpython/releases/download/4.6.20220116/Winpython64-3.10.2.0dot.exe";
$pipurl = "https://bootstrap.pypa.io/get-pip.py"
$zipFile = "python-embed.exe"
$embedDir = "python-embed"
$tempInstall = "temp-python-install"

Write-Host "Downloading embeddable Python..."
Invoke-WebRequest -Uri $pythonUrl -OutFile $zipFile

Write-Host "Extracting 7Ζip archive..."
# Extract the zip file using 7-Zip
$sevenZipPath = ".\7zr.exe"
& $sevenZipPath x $zipFile -o$embedDir -y
# Delete the zip file after extraction
Remove-Item $zipFile


# Create site-packages folder
New-Item -ItemType Directory -Force -Path "$embedDir\Lib"
New-Item -ItemType Directory -Force -Path "$embedDir\Lib\site-packages"

<# download get-pip.py
Write-Host "Downloading get-pip.py..."
Invoke-WebRequest -Uri $pipurl -OutFile "get-pip.py"

# Install pip 
Write-Host "Setting up temporary environment for pip installation..."

# Install packages using normal Python
Write-Host "Installing packages in temporary environment..."
python -m venv $tempInstall
& "$tempInstall\Scripts\python.exe" -m pip install --upgrade pip
& "$tempInstall\Scripts\python.exe" -m pip install github-copilot-sdk asyncio pywin32

Write-Host "Copying installed packages to embeddable..."
Copy-Item -Recurse -Force "$tempInstall\Lib\site-packages\*" "$embedDir\Lib\site-packages\"

Write-Host "Cleaning up..."
Remove-Item -Recurse -Force $tempInstall
Remove-Item $zipFile

Write-Host "Embedded Python ready in folder: $embedDir"

#>