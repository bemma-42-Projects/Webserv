#!/usr/bin/php-cgi
<?php
// Les en-têtes CGI obligatoires (très important avec les \r\n\r\n)
echo "Content-Type: text/html\r\n\r\n";

echo "<!DOCTYPE html>\n";
echo "<html>\n<head><title>Test Asynchrone</title></head>\n<body>\n";
echo "<h1>Test CGI Asynchrone (Non-Bloquant)</h1>\n";

echo "<p>Début de l'exécution : <strong>" . date('H:i:s') . "</strong></p>\n";

// On force le script PHP à s'endormir pendant 5 secondes
sleep(5);

echo "<p>Fin de l'exécution : <strong>" . date('H:i:s') . "</strong></p>\n";
echo "<p>Si tu as pu charger une autre page pendant ces 5 secondes, ton epoll est 100% parfait !</p>\n";
echo "</body>\n</html>\n";
?>