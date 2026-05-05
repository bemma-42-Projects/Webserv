<?php
// On affiche un message clair pour le debug
echo "--- DEBUG CGI ---\n";
echo "Method: " . $_SERVER['REQUEST_METHOD'] . "\n";
echo "Content-Length: " . $_SERVER['CONTENT_LENGTH'] . "\n";

// On lit les données reçues
$raw_body = file_get_contents('php://input');
echo "Body recu: [" . $raw_body . "]\n";
echo "--- END DEBUG ---";
?>