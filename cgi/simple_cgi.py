#!/usr/bin/env python3
import os
import html

print(f"""Content-type: text/html\r\n\r
<!DOCTYPE html>
<html>
<head>
    <title>Simple CGI Python Script</title>
</head>
<body>
    <h1>Welcome to my website!</h1>
    <p>This is the URL you requested: {html.escape(os.environ.get('REQUEST_URI', ''))}</p>
</body>
</html>""")
