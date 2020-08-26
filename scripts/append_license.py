from pathlib import Path

def addFiles(paths, extensions):
    files = []
    for path in paths:
        for ext in extensions:
            for file in Path(path).rglob(f"*{ext}"):
                files.append(file.as_posix())
    return files

def appendLicense(filename, license):
    file = open(filename, 'r')
    contents = file.read()
    file.close() 
    if 'Copyright' in contents:
        return
    contents = license + contents
    file = open(filename, 'w')
    file.write(contents)
    file.close()
    
def main():
    files = addFiles(['src', 'porting', 'test', 'NodeGL/src', 'NodeGL/porting', 'NodeGL/test'], ['.c','.cpp','.h'])
    licenseFile = open('doc/LICENSE.txt', 'r')
    license = licenseFile.read()
    licenseFile.close()
    for file in files:
        appendLicense(file, license)
    print(len(files), files)
    
main()
