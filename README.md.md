# ✈️ Skyward Airways — Airline Booking System

A premium full-stack airline-booking demo built with vanilla HTML, CSS, JavaScript, and a zero-dependency Node.js server.

## Run locally

1. Open a terminal in this folder.
2. Run `node server.js`.
3. Visit [http://localhost:3000](http://localhost:3000).

Bookings are stored locally in `data/bookings.json`. The booking flow validates passenger, seat, and card-format fields; card numbers and CVVs are never sent to or stored by the server. This is an academic/demo payment workflow, not a live payment gateway.

## Working features

- Route-based flight search for the available popular routes
- Server-backed seat availability that prevents duplicate reservations
- Passenger, cabin, add-on, and payment-form validation
- Persistent bookings and cancellation through the local API
- Animated, responsive interface with saved trip management
