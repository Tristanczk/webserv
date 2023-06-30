#!/usr/bin/env node

const htmlEscape = require('escape-html');

// TODO use environment instead?
// TODO 
const requestUri = process.argv[2] || '';

console.log(`Content-type: text/html\r\n\r\n
<!DOCTYPE html>
<html>
<head>
    <title>Simple CGI Node.js Script</title>
</head>
<body>
    <h1>Welcome to my website!</h1>
    <p>This is the URL you requested: ${htmlEscape(requestUri)}</p>
</body>
</html>`);
