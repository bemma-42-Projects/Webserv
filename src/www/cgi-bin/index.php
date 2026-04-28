<?php
// PHP-CGI s'occupe d'envoyer les headers si on les écrit ainsi
header("Content-Type: text/html");
header("X-Powered-By: MyWebserv");

echo "<!DOCTYPE html>
<html>
<head>
    <title>Webserv CGI Test</title>
    <style>
        body { font-family: sans-serif; background: #2c3e50; color: white; text-align: center; padding-top: 50px; }
        .container { background: #34495e; display: inline-block; padding: 20px; border-radius: 10px; border: 2px solid #27ae60; }
        h1 { color: #27ae60; }
    </style>
</head>
<body>
    <div class='container'>
        <h1>🚀 CGI Success!</h1>
        <p>Le serveur a executé l'interpreteur PHP avec succès.</p>
        <p><strong>Heure du serveur :</strong> " . date('H:i:s') . "</p>
        <p><strong>Methode :</strong> " . $_SERVER['REQUEST_METHOD'] . "</p>
    </div>
</body>
</html>";
?>