var cart = { 'Computer': 0, 'Phone': 0, 'Printer': 0 };

function addToCart(item) {
	cart[item] += 1;
}

function displayCart() {
	fetch('/cgi-bin/shoppingcart2.py', {
		method: 'POST',
		headers: {
			'Content-Type': 'application/json'
		},
		body: JSON.stringify({
			computer: cart['Computer'],
			phone: cart['Phone'],
			printer: cart['Printer']
		})
	}).then(response => response.json()).then(data => console.log(data));
}

// function removeItem(index) {
// 	cart.splice(index, 1);
// 	displayCart();
// }

// function calculateCartTotal() {
// 	// Add your own logic to calculate the total price based on cart items
// 	// For this example, we assume each item costs $100
// 	return cart.length * 100;
// }

function getCookie(name) {
	var nameEQ = name + "=";
	var ca = document.cookie.split(';');
	console.log(ca);
	for (var i = 0; i < ca.length; i++) {
		var c = ca[i];
		while (c.charAt(0) == ' ') c = c.substring(1, c.length);
		if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length, c.length);
	}
	return null;
}

window.onload = function () {
	let computer = getCookie('computer');
	let phone = getCookie('phone');
	let printer = getCookie('printer');
	console.log(computer);
	console.log(phone);
	console.log(printer);
	if (!computer) {
		computer = 0;
	}
	if (!phone) {
		phone = 0;
	}
	if (!printer) {
		printer = 0;
	}
	cart['Computer'] = parseInt(computer);
	cart['Phone'] = parseInt(phone);
	cart['Printer'] = parseInt(printer);
}