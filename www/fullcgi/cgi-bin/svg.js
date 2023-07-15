const params = require('querystring').parse(process.env.QUERY_STRING);
const name = params.name || 'World';
const message = `Hello ${name}! 💞`;
const scale = 9 / message.length;

console.log(`Content-Type: image/svg+xml

<svg width="100%" height="100%" style="background-color: #596886;" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1000 ${600}">
    <defs>
        <style type="text/css">
			@import url('https://fonts.googleapis.com/css2?family=Bungee+Shade&amp;display=swap');
        </style>
        <linearGradient id="gradient" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" style="stop-color:rgb(255,180,20);stop-opacity:1" />
            <stop offset="100%" style="stop-color:rgb(255,20,180);stop-opacity:1" />
        </linearGradient>
        <filter id="3d">
            <feDropShadow dx="5" dy="5" stdDeviation="3" flood-color="#000"/>
        </filter>
    </defs>`);

for (let i = 0; i < 25; i++) {
	console.log(`
    <g transform="translate(${(1 - scale) * 500}, ${-1000 + i * 100}) scale(${scale}) rotate(-15 500 300)">
        <text x="50%" y="50%" alignment-baseline="middle" text-anchor="middle" font-family="'Bungee Shade', cursive" font-size="120" filter="url(#3d)">
            ${message}
        </text>
    </g>
    `);
}

console.log(`<style>
        text {
            fill: url(#gradient);
            stroke: #fff;
            stroke-width: 2px;
        }
    </style>
</svg>`);
