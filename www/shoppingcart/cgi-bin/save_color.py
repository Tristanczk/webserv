import json
import http.cookies as Cookie
import os
import sys

RESET = "\033[0m"
RED = "\033[31m"
DEBUG = False


def debug(s):
    if DEBUG:
        print(RED + str(s) + RESET, file=sys.stderr)


debug("Starting save color")
request_body_size = int(os.environ["CONTENT_LENGTH"])
request_body = sys.stdin.read(request_body_size)
json_data = json.loads(request_body)
hex_color = "#{:02x}{:02x}{:02x}".format(
    int(json_data["red"]), int(json_data["green"]), int(json_data["blue"])
)
debug(f"Saving color {hex_color}")

cookie = Cookie.SimpleCookie()
cookie["color"] = hex_color
cookie["color"]["path"] = "/"
cookie["color"]["max-age"] = 31536000
cookie_output = cookie.output()
debug(cookie_output)
body = json.dumps({"color": hex_color})
debug(body)

print("Content-type: application/json")
print(cookie_output)
print()
print(body)
