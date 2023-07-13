import cgi
import inspect
import os
import random
import sys

# TODO remove lines in file if cookie expired
# TODO add expiration date to cookie

RESET = "\033[0m"
RED = "\033[31m"
DEBUG = True


def debug(s):
    if DEBUG:
        print(RED + str(s) + RESET, file=sys.stderr)

debug("Starting shopping cart")
request_body_size = int(os.environ["CONTENT_LENGTH"])
request_body = sys.stdin.read(request_body_size)
debug(f"Request body: {request_body}")


# COOKIE_SIZE = 6
# MIN_COOKIE = 10 ** (COOKIE_SIZE - 1)
# MAX_COOKIE = 10**COOKIE_SIZE - 1
# PATH = path = os.path.join(
#     os.path.dirname(
#         os.path.abspath(inspect.getframeinfo(inspect.currentframe()).filename)
#     ),
#     "shoppingcart.txt",
# )


# def get_random_uid(uids):
#     while True:
#         uid = random.randint(MIN_COOKIE, MAX_COOKIE)
#         if uid not in uids:
#             return uid


# def get_cookies():
#     cookies = dict()
#     for cookie in os.environ["HTTP_COOKIE"].split(","):
#         if "=" in cookie:
#             key, value = cookie.split("=")
#             cookies[key] = value
#     return cookies


# cookies = get_cookies()

# database = dict()
# for line in open(path).read().splitlines():
#     uid, computers, phones, printers = line.split(",", 1)
#     database[uid] = (computers, phones, printers)


# form = cgi.FieldStorage()
# item = form.getvalue("item") or ""
# cnt = form.getvalue("count") or "0"
# try:
#     cnt = int(cnt)
# except ValueError:
#     cnt = 0

# print(item, cnt, file=sys.stderr)

# computers = phones = printers = 0
# if item == "computer":
#     computers += cnt
# elif item == "phone":
#     phones += cnt
# elif item == "printer":
#     printers += cnt

# print("Content-Type: text/html")
# # if set_cookie:
# #     print(f"Set-Cookie: UID={get_random_uid(database.keys())}")
# print()

# print("<!DOCTYPE html>")
# print('<html lang="en">')
# print("<head><title>Cart</title></head>")
# print("<body>")

# print(
#     f"<p>There are {computers} computers, {phones} phones and "
#     f"{printers} printers in your cart.</p>"
# )
# print("<a href='/'>Go back to shop.</a>")

# print("</body>")
# print("</html>")
