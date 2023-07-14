import json
import http.cookies as Cookie
import os
import sys

RESET = "\033[0m"
RED = "\033[31m"
DEBUG = True


def debug(s):
    if DEBUG:
        print(RED + str(s) + RESET, file=sys.stderr)

debug("Starting shoppingcart2")
request_body_size = int(os.environ["CONTENT_LENGTH"])
request_body = sys.stdin.read(request_body_size)
debug(f"request_body: {request_body}")
json_data = json.loads(request_body)
debug(f"json_data: {json_data}")
computer = json_data["computer"]
phone = json_data["phone"]
printer = json_data["printer"]
cookie = Cookie.SimpleCookie()
cookie["computer"] = computer
cookie["computer"]["path"] = "/"
cookie["computer"]["max-age"] = 31536000
cookie["phone"] = phone
cookie["phone"]["path"] = "/"
cookie["phone"]["max-age"] = 31536000
cookie["printer"] = printer
cookie["printer"]["path"] = "/"
cookie["printer"]["max-age"] = 31536000
cookie_output = cookie.output()
debug(f"cookie_output: {cookie_output}")
body = json.dumps({"computer": computer, "phone": phone, "printer": printer})
debug(f"body: {body}")

print("Content-Type: application/json")
print(cookie_output)
print()
print(body)