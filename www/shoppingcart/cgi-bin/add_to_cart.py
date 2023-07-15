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


debug("Starting shoppingcart2")
request_body_size = int(os.environ["CONTENT_LENGTH"])
request_body = sys.stdin.read(request_body_size)
debug(f"request_body: {request_body}")
json_data = json.loads(request_body)
debug(f"json_data: {json_data}")
paperclip = json_data["paperclip"]
monalisa = json_data["monalisa"]
spaceshuttle = json_data["spaceshuttle"]
cookie = Cookie.SimpleCookie()
cookie["paperclip"] = paperclip
cookie["paperclip"]["path"] = "/"
cookie["paperclip"]["max-age"] = 31536000
cookie["monalisa"] = monalisa
cookie["monalisa"]["path"] = "/"
cookie["monalisa"]["max-age"] = 31536000
cookie["spaceshuttle"] = spaceshuttle
cookie["spaceshuttle"]["path"] = "/"
cookie["spaceshuttle"]["max-age"] = 31536000
cookie_output = cookie.output()
debug(f"cookie_output: {cookie_output}")
body = json.dumps(
    {
        "paperclip": paperclip,
        "monalisa": monalisa,
        "spaceshuttle": spaceshuttle,
    }
)
debug(f"body: {body}")

print("Content-Type: application/json")
print(cookie_output)
print()
print(body)
