# Skyward Airways — Flight Ticket Booking Platform

Skyward Airways is a responsive, single-page flight-ticket booking platform created for an academic project submission. It demonstrates the complete customer journey: searching airports, comparing generated flight options, filtering and sorting results, selecting a cabin and seats, choosing add-ons, validating payment-form input, generating a boarding pass/e-ticket, and managing saved bookings.

## Run the project

No installation is required.

1. Open `final website.html` in Chrome, Edge, or any modern browser.
2. Select a departure city, destination, date, and passenger count.
3. Click **Search flights** and complete the booking flow.

The app stores bookings, loyalty miles, and reserved seats in the browser's local storage. They persist after refreshing the page on the same browser/device.

## Important project note

This is a front-end academic demonstration. The flight inventory, fares, seat availability, payment authorisation, QR codes, and booking records are simulated. It does not process live payments or issue airline tickets.

## Submission files

- `final website.html` — complete standalone application
- `README.md` — setup and usage guide
- `PROJECT_REPORT.md` — project documentation
- `VIVA_GUIDE.md` — presentation and viva preparation notes

## Technology

- HTML5 for page structure
- CSS3 for responsive layout, styling, and animation
- Vanilla JavaScript for application state, search, filters, checkout, ticket generation, and browser storage
- Browser Local Storage for persistent demo bookings

## Main features

- Airport autocomplete using an Indian and international airport catalogue
- Destination-based currency display
- Generated flight results with price, airline, duration, stops, and availability
- Stop, time, airline, and price filters plus sorting
- Passenger, cabin, meal, add-on, and seat selection
- Client-side payment-form validation; sensitive card data is never stored
- Generated boarding pass, e-ticket, QR code, loyalty miles, printing, saved trips, and cancellation
