#!/usr/bin/php-cgi

<?php
header("Content-Type: text/html");
header("Status: 404 Not Found");

echo "<!DOCTYPE html>";
echo '<html lang="en">';
echo "<head><title>Environment Variables</title></head>";
echo "<body>";
echo "<table border='1'>";
echo "<tr><th>Variable</th><th>Value</th></tr>";

foreach ($_SERVER as $key => $value) {
    echo "<tr><td>" . htmlspecialchars($key) . "</td><td>" . htmlspecialchars($value) . "</td></tr>";
}

echo "</table>";
echo "</body>";
echo "</html>";

?>
