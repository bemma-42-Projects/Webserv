<?php
header("Content-Type: text/plain");

echo "Hello World !\n";
echo "--- Variables transmises par Webserv ---\n";
echo "Ton IP (REMOTE_ADDR) : " . $_SERVER['REMOTE_ADDR'] . "\n";
echo "Query String brute : " . $_SERVER['QUERY_STRING'] . "\n";
echo "Méthode utilisée : " . $_SERVER['REQUEST_METHOD'] . "\n";

echo "\n--- Paramètres $_GET extraits par PHP ---\n";
foreach ($_GET as $key => $value) {
    echo "$key => $value\n";
}
?>