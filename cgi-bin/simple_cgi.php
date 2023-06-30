#!/usr/bin/php-cgi
<?php

$url = isset($_SERVER['REQUEST_URI']) ? $_SERVER['REQUEST_URI'] : 'not found, server operator probably goofed';

header('Content-Type: text/html');
?>
<!DOCTYPE html>
<html>
<head>
    <title>Simple CGI Python Script</title>
</head>
<body>
    <h1>Welcome to my website!</h1>
    <p>This is the URL you requested: <?= htmlentities($url) ?></p>
</body>
</html>
