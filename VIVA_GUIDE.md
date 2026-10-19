# Skyward Airways — Viva Guide

## 30-second introduction

“Skyward Airways is a responsive flight ticket booking platform built with HTML, CSS, and vanilla JavaScript. It lets a user search airports, compare and filter flights, select seats and add-ons, validate payment information, generate a ticket and boarding pass, and manage saved trips. Browser Local Storage keeps the demonstration bookings and seat selections after refresh.”

## Demo sequence

1. Open `final website.html` and wait for the opening animation.
2. Search from Delhi to Mumbai with tomorrow's date.
3. Show autocomplete by typing part of a city name.
4. On results, demonstrate a filter and a sort option.
5. Select a flight and explain the three-step booking process.
6. Pick a cabin, fill passenger names, select seats, and show the fare change after add-ons.
7. Confirm the demo booking to show the e-ticket, boarding pass, QR code, and loyalty miles.
8. Open **My Trips**, show persistence after refresh, then demonstrate cancellation.

## Likely questions and short answers

**Why did you use Local Storage?**  
It provides persistent browser-side storage without a backend, which makes the project easy to run and demonstrate. The saved data is scoped to the current browser/device.

**How is a booking reference created?**  
JavaScript combines the `SKW-` prefix with a short random uppercase value, which is sufficient for a front-end demonstration.

**How does seat selection work?**  
The app creates a 14-row aircraft layout. Selected seats are saved per flight and date in Local Storage, so they remain unavailable until a booking is cancelled.

**How are ticket prices calculated?**  
The base fare is adjusted by cabin multiplier, then add-ons, premium-seat fees, and 12% taxes are added. Currency display changes according to destination.

**Does it make real payments?**  
No. The payment step validates fields only and deliberately never stores card details. Real payments require a secure backend and a gateway such as Stripe or Razorpay.

**How can it use live flights later?**  
Replace the generated flight function with a secure backend call to a flight/GDS API and keep its credentials off the client.

## Concepts to mention

- DOM manipulation and event listeners
- Form validation and error feedback
- Arrays, objects, filtering, mapping, and sorting
- Browser Local Storage and JSON serialisation
- Responsive CSS media queries
- Dynamic ticket and QR-code generation
- Separation between simulated front-end behaviour and production-grade backend requirements
