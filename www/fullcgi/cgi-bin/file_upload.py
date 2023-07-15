import cgi
import os
import sys

form = cgi.FieldStorage()
filename = "/tmp/" + os.path.basename(form["file"].filename)
if os.path.exists(filename):
    print("Content-Type: text/html")
    print("Status: 409 Conflict")
    print()
    sys.exit(0)

open(filename, "wb").write(form["file"].file.read())
print("Content-Type: text/html")
print()
print(
    f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <title>File Upload Result</title>
</head>
<body>
   <p>The file {filename} was uploaded successfully.</p>
</body>
</html>
"""
)
