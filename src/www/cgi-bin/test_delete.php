<?php
// On essaie de supprimer un fichier temporaire par exemple
if ($_SERVER['REQUEST_METHOD'] === 'DELETE') {
    if (unlink("fichier_test.txt")) {
        echo "Fichier supprime par le script PHP !";
    } else {
        echo "Le script n'a pas pu supprimer le fichier.";
    }
}
?>