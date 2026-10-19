# Skyward Airways: Flight Ticket Booking Platform

## 1. Abstract

Skyward Airways is a browser-based flight ticket booking platform designed to simplify the airline booking experience. Users can search a curated airport catalogue, compare flights, apply filters, select passenger and cabin details, reserve seats, choose payment methods, and receive a digital boarding pass and e-ticket. The project focuses on presenting a complete and visually engaging booking workflow using only front-end web technologies.

## 2. Problem statement

Many users find travel booking interfaces difficult because the journey from search to confirmation is fragmented. This project demonstrates one unified interface that guides a user through route selection, flight comparison, traveller details, seat selection, payment validation, and ticket confirmation.

## 3. Objectives

- Build a responsive, attractive airline-booking user interface.
- Provide airport search, validation, route search, filters, and flight sorting.
- Simulate a realistic multi-step booking process.
- Preserve bookings and loyalty data between browser sessions.
- Generate a printable ticket and boarding pass after booking.

## 4. Technology stack

| Layer | Technology | Purpose |
|---|---|---|
| Structure | HTML5 | Semantic page layout and forms |
| Styling | CSS3 | Responsive design, cards, animation, print layout |
| Behaviour | Vanilla JavaScript | Search, filters, booking state, validation, ticket generation |
| Storage | Browser Local Storage | Saves bookings, miles, and reserved seats locally |

## 5. Functional modules

| Module | What it does |
|---|---|
| Airport search | Suggests airports while the user types and validates origin/destination. |
| Flight search | Generates route-specific demo flights based on selected locations and date. |
| Results | Displays airline, timing, duration, stops, fare, rating, aircraft, and seats left. |
| Filters | Filters by stops, departure time, airline, and price; sorts by price, duration, or rating. |
| Booking | Collects passenger information, cabin class, meal, add-ons, and seats. |
| Payment | Validates input for the selected demo payment method without storing sensitive values. |
| Confirmation | Creates a booking reference, e-ticket, boarding pass, QR code, and loyalty miles. |
| My trips | Displays saved bookings and supports cancellation. |

## 6. Data model

The application contains a local catalogue of airports, airlines, currencies, and exchange-rate display values. At runtime it creates flight objects with route, timing, seats, aircraft, fare, airline, and booking information. Browser Local Storage uses these keys:

- `skw_bookings`: completed demonstration bookings.
- `skw_miles`: loyalty-mile balance.
- `skw_reserved_seats`: reserved seat labels for each searched flight.

## 7. Validation and workflow

1. The user selects valid origin and destination airports and a future departure date.
2. The system prevents identical origin/destination selections.
3. The user selects one flight, completes traveller names, and chooses the same number of seats as passengers.
4. The payment form validates its active method. Card input uses a client-side checksum/format check and is never persisted.
5. The confirmed booking is saved locally, and selected seats become unavailable for the active demo flight.
6. Cancelling a saved trip releases its selected seats and removes that booking from local storage.

## 8. Testing summary

| Test case | Expected result |
|---|---|
| Empty search date | A prompt asks the user to choose a date. |
| Same origin and destination | Search is prevented with a message. |
| Empty passenger names | The workflow cannot move to seat selection. |
| Too few seats | The workflow cannot move to payment. |
| Invalid card data | Confirmation is blocked with a validation message. |
| Confirmed booking | Ticket, boarding pass, miles, and saved trip are created. |
| Cancel booking | Booking is removed and its seats are released. |

## 9. Limitations and future scope

The current project is a front-end demonstration. Flight data, fares, payments, and QR codes are simulated; no real airline ticket is issued. A production version could add a secure server, database, user authentication, an airline/GDS API such as Amadeus, email notifications, payment-gateway integration, and role-based admin controls.

## 10. Conclusion

Skyward Airways demonstrates the complete interaction design of a modern ticket-booking platform. It successfully combines usability, validation, responsive design, state management, persistent browser storage, and printable travel documents in a submission-ready standalone website.
