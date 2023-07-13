const redSlider = document.getElementById("red");
const greenSlider = document.getElementById("green");
const blueSlider = document.getElementById("blue");
const body = document.getElementById("body");
const saveColorButton = document.getElementById("saveColor");

redSlider.oninput = function () {
	updateBackgroundColor();
}

greenSlider.oninput = function () {
	updateBackgroundColor();
}

blueSlider.oninput = function () {
	updateBackgroundColor();
}

saveColorButton.onclick = function () {
	saveColor();
}

const updateBackgroundColor = () => {
	body.style.backgroundColor = "rgb(" + redSlider.value + ", " + greenSlider.value + ", " + blueSlider.value + ")";
}

const saveColor = () => {
	fetch('/cgi-bin/save_color.py', {
		method: 'POST',
		headers: {
			'Content-Type': 'application/json'
		},
		body: JSON.stringify({
			red: redSlider.value,
			green: greenSlider.value,
			blue: blueSlider.value
		})
	}).then(response => response.json()).then(data => console.log(data));
}

function getCookie(name) {
	var nameEQ = name + "=";
	var ca = document.cookie.split(';');
	for (var i = 0; i < ca.length; i++) {
		var c = ca[i];
		while (c.charAt(0) == ' ') c = c.substring(1, c.length);
		if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length, c.length);
	}
	return null;
}

window.onload = function () {
	let color = getCookie('color');
	if (!color || color.length != 7)
		color = "#808080";
	if (color) {
		body.style.backgroundColor = color;
		const red = parseInt(color.substring(1, 3), 16);
		const green = parseInt(color.substring(3, 5), 16);
		const blue = parseInt(color.substring(5, 7), 16);

		console.log(red, green, blue);

		document.getElementById('red').value = red;
		document.getElementById('red').classList.remove('invisible');
		document.getElementById('red').classList.add('visible');
		document.getElementById('green').value = green;
		document.getElementById('green').classList.remove('invisible');
		document.getElementById('green').classList.add('visible');
		document.getElementById('blue').value = blue;
		document.getElementById('blue').classList.remove('invisible');
		document.getElementById('blue').classList.add('visible');
	}
}