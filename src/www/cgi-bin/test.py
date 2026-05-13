#!/usr/bin/python3
import os
import sys

# 1. Les Headers (Indispensables pour le navigateur)
# On utilise \r\n pour être parfaitement conforme au protocole HTTP
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n") # La ligne vide qui sépare les headers du body

# 2. Le corps (Body) en HTML
html_content = """
<html>
<head><title>Test CGI Python</title></head>
<body>
    <h1>Bravo ! Ton CGI Python fonctionne.</h1>
    <h3>Variables d'environnement reçues :</h3>
    <ul>
"""
sys.stdout.write(html_content)

# Affichage de l'environnement (envp transmis par ton C++)
for key, value in os.environ.items():
    sys.stdout.write(f"<li><b>{key}:</b> {value}</li>\n")

sys.stdout.write("    </ul>")

# 3. Gestion du POST (Lecture de stdin)
if os.environ.get("REQUEST_METHOD") == "POST":
    sys.stdout.write("<h3>Contenu du Body (reçu sur stdin) :</h3>")
    # On lit le nombre d'octets spécifié par CONTENT_LENGTH
    try:
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        if content_length > 0:
            body = sys.stdin.read(content_length)
            sys.stdout.write(f"<pre>{body}</pre>")
    except ValueError:
        sys.stdout.write("<p>Erreur : CONTENT_LENGTH invalide.</p>")

sys.stdout.write("</body></html>")