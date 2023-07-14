const querystring = require('querystring');

const params = querystring.parse(process.env.QUERY_STRING);
const name = params.name || 'World!';

console.log(`Content-Type: image/svg+xml

<svg width="100%" height="100%" style="background-color: #596886;" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1000 600">
    <defs>
        <linearGradient id="gradient" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" style="stop-color:rgb(255,180,20);stop-opacity:1" />
            <stop offset="100%" style="stop-color:rgb(255,20,180);stop-opacity:1" />
        </linearGradient>
        <filter id="3d">
            <feDropShadow dx="5" dy="5" stdDeviation="3" flood-color="#000"/>
        </filter>
    </defs>
    <g transform="rotate(-15 500 300)">
        <text x="50%" y="40%" alignment-baseline="middle" text-anchor="middle" font-family="Verdana" font-size="120" filter="url(#3d)">
            Hello ${name}
        </text>
    </g>
    <style>
        text {
            font-weight: bold;
            fill: url(#gradient);
            stroke: #fff;
            stroke-width: 2px;
        }
    </style>
</svg>`);
