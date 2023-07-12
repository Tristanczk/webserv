import html
import os

print("Content-Type: text/html")
print()

print("<!DOCTYPE html>")
print('<html lang="en">')
print("<head><title>Environment Variables</title></head>")
print("<body>")
open("shoppingcart.txt", "r").read()
for key, value in os.environ.items():
    print(f"<tr><td>{html.escape(key)}</td><td>{html.escape(value)}</td></tr>")

print("</table>")
print("</body>")
print("</html>")
