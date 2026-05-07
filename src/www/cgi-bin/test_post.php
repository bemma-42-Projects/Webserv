<?php
// On récupère le body brut envoyé par Webserv via STDIN (le Tuyau 1)
$body = file_get_contents('php://input');

echo "<html><body>";
echo "<h1>Test POST CGI</h1>";
echo "<p>J'ai bien recu : " . htmlspecialchars($body) . "</p>";

// Si le type est x-www-form-urlencoded, PHP remplit aussi $_POST automatiquement
if (!empty($_POST)) {
    echo "<h2>Variables POST :</h2>";
    echo "<pre>";
    print_r($_POST);
    echo "</pre>";
}
echo "</body></html>";
?>