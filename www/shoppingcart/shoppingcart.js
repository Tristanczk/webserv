var cart = {};
var prices = { paperclip: 0.01, monalisa: 860000000, spaceshuttle: 1700000000 };
var toDisplay = false;

function sendCookie() {
	fetch('/cgi-bin/add_to_cart.py', {
		method: 'POST',
		headers: {
			'Content-Type': 'application/json'
		},
		body: JSON.stringify({
			paperclip: cart['paperclip'],
			monalisa: cart['monalisa'],
			spaceshuttle: cart['spaceshuttle']
		})
	}).then(response => response.json()).then(data => console.log(data));
}

function addToCart(item) {
	cart[item] += 1;
	sendCookie();
	if (toDisplay)
		displayCart();
}

function displayCart() {
	toDisplay = true;
	let cartDiv = document.getElementById('cart');
	cartDiv.innerHTML = ''; // Clear the cart display

	let totalItems = 0;
	let title = document.createElement('h2');
	title.textContent = "Cart";
	cartDiv.appendChild(title);

	for (let item in cart) {
		if (cart.hasOwnProperty(item)) {
			let quantity = cart[item];
			totalItems += quantity;
			console.log("item: " + item);
			// Create a new paragraph for each item
			let p = document.createElement('p');
			if (item == 'paperclip')
				p.textContent = "Paperclip: " + quantity;
			else if (item == 'monalisa')
				p.textContent = "Mona Lisa: " + quantity;
			else if (item == 'spaceshuttle')
				p.textContent = "Spaceshuttle: " + quantity;
			// Add the paragraph to the cart div
			console.log("p text: " + p.textContent);

			let removeOneButton = document.createElement('button');
			removeOneButton.textContent = 'Remove One';
			removeOneButton.className = 'btn';
			removeOneButton.onclick = function () {
				removeOne(item);
			};

			// Create "Remove All" button
			let removeAllButton = document.createElement('button');
			removeAllButton.textContent = 'Remove All';
			removeAllButton.className = 'btn';
			removeAllButton.onclick = function () {
				removeAll(item);
			};

			let itemDiv = document.createElement('div');
			itemDiv.className = 'cart-item';
			itemDiv.appendChild(p);
			itemDiv.appendChild(removeOneButton);
			itemDiv.appendChild(removeAllButton);

			cartDiv.appendChild(itemDiv);
		}
	}

	// Display total number of items
	let p = document.createElement('p');
	p.textContent = "Total items: " + totalItems;
	cartDiv.appendChild(p);
	let price = document.createElement('p');
	price.textContent = "Total price: $" + Intl.NumberFormat('en-US').format(calculateCartTotal());
	cartDiv.appendChild(price);

	let cartButton = document.getElementById('cartButton');
	cartButton.textContent = "Hide Cart";
	cartButton.setAttribute('onclick', 'hideCart()');

	cartDiv.style.display = 'block';
}

function hideCart() {
	toDisplay = false;
	let cartDiv = document.getElementById('cart');
	cartDiv.style.display = 'none';

	let cartButton = document.getElementById('cartButton');
	cartButton.textContent = "Show Cart";
	cartButton.setAttribute('onclick', 'displayCart()');
}

function calculateCartTotal() {
	let total = 0;
	for (let item in cart) {
		if (cart.hasOwnProperty(item)) {
			let quantity = cart[item];
			total += quantity * prices[item];
		}
	}
	return total;
}

function removeOne(item) {
	if (cart[item] > 0) {
		cart[item] -= 1;
		sendCookie();
		displayCart();
	}
}

function removeAll(item) {
	if (cart[item] > 0) {
		cart[item] = 0;
		sendCookie();
		displayCart();
	}
}

function getCookie(name) {
	var nameEQ = name + "=";
	var ca = document.cookie.split(';');
	console.log(ca);
	for (var i = 0; i < ca.length; i++) {
		var c = ca[i];
		while (c.charAt(0) == ' ') c = c.substring(1, c.length);
		if (c.indexOf(nameEQ) == 0) return parseInt(c.substring(nameEQ.length, c.length));
	}
	return 0;
}

window.onload = function () {
	cart['paperclip'] = getCookie('paperclip');
	cart['monalisa'] = getCookie('monalisa');
	cart['spaceshuttle'] = getCookie('spaceshuttle');
}