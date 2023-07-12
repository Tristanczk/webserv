import cgi


def get_answer(n1, op, n2):
    if n1 == "" or op == "" or n2 == "":
        raise ValueError("Please enter all required fields.")
    try:
        n1 = int(n1)
        n2 = int(n2)
    except ValueError:
        raise ValueError("Please enter valid integers.")
    if op == "add":
        return n1 + n2
    elif op == "sub":
        return n1 - n2
    elif op == "mul":
        return n1 * n2
    elif op == "div":
        # We don't protect against division by zero for crash testing
        return n1 / n2
    else:
        raise ValueError("Invalid operator.")


def fill(index, name, value):
    return index.replace(
        f'name="{name}" value=""', f'name="{name}" value="{value}"'
    )


OPERATIONS = {"add": "+", "sub": "-", "mul": "*", "div": "/"}

form = cgi.FieldStorage()
n1 = form.getvalue("n1") or ""
op = form.getvalue("op") or ""
n2 = form.getvalue("n2") or ""
try:
    answer = get_answer(n1, op, n2)
    title = "Success"
    message = f"{n1} {OPERATIONS[op]} {n2} = {answer}"
except ValueError as e:
    title = "Failure"
    message = str(e)

print("Content-Type: text/html")
print()
print(
    f"""<!DOCTYPE html>
<html lang="en">

<head>
\t<title>{title}</title>
</head>

<body>
\t<p>{message}</p>
\t<a href="/">Go back to calculator</a>
</body>

</html>"""
)
