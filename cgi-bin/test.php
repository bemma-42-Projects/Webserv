<?php
# la ligne Content-Type est indispensable
# sans cette ligne, le navigateur téléchargera le fichier au lieu d'afficher son contenu
# il faut indiquer le "type" du texte
# avec Content-Type
# text/plain : texte brut
# text/html : contenu HTML
# pour un premier test, texte brut, puis contenu HTML plus tard
# ATTENTION : il faut séparer les headers HTTP du body HTTP
# par \r\n\r
# c'est le script PHP qui doit fournir ce header
# car c'est le script PHP qui doit savoir quoi générer
# du texte, du JSON, du HTML, ...
echo "Content-Type: text/plain";
echo "\r\n\r\n";
echo "Hello World";
?>