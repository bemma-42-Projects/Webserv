*This project has been created as part of the 42 curriculum by rmetge, juduchar, and bemma.*

## Description
**Webserv** est un serveur HTTP/1.1 écrit en **C++98**. L'objectif est de recréer un logiciel capable de rivaliser avec les fonctionnalités de base de NGINX, en utilisant le multiplexage d'entrées/sorties via `epoll`.

Le serveur est conçu pour être résilient, totalement non-bloquant et capable de gérer plusieurs "serveurs virtuels" via un fichier de configuration unique. Il traite les requêtes `GET`, `POST` et `DELETE`, gère les erreurs HTTP avec des pages personnalisées et permet l'exécution de scripts via **CGI**.

---

## Fonctionnalités
- **Multiplexage :** Gestion de multiples clients sur un seul thread avec `epoll`.
- **Hébergement Virtuel :** Capacité d'écouter sur plusieurs paires IP:Port et de différencier les serveurs par le `server_name`.
- **CGI :** Exécution de scripts basés sur l'extension du fichier (ex: `.php`, `.py`).
- **Gestion des Fichiers :** Support de l'indexation de répertoire (`autoindex`), de l'upload de fichiers et du service de fichiers statiques.
- **Robustesse :** Le serveur est conçu pour rester opérationnel même en cas de requêtes malformées ou volumineuses.

---

## Instructions

### Compilation
Utilisez le `Makefile` fourni pour compiler le projet

### Installation
Le projet ne nécessite aucune dépendance externe, hormis un environnement Linux pour le support de epoll.

### Exécution
Lancez le serveur en passant le fichier de configuration en argument : ./webserv src/test.conf

## Configuration
Le fichier de configuration utilise une syntaxe inspirée de NGINX, structurée en "directives simples" (terminées par un ;) et en "blocs/contextes" (délimités par {}).

### Directives du bloc server
listen : Définit l'interface IP et le port (ex: listen 127.0.0.1:8080; ou listen 80;).

server_name : Permet de différencier plusieurs serveurs écoutant sur le même port.

error_page : Associe un code d'erreur à un chemin de fichier (ex: error_page 404 /404.html;).

client_max_body_size : Limite la taille du corps de la requête (413 si dépassé). Une valeur de 0 désactive la vérification.

### Directives du bloc location (Routes)
Le bloc location permet de définir des comportements spécifiques selon l'URL :

methods : Liste des méthodes autorisées (ex: methods GET POST;).

root : Définit le répertoire racine pour la route.

index : Fichier par défaut si la cible est un répertoire.

autoindex : Active ou désactive l'affichage du contenu d'un dossier (on/off).

return : Gère les redirections HTTP.

upload_store : Définit l'emplacement où les fichiers téléchargés sont stockés.

cgi_ext : Associe une extension de fichier à un exécutable CGI.

### Règles de parsing
Les commentaires commencent par #.

Les espaces, tabulations et retours à la ligne sont autorisés entre les directives.

Les blocs location ne peuvent pas être imbriqués.

Chaque serveur doit avoir au moins une location / définie.

## Détails Techniques (Système)
Le serveur utilise la structure stat pour vérifier l'existence et les permissions des fichiers demandés.

Protection : st_mode permet de vérifier si un fichier est un répertoire ou un exécutable.

Taille : st_size est utilisé pour remplir le header Content-Length des réponses.

Le CGI reçoit ses informations via des variables d'environnement et lit le corps de la requête (POST) sur son stdin. Il attend un EOF pour terminer son exécution, tout comme le serveur attend l'EOF du CGI pour fermer la réponse.

## Ressources

### Documentation
RFC 7230 : Protocole HTTP/1.1.

Beej's Guide to Network Programming : Programmation de sockets.

Man pages : epoll(7), fcntl(2), stat(2).
