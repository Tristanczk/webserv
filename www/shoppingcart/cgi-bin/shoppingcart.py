import cgi
import inspect
import os
import random
import sys

# TODO remove lines in file if cookie expired
# TODO add expiration date to cookie
# TODO create database if it doesn't exist

RESET = "\033[0m"
RED = "\033[31m"
DEBUG = False


def debug(s):
    if DEBUG:
        print(RED + str(s) + RESET, file=sys.stderr)


COOKIE_SIZE = 6
MIN_COOKIE = 10 ** (COOKIE_SIZE - 1)
MAX_COOKIE = 10**COOKIE_SIZE - 1
PATH = os.path.join(
    os.path.dirname(
        os.path.abspath(inspect.getframeinfo(inspect.currentframe()).filename)
    ),
    "shoppingcart.txt",
)


def get_random_uid(uids):
    while True:
        uid = random.randint(MIN_COOKIE, MAX_COOKIE)
        if uid not in uids:
            return uid


def get_cookies():
    cookies = dict()
    for cookie in os.environ["HTTP_COOKIE"].split(";"):
        debug(f"cookie: {cookie}")
        if "=" in cookie:
            key, value = cookie.split("=")
            cookies[key] = value
    return cookies


def printHtml(computers, phones, printers, setcookies, database, cookieValue):
    print("Content-Type: text/html")
    if setcookies:
        print(f"Set-Cookie: UID={cookieValue}")
    print()

    print("<!DOCTYPE html>")
    print('<html lang="en">')
    print("<head><title>Cart</title></head>")
    print("<body>")

    print(
        f"<p>There are {computers} computers, {phones} phones and "
        f"{printers} printers in your cart.</p>"
    )
    print("<a href='/'>Go back to shop.</a>")

    print("</body>")
    print("</html>")


setcookies = False
cookies = get_cookies()
debug(f"cookies: {cookies}")
database = dict()
for line in open(PATH).read().splitlines():
    debug(line)
    uid, computers, phones, printers = line.split(",", 3)
    database[uid] = (int(computers), int(phones), int(printers))

if "UID" in cookies:
    uid = cookies["UID"]
else:
    uid = get_random_uid(database.keys())
    database[uid] = (0, 0, 0)
    setcookies = True

debug(f"UID: {uid}")

form = cgi.FieldStorage()
item = form.getvalue("item") or ""
cnt = form.getvalue("count") or "0"
try:
    cnt = int(cnt)
except ValueError:
    cnt = 0

debug(f"Item: {item}, count: {cnt}")

computers, phones, printers = database[uid]
if item == "computer":
    computers += cnt
elif item == "phone":
    phones += cnt
elif item == "printer":
    printers += cnt
database[uid] = (computers, phones, printers)

with open(PATH, "w") as f:
    for uid, (computers, phones, printers) in database.items():
        f.write(f"{uid},{computers},{phones},{printers}\n")

printHtml(computers, phones, printers, setcookies, database, uid)
