#!/usr/bin/python3
import os
import sys

sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")

html_content = """
<html>
<head><title>Test CGI Python</title></head>
<body>
    <h1>Variables d'environnement recues :</h3>
    <ul>
"""
sys.stdout.write(html_content)

for key, value in os.environ.items():
    sys.stdout.write(f"<li><b>{key}:</b> {value}</li>\n")

sys.stdout.write("    </ul>")

if os.environ.get("REQUEST_METHOD") == "POST":
    sys.stdout.write("<h3>Contenu du Body (reçu sur stdin) :</h3>")

    try:
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        if content_length > 0:
            body = sys.stdin.read(content_length)
            sys.stdout.write(f"<pre>{body}</pre>")
    except ValueError:
        sys.stdout.write("<p>Erreur : CONTENT_LENGTH invalide.</p>")

sys.stdout.write("</body></html>")