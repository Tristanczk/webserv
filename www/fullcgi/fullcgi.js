const querystring = require('querystring');

let html = `Content-Type: text/html

<!DOCTYPE html>
<html lang="en">
<head><title>Environment Variables</title></head>
<body>
<table border='1'>
<tr><th>Variable</th><th>Value</th></tr>
`;

for (const key in process.env) {
	const escapedKey = key;
	const escapedValue = process.env[key];

	html += `<tr><td>${escapedKey}</td><td>${escapedValue}</td></tr>`;
}

html += `
    </table>
    </body>
    </html>
`;

console.log(html);
