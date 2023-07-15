function changeBgColor() {
	console.log("lol");
	const colors = ["#f0f0f0", "#d0e1f9", "#f7d794", "#fcf0c8", "#dbf6e9"];
	document.body.style.backgroundColor = colors[Math.floor(Math.random() * colors.length)];
}
