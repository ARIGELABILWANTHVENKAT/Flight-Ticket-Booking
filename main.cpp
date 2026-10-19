// ============================================================================
//  SKYWARD AIRWAYS  -  Flight Ticket Booking System
//  Object-Oriented Programming in C++ (end-semester project)
//
//  Build :  g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o skyward
//  Run   :  ./skyward        (Windows: skyward.exe)
//
//  OOP concepts demonstrated
//   - Abstraction & inheritance : Flight -> DomesticFlight / InternationalFlight
//                                 Passenger -> Adult / Child / Senior
//                                 Payment -> CardPayment / UpiPayment / NetBankingPayment
//   - Polymorphism              : virtual fares, tax rules, discounts, payment flows
//   - Encapsulation             : private data + getters in every class
//   - Operator overloading      : Money (+ - * < <<), Date (< ==, <<), Fare (<<)
//   - Templates                 : Repository<T> generic store
//   - Exceptions                : BookingError hierarchy
//   - STL & lambdas             : vector, map, set, sort, remove_if, std::function
//   - Smart pointers            : unique_ptr / shared_ptr (no raw new/delete)
//   - File handling             : bookings.dat, accounts.dat, custom_flights.dat
// ============================================================================
#define NOMINMAX
#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

// ----------------------------------------------------------------------------
//  Small utilities
// ----------------------------------------------------------------------------
namespace util {
inline string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
inline string upper(string s) { for (auto& c : s) c = (char)toupper((unsigned char)c); return s; }
inline string lower(string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
inline unsigned fnv(const string& s) {
    unsigned h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}
inline string clean(string s) {  // keep our file format safe
    for (char& c : s) if (c == '|' || c == ';' || c == '~' || c == '\n' || c == '\r') c = ' ';
    return trim(s);
}
inline vector<string> split(const string& s, char d) {
    vector<string> out; string cur; stringstream ss(s);
    while (getline(ss, cur, d)) out.push_back(cur);
    if (!s.empty() && s.back() == d) out.push_back("");
    return out;
}
inline string pad(const string& s, size_t w) {
    return s.size() >= w ? s.substr(0, w) : s + string(w - s.size(), ' ');
}
inline string padLeft(const string& s, size_t w) {
    return s.size() >= w ? s : string(w - s.size(), ' ') + s;
}
inline string hhmm(int minutes) {
    minutes = ((minutes % 1440) + 1440) % 1440;
    ostringstream o; o << setfill('0') << setw(2) << minutes / 60 << ':' << setw(2) << minutes % 60;
    return o.str();
}
}  // namespace util

// ----------------------------------------------------------------------------
//  Terminal helpers (colours, input)
// ----------------------------------------------------------------------------
namespace ui {
bool color = true;
inline string paint(const string& s, const char* code) {
    return color ? string("\033[") + code + "m" + s + "\033[0m" : s;
}
inline string bold(const string& s)   { return paint(s, "1"); }
inline string red(const string& s)    { return paint(s, "31"); }
inline string green(const string& s)  { return paint(s, "32"); }
inline string yellow(const string& s) { return paint(s, "33"); }
inline string cyan(const string& s)   { return paint(s, "36"); }
inline string dim(const string& s)    { return paint(s, "2"); }

inline void title(const string& t) {
    cout << "\n" << bold(cyan("  " + t)) << "\n" << string(72, '-') << "\n";
}
inline void ok(const string& s)   { cout << "  " << green("[OK] ") << s << "\n"; }
inline void warn(const string& s) { cout << "  " << yellow("[!]  ") << s << "\n"; }
inline void err(const string& s)  { cout << "  " << red("[X]  ") << s << "\n"; }
inline void info(const string& s) { cout << "  " << dim(s) << "\n"; }
}  // namespace ui

struct InputClosed {};  // thrown when stdin ends (Ctrl+D / Ctrl+Z / piped input finished)

string ask(const string& prompt) {
    cout << prompt << flush;
    string s;
    if (!getline(cin, s)) throw InputClosed();
    return util::trim(s);
}
string askNonEmpty(const string& prompt) {
    while (true) {
        string s = ask(prompt);
        if (!s.empty()) return s;
        ui::err("This cannot be empty.");
    }
}
int askInt(const string& prompt, int lo, int hi) {
    while (true) {
        string s = ask(prompt);
        try {
            size_t pos = 0;
            int v = stoi(s, &pos);
            if (pos == s.size() && v >= lo && v <= hi) return v;
        } catch (...) {}
        ui::err("Please enter a number between " + to_string(lo) + " and " + to_string(hi) + ".");
    }
}
bool askYesNo(const string& prompt) {
    while (true) {
        string s = util::lower(ask(prompt + " (y/n): "));
        if (s == "y" || s == "yes") return true;
        if (s == "n" || s == "no") return false;
        ui::err("Please answer y or n.");
    }
}

// ----------------------------------------------------------------------------
//  Exceptions
// ----------------------------------------------------------------------------
class BookingError : public runtime_error { public: using runtime_error::runtime_error; };
class InvalidInput : public BookingError   { public: using BookingError::BookingError; };
class SeatUnavailable : public BookingError{ public: using BookingError::BookingError; };
class NotFound : public BookingError       { public: using BookingError::BookingError; };

// ----------------------------------------------------------------------------
//  Money  (whole rupees, operator overloading, Indian digit grouping)
// ----------------------------------------------------------------------------
class Money {
    long long rs_;
public:
    Money(long long rs = 0) : rs_(rs) {}
    static Money fromDouble(double d) { return Money(llround(d)); }
    long long value() const { return rs_; }
    Money operator+(const Money& o) const { return Money(rs_ + o.rs_); }
    Money operator-(const Money& o) const { return Money(rs_ - o.rs_); }
    Money operator*(double f) const { return Money(llround(rs_ * f)); }
    Money operator*(int n) const { return Money(rs_ * n); }
    Money& operator+=(const Money& o) { rs_ += o.rs_; return *this; }
    bool operator<(const Money& o) const { return rs_ < o.rs_; }
    bool operator>(const Money& o) const { return rs_ > o.rs_; }
    string str() const {
        string s = to_string(rs_ < 0 ? -rs_ : rs_), out;
        if (s.size() > 3) {
            string last3 = s.substr(s.size() - 3), rest = s.substr(0, s.size() - 3), g;
            for (size_t i = 0; i < rest.size(); ++i) {
                if (i > 0 && (rest.size() - i) % 2 == 0) g += ',';
                g += rest[i];
            }
            out = g + "," + last3;
        } else out = s;
        return (rs_ < 0 ? "-Rs. " : "Rs. ") + out;
    }
    friend ostream& operator<<(ostream& os, const Money& m) { return os << m.str(); }
};

// ----------------------------------------------------------------------------
//  Date  (no external libs; proleptic Gregorian calendar maths)
// ----------------------------------------------------------------------------
class Date {
    int y_ = 1970, m_ = 1, d_ = 1;
    static long long toDays(long long y, long long m, long long d) {
        y -= (m <= 2);
        long long era = (y >= 0 ? y : y - 399) / 400;
        long long yoe = y - era * 400;
        long long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
        long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097 + doe - 719468;
    }
    static void fromDays(long long z, int& y, int& m, int& d) {
        z += 719468;
        long long era = (z >= 0 ? z : z - 146096) / 146097;
        long long doe = z - era * 146097;
        long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        long long yy = yoe + era * 400;
        long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        long long mp = (5 * doy + 2) / 153;
        d = (int)(doy - (153 * mp + 2) / 5 + 1);
        m = (int)(mp < 10 ? mp + 3 : mp - 9);
        y = (int)(yy + (m <= 2));
    }
public:
    Date() = default;
    Date(int y, int m, int d) : y_(y), m_(m), d_(d) {}
    static bool valid(int y, int m, int d) {
        if (y < 1970 || y > 2200 || m < 1 || m > 12 || d < 1) return false;
        static const int dm[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
        return d <= dm[m - 1] + (m == 2 && leap ? 1 : 0);
    }
    static bool parse(const string& s, Date& out) {
        if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
        try {
            int y = stoi(s.substr(0, 4)), m = stoi(s.substr(5, 2)), d = stoi(s.substr(8, 2));
            if (!valid(y, m, d)) return false;
            out = Date(y, m, d);
            return true;
        } catch (...) { return false; }
    }
    static Date today() {
        time_t t = time(nullptr);
        tm* lt = localtime(&t);
        return Date(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
    }
    long long days() const { return toDays(y_, m_, d_); }
    Date plusDays(int n) const { int y, m, d; fromDays(days() + n, y, m, d); return Date(y, m, d); }
    int daysFrom(const Date& o) const { return (int)(days() - o.days()); }
    string str() const {
        ostringstream o;
        o << setfill('0') << setw(4) << y_ << '-' << setw(2) << m_ << '-' << setw(2) << d_;
        return o.str();
    }
    string pretty() const {
        static const char* wd[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        static const char* mn[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        ostringstream o;
        o << wd[(days() + 4) % 7] << ", " << d_ << ' ' << mn[m_ - 1] << ' ' << y_;
        return o.str();
    }
    bool operator<(const Date& o) const { return days() < o.days(); }
    bool operator==(const Date& o) const { return days() == o.days(); }
    friend ostream& operator<<(ostream& os, const Date& d) { return os << d.str(); }
};

// ----------------------------------------------------------------------------
//  Airports, cabins, seat layout
// ----------------------------------------------------------------------------
struct Airport {
    string code, city, country;
    double lat, lon;
    bool domestic() const { return country == "India"; }
};
double distanceKm(const Airport& a, const Airport& b) {  // haversine
    const double R = 6371.0, k = 3.14159265358979 / 180.0;
    double dlat = (b.lat - a.lat) * k, dlon = (b.lon - a.lon) * k;
    double h = sin(dlat / 2) * sin(dlat / 2) + cos(a.lat * k) * cos(b.lat * k) * sin(dlon / 2) * sin(dlon / 2);
    return 2 * R * asin(sqrt(h));
}

enum class Cabin { Economy, Business, First };
inline string cabinName(Cabin c) { return c == Cabin::First ? "First" : c == Cabin::Business ? "Business" : "Economy"; }
inline double cabinMultiplier(Cabin c) { return c == Cabin::First ? 4.2 : c == Cabin::Business ? 2.4 : 1.0; }

constexpr int ROWS = 14;
const string COLS = "ABCDEF";
inline Cabin cabinOfRow(int r) { return r <= 2 ? Cabin::First : r <= 4 ? Cabin::Business : Cabin::Economy; }
inline bool extraLegroom(int r) { return r == 5 || r == 10; }
const Money LEGROOM_FEE(400);
constexpr int CAPACITY = ROWS * 6;

// Parses "12C" -> row 12, col 'C'. Returns false when the label is not a real seat.
bool parseSeat(const string& label, int& row, char& col) {
    if (label.size() < 2 || label.size() > 3) return false;
    try {
        size_t pos = 0;
        row = stoi(label, &pos);
        if (pos != label.size() - 1) return false;
    } catch (...) { return false; }
    col = (char)toupper((unsigned char)label.back());
    return row >= 1 && row <= ROWS && COLS.find(col) != string::npos;
}
inline Cabin cabinOfSeat(const string& label) {
    int r; char c;
    return parseSeat(label, r, c) ? cabinOfRow(r) : Cabin::Economy;
}

// ----------------------------------------------------------------------------
//  Airlines
// ----------------------------------------------------------------------------
struct AirlineInfo { string code, name; double mult; };
const vector<AirlineInfo>& airlineTable() {
    static const vector<AirlineInfo> t = {
        {"6E", "IndiGo", 1.00}, {"AI", "Air India", 1.15}, {"QP", "Akasa Air", 0.98},
        {"IX", "Air India Express", 0.95}, {"SG", "SpiceJet", 0.92}, {"EK", "Emirates", 1.30},
        {"SQ", "Singapore Airlines", 1.35}, {"QR", "Qatar Airways", 1.28}};
    return t;
}
const AirlineInfo& airlineInfo(const string& code) {
    for (auto& a : airlineTable()) if (a.code == code) return a;
    return airlineTable().front();
}

// ----------------------------------------------------------------------------
//  Flight hierarchy  (abstract base + two concrete flight types)
// ----------------------------------------------------------------------------
class Flight {
protected:
    string code_, airline_;
    const Airport *from_, *to_;
    int dep_, dur_, stops_;  // dep_ = minutes after midnight
    Money base_;
    string aircraft_;
public:
    Flight(string code, string airline, const Airport* f, const Airport* t, int dep, int dur, Money base,
           string aircraft, int stops)
        : code_(move(code)), airline_(move(airline)), from_(f), to_(t), dep_(dep), dur_(dur), stops_(stops),
          base_(base), aircraft_(move(aircraft)) {}
    virtual ~Flight() = default;

    virtual string category() const = 0;
    virtual double taxRate() const = 0;
    virtual int baggageKg() const = 0;
    virtual bool needsPassport() const = 0;

    const string& code() const { return code_; }
    const string& airlineCode() const { return airline_; }
    string airlineName() const { return airlineInfo(airline_).name; }
    const Airport& from() const { return *from_; }
    const Airport& to() const { return *to_; }
    int depMinutes() const { return dep_; }
    int duration() const { return dur_; }
    int stops() const { return stops_; }
    Money basePrice() const { return base_; }
    const string& aircraft() const { return aircraft_; }
    string depStr() const { return util::hhmm(dep_); }
    string arrStr() const { return util::hhmm(dep_ + dur_) + (dep_ + dur_ >= 1440 ? "+1" : ""); }
    string durStr() const { return to_string(dur_ / 60) + "h " + to_string(dur_ % 60) + "m"; }
    string stopsStr() const { return stops_ == 0 ? "Non-stop" : to_string(stops_) + " stop"; }
};
class DomesticFlight : public Flight {
public:
    using Flight::Flight;
    string category() const override { return "Domestic"; }
    double taxRate() const override { return 0.12; }
    int baggageKg() const override { return 15; }
    bool needsPassport() const override { return false; }
};
class InternationalFlight : public Flight {
public:
    using Flight::Flight;
    string category() const override { return "International"; }
    double taxRate() const override { return 0.18; }
    int baggageKg() const override { return 30; }
    bool needsPassport() const override { return true; }
};

// ----------------------------------------------------------------------------
//  Passenger hierarchy
// ----------------------------------------------------------------------------
class Passenger {
protected:
    string name_, passport_;
    int age_;
public:
    Passenger(string n, int a, string p) : name_(move(n)), passport_(move(p)), age_(a) {}
    virtual ~Passenger() = default;
    virtual double discount() const = 0;
    virtual string kind() const = 0;
    const string& name() const { return name_; }
    const string& passport() const { return passport_; }
    int age() const { return age_; }
    static shared_ptr<Passenger> create(const string& n, int age, const string& passport);
};
class Adult  : public Passenger { public: using Passenger::Passenger; double discount() const override { return 0.0; }  string kind() const override { return "Adult"; } };
class Child  : public Passenger { public: using Passenger::Passenger; double discount() const override { return 0.25; } string kind() const override { return "Child"; } };
class Senior : public Passenger { public: using Passenger::Passenger; double discount() const override { return 0.10; } string kind() const override { return "Senior"; } };
shared_ptr<Passenger> Passenger::create(const string& n, int age, const string& passport) {
    if (age < 12) return make_shared<Child>(n, age, passport);
    if (age >= 60) return make_shared<Senior>(n, age, passport);
    return make_shared<Adult>(n, age, passport);
}

// ----------------------------------------------------------------------------
//  Loyalty tiers
// ----------------------------------------------------------------------------
struct Tier { string name; double discount; int nextAt; };
Tier tierFor(int miles) {
    if (miles >= 40000) return {"Platinum", 0.08, 0};
    if (miles >= 15000) return {"Gold", 0.05, 40000};
    if (miles >= 5000) return {"Silver", 0.03, 15000};
    return {"Blue", 0.0, 5000};
}

// ----------------------------------------------------------------------------
//  Fare breakdown
// ----------------------------------------------------------------------------
struct Fare {
    Money base, paxDiscount, seatFees, addons, coupon, loyalty, taxes, total;
    int miles = 0;
    friend ostream& operator<<(ostream& os, const Fare& f) {
        auto line = [&](const string& l, const Money& m, bool neg = false) {
            string v = (neg && m.value() > 0 ? "-" : "") + m.str();
            os << "    " << util::pad(l, 28) << util::padLeft(v, 16) << "\n";
        };
        line("Base fare (by cabin)", f.base);
        if (f.paxDiscount.value() > 0) line("Child / senior discount", f.paxDiscount, true);
        if (f.seatFees.value() > 0) line("Extra-legroom seats", f.seatFees);
        if (f.addons.value() > 0) line("Add-ons", f.addons);
        if (f.coupon.value() > 0) line("Coupon", f.coupon, true);
        if (f.loyalty.value() > 0) line("Loyalty tier discount", f.loyalty, true);
        line("Taxes & fees", f.taxes);
        os << "    " << string(44, '-') << "\n";
        os << "    " << util::pad("TOTAL", 28) << util::padLeft(f.total.str(), 16) << "\n";
        os << "    " << ui::dim("You will earn " + to_string(f.miles) + " Skyward Miles") << "\n";
        return os;
    }
};

// add-on bitmask
enum AddOn { BAGGAGE = 1, GOURMET = 2, LOUNGE = 4, INSURANCE = 8 };
struct AddOnInfo { int bit; string name; Money price; };
const vector<AddOnInfo>& addOnTable() {
    static const vector<AddOnInfo> t = {{BAGGAGE, "Extra baggage (+23 kg)", Money(1200)},
                                       {GOURMET, "Gourmet meal", Money(750)},
                                       {LOUNGE, "Lounge access", Money(1500)},
                                       {INSURANCE, "Trip insurance", Money(950)}};
    return t;
}

// ----------------------------------------------------------------------------
//  Booking
// ----------------------------------------------------------------------------
enum class Status { Confirmed, Cancelled };

class Booking {
    string pnr_, account_, flightCode_, meal_, payment_;
    Date date_;
    vector<shared_ptr<Passenger>> pax_;
    vector<string> seats_;
    int addons_ = 0;
    Money total_, refund_;
    Status status_ = Status::Confirmed;
public:
    Booking() = default;
    Booking(string pnr, string account, string flightCode, Date d, vector<shared_ptr<Passenger>> pax,
            vector<string> seats, string meal, int addons, string payment, Money total)
        : pnr_(move(pnr)), account_(move(account)), flightCode_(move(flightCode)), meal_(move(meal)),
          payment_(move(payment)), date_(d), pax_(move(pax)), seats_(move(seats)), addons_(addons), total_(total) {}

    const string& pnr() const { return pnr_; }
    const string& account() const { return account_; }
    const string& flightCode() const { return flightCode_; }
    const string& meal() const { return meal_; }
    const string& payment() const { return payment_; }
    const Date& date() const { return date_; }
    const vector<shared_ptr<Passenger>>& passengers() const { return pax_; }
    const vector<string>& seats() const { return seats_; }
    int addons() const { return addons_; }
    Money total() const { return total_; }
    Money refund() const { return refund_; }
    Status status() const { return status_; }
    bool confirmed() const { return status_ == Status::Confirmed; }
    void cancel(Money refund) { status_ = Status::Cancelled; refund_ = refund; }

    string serialize() const {
        ostringstream o;
        o << pnr_ << '|' << util::clean(account_) << '|' << flightCode_ << '|' << date_.str() << '|'
          << (confirmed() ? 'C' : 'X') << '|' << util::clean(meal_) << '|' << addons_ << '|'
          << util::clean(payment_) << '|' << total_.value() << '|' << refund_.value() << '|';
        for (size_t i = 0; i < pax_.size(); ++i) {
            if (i) o << ';';
            o << util::clean(pax_[i]->name()) << '~' << pax_[i]->age() << '~'
              << (pax_[i]->passport().empty() ? "-" : pax_[i]->passport()) << '~' << seats_[i];
        }
        return o.str();
    }
    static bool deserialize(const string& line, Booking& b) {
        try {
            auto f = util::split(line, '|');
            if (f.size() < 11) return false;
            Date d;
            if (!Date::parse(f[3], d)) return false;
            vector<shared_ptr<Passenger>> pax; vector<string> seats;
            for (auto& p : util::split(f[10], ';')) {
                auto q = util::split(p, '~');
                if (q.size() < 4) return false;
                pax.push_back(Passenger::create(q[0], stoi(q[1]), q[2] == "-" ? "" : q[2]));
                seats.push_back(q[3]);
            }
            b = Booking(f[0], f[1], f[2], d, pax, seats, f[5], stoi(f[6]), f[7], Money(stoll(f[8])));
            if (f[4] == "X") b.cancel(Money(stoll(f[9])));
            return !pax.empty();
        } catch (...) { return false; }
    }
};

// ----------------------------------------------------------------------------
//  Generic template repository
// ----------------------------------------------------------------------------
template <typename T>
class Repository {
    vector<T> items_;
public:
    T& add(T item) { items_.push_back(move(item)); return items_.back(); }
    template <typename Pred> T* findIf(Pred p) { for (auto& i : items_) if (p(i)) return &i; return nullptr; }
    template <typename Pred> vector<T*> where(Pred p) {
        vector<T*> out;
        for (auto& i : items_) if (p(i)) out.push_back(&i);
        return out;
    }
    template <typename KeyFn> auto countBy(KeyFn key) const {
        using K = decay_t<decltype(key(declval<const T&>()))>;
        map<K, int> m;
        for (auto& i : items_) ++m[key(i)];
        return m;
    }
    vector<T>& all() { return items_; }
    const vector<T>& all() const { return items_; }
    size_t size() const { return items_.size(); }
};

// ----------------------------------------------------------------------------
//  Payment hierarchy (polymorphic checkout)
// ----------------------------------------------------------------------------
class Payment {
public:
    virtual ~Payment() = default;
    virtual bool collect(const Money& amount) = 0;  // false = user aborted
    virtual string label() const = 0;
};

class CardPayment : public Payment {
    string masked_ = "Card";
    static bool luhn(const string& digits) {
        if (digits.size() < 13 || digits.size() > 19) return false;
        int sum = 0; bool alt = false;
        for (int i = (int)digits.size() - 1; i >= 0; --i) {
            int d = digits[i] - '0';
            if (alt) { d *= 2; if (d > 9) d -= 9; }
            sum += d; alt = !alt;
        }
        return sum % 10 == 0;
    }
public:
    bool collect(const Money& amount) override {
        ui::info("Demo mode: try 4242 4242 4242 4242, any future expiry, any 3-digit CVV. Nothing is stored.");
        for (int attempt = 1; attempt <= 3; ++attempt) {
            string num = ask("  Card number: ");
            string digits;
            for (char c : num) if (isdigit((unsigned char)c)) digits += c;
            if (!luhn(digits)) { ui::err("Invalid card number (Luhn check failed)."); continue; }
            string name = askNonEmpty("  Name on card: ");
            string exp = ask("  Expiry (MM/YY): ");
            int mm = 0, yy = 0;
            if (exp.size() != 5 || exp[2] != '/' || !isdigit((unsigned char)exp[0]) || !isdigit((unsigned char)exp[1]) ||
                !isdigit((unsigned char)exp[3]) || !isdigit((unsigned char)exp[4])) {
                ui::err("Use the format MM/YY."); continue;
            }
            mm = stoi(exp.substr(0, 2)); yy = 2000 + stoi(exp.substr(3, 2));
            Date t = Date::today();
            Date today = t;
            int curY = stoi(today.str().substr(0, 4)), curM = stoi(today.str().substr(5, 2));
            if (mm < 1 || mm > 12 || yy < curY || (yy == curY && mm < curM)) { ui::err("Card has expired."); continue; }
            string cvv = ask("  CVV: ");
            if (cvv.size() < 3 || cvv.size() > 4 || !all_of(cvv.begin(), cvv.end(), [](char c) { return isdigit((unsigned char)c); })) {
                ui::err("CVV must be 3 or 4 digits."); continue;
            }
            masked_ = "Card ****" + digits.substr(digits.size() - 4);
            cout << "  Authorising " << amount << " ...\n";
            return true;
        }
        ui::err("Too many failed attempts.");
        return false;
    }
    string label() const override { return masked_; }
};

class UpiPayment : public Payment {
    string id_ = "UPI";
public:
    bool collect(const Money& amount) override {
        for (int attempt = 1; attempt <= 3; ++attempt) {
            string id = ask("  UPI ID (e.g. name@bank): ");
            size_t at = id.find('@');
            if (at == string::npos || at == 0 || at + 1 >= id.size() || id.find(' ') != string::npos) {
                ui::err("That does not look like a valid UPI ID."); continue;
            }
            id_ = "UPI " + id;
            cout << "  Collect request for " << amount << " sent to " << id << ". Approving ...\n";
            return true;
        }
        return false;
    }
    string label() const override { return id_; }
};

class NetBankingPayment : public Payment {
    string bank_ = "Net Banking";
public:
    bool collect(const Money& amount) override {
        static const vector<string> banks = {"State Bank of India", "HDFC Bank", "ICICI Bank", "Axis Bank", "Kotak Bank"};
        for (size_t i = 0; i < banks.size(); ++i) cout << "    " << i + 1 << ". " << banks[i] << "\n";
        int c = askInt("  Choose bank: ", 1, (int)banks.size());
        bank_ = "NetBanking " + banks[c - 1];
        cout << "  Redirecting to " << banks[c - 1] << " for " << amount << " ... payment received.\n";
        return true;
    }
    string label() const override { return bank_; }
};

// ----------------------------------------------------------------------------
//  Ticket rendering helpers
// ----------------------------------------------------------------------------
vector<string> qrArt(const string& seed, int n = 15) {
    mt19937 g(util::fnv(seed));
    vector<string> grid(n, string(n, ' '));
    for (int r = 0; r < n; ++r) for (int c = 0; c < n; ++c) grid[r][c] = (g() % 2) ? '#' : ' ';
    auto finder = [&](int r0, int c0) {
        for (int r = 0; r < 5; ++r) for (int c = 0; c < 5; ++c) {
            bool ring = r == 0 || r == 4 || c == 0 || c == 4, centre = r == 2 && c == 2;
            grid[r0 + r][c0 + c] = (ring || centre) ? '#' : ' ';
        }
    };
    finder(0, 0); finder(0, n - 5); finder(n - 5, 0);
    vector<string> out;
    for (auto& row : grid) { string s; for (char ch : row) s += ch == '#' ? "##" : "  "; out.push_back(s); }
    return out;
}

// ----------------------------------------------------------------------------
//  The booking system (controller)
// ----------------------------------------------------------------------------
class BookingSystem {
    struct Account { string display; int miles = 0; };

    map<string, Airport> airports_;
    vector<unique_ptr<Flight>> flights_;
    Repository<Booking> bookings_;
    map<string, Account> accounts_;
    vector<string> customLines_;
    string user_;  // lower-case key of the signed-in account
    mt19937 rng_{random_device{}()};
    const string BOOKINGS = "bookings.dat", ACCOUNTS = "accounts.dat", CUSTOM = "custom_flights.dat";
    const string ADMIN_PASSWORD = "admin123";

    // ---------------- data setup ----------------
    void buildAirports() {
        vector<Airport> a = {
            {"DEL", "Delhi", "India", 28.556, 77.100},       {"BOM", "Mumbai", "India", 19.089, 72.868},
            {"BLR", "Bengaluru", "India", 13.199, 77.706},   {"MAA", "Chennai", "India", 12.990, 80.169},
            {"HYD", "Hyderabad", "India", 17.241, 78.429},   {"CCU", "Kolkata", "India", 22.654, 88.447},
            {"VTZ", "Visakhapatnam", "India", 17.721, 83.224}, {"VGA", "Vijayawada", "India", 16.530, 80.797},
            {"TIR", "Tirupati", "India", 13.633, 79.543},    {"GOI", "Goa", "India", 15.380, 73.831},
            {"COK", "Kochi", "India", 10.152, 76.402},       {"PNQ", "Pune", "India", 18.582, 73.920},
            {"DXB", "Dubai", "UAE", 25.253, 55.366},         {"SIN", "Singapore", "Singapore", 1.364, 103.992},
            {"LHR", "London", "UK", 51.470, -0.454},         {"BKK", "Bangkok", "Thailand", 13.690, 100.750}};
        for (auto& x : a) airports_[x.code] = x;
    }

    bool addFlight(const string& code, const string& airline, const string& from, const string& to, int dep,
                   int dur, Money price, const string& aircraft, int stops) {
        auto f = airports_.find(from), t = airports_.find(to);
        if (f == airports_.end() || t == airports_.end() || from == to) return false;
        if (findFlight(code)) return false;
        bool dom = f->second.domestic() && t->second.domestic();
        if (dom) flights_.push_back(make_unique<DomesticFlight>(code, airline, &f->second, &t->second, dep, dur, price, aircraft, stops));
        else flights_.push_back(make_unique<InternationalFlight>(code, airline, &f->second, &t->second, dep, dur, price, aircraft, stops));
        return true;
    }

    void buildNetwork() {
        static const vector<string> domA = {"6E", "AI", "QP", "IX", "SG"}, intA = {"EK", "SQ", "QR", "AI"};
        static const vector<string> small = {"Airbus A320", "Boeing 737-800", "ATR 72"};
        static const vector<string> big = {"Boeing 787-9", "Airbus A350", "Boeing 777-300ER"};
        int number = 100;
        for (auto& [fc, fa] : airports_) for (auto& [tc, ta] : airports_) {
            if (fc == tc) continue;
            double km = distanceKm(fa, ta);
            if (km < 120) continue;
            bool dom = fa.domestic() && ta.domestic();
            for (int k = 0; k < 3; ++k) {
                unsigned h = util::fnv(fc + tc + to_string(k));
                const string& al = dom ? domA[h % domA.size()] : intA[h % intA.size()];
                int dep = 360 + k * 360 + (int)(h % 12) * 15;
                int stops = (k == 1 && km > 600 && dom) ? 1 : 0;
                int dur = (int)(km / (dom ? 780.0 : 830.0) * 60) + 35 + stops * 70;
                double price = dom ? 1400 + km * 3.1 : 4500 + km * 4.0;
                price *= airlineInfo(al).mult * (stops ? 0.88 : 1.0) * (0.95 + (h % 11) / 100.0);
                const string& ac = (km > 1800) ? big[h % big.size()] : small[h % small.size()];
                addFlight(al + "-" + to_string(number++), al, fc, tc, dep, dur, Money(llround(price / 10) * 10), ac, stops);
            }
        }
    }

    const Flight* findFlight(const string& code) const {
        for (auto& f : flights_) if (f->code() == code) return f.get();
        return nullptr;
    }

    // ---------------- persistence ----------------
    void loadCustomFlights() {
        ifstream in(CUSTOM);
        string line;
        while (getline(in, line)) {
            auto f = util::split(line, '|');
            if (f.size() < 9) continue;
            try {
                if (addFlight(f[0], f[1], f[2], f[3], stoi(f[4]), stoi(f[5]), Money(stoll(f[6])), f[7], stoi(f[8])))
                    customLines_.push_back(line);
            } catch (...) {}
        }
    }
    void load() {
        loadCustomFlights();
        ifstream in(BOOKINGS);
        string line; int bad = 0;
        while (getline(in, line)) {
            if (line.empty()) continue;
            Booking b;
            if (Booking::deserialize(line, b)) bookings_.add(b); else ++bad;
        }
        if (bad) ui::warn(to_string(bad) + " damaged record(s) in bookings.dat were skipped.");
        ifstream ac(ACCOUNTS);
        while (getline(ac, line)) {
            auto f = util::split(line, '|');
            if (f.size() < 2) continue;
            try { accounts_[util::lower(f[0])] = {f[0], stoi(f[1])}; } catch (...) {}
        }
    }
    void save() {
        auto writeSafely = [](const string& path, const function<void(ofstream&)>& fn) {
            string tmp = path + ".tmp";
            { ofstream out(tmp); fn(out); }
            remove(path.c_str());
            rename(tmp.c_str(), path.c_str());
        };
        writeSafely(BOOKINGS, [&](ofstream& o) { for (auto& b : bookings_.all()) o << b.serialize() << "\n"; });
        writeSafely(ACCOUNTS, [&](ofstream& o) { for (auto& [k, a] : accounts_) o << util::clean(a.display) << '|' << a.miles << "\n"; });
        writeSafely(CUSTOM, [&](ofstream& o) { for (auto& l : customLines_) o << l << "\n"; });
    }

    // ---------------- seat / price logic ----------------
    static vector<string> allSeats() {
        vector<string> v;
        for (int r = 1; r <= ROWS; ++r) for (char c : COLS) v.push_back(to_string(r) + c);
        return v;
    }
    set<string> occupiedSeats(const Flight& f, const Date& d) {
        set<string> taken;
        // deterministic "other passengers" so every flight/day has a believable cabin
        mt19937 g(util::fnv(f.code() + d.str()));
        auto seats = allSeats();
        for (int i = (int)seats.size() - 1; i > 0; --i) swap(seats[i], seats[g() % (i + 1)]);
        int n = 12 + (int)(g() % 40);
        for (int i = 0; i < n; ++i) taken.insert(seats[i]);
        for (auto* b : bookings_.where([&](const Booking& b) { return b.confirmed() && b.flightCode() == f.code() && b.date() == d; }))
            for (auto& s : b->seats()) taken.insert(s);
        return taken;
    }
    // Economy price per person: demand (occupancy) and days-to-go make the price move.
    Money unitPrice(const Flight& f, const Date& d) {
        double occ = (double)occupiedSeats(f, d).size() / CAPACITY;
        int days = d.daysFrom(Date::today());
        double timeF = days < 3 ? 1.25 : days < 7 ? 1.12 : days > 60 ? 0.92 : 1.0;
        return Money::fromDouble(f.basePrice().value() * (1.0 + 0.6 * occ * occ) * timeF / 10.0) * 10;
    }
    static bool departed(const Flight& f, const Date& d) {
        Date t = Date::today();
        if (t < d) return false;
        if (d < t) return true;
        time_t now = time(nullptr);
        tm* lt = localtime(&now);
        return f.depMinutes() <= lt->tm_hour * 60 + lt->tm_min + 45;  // check-in closes 45 min before
    }

    struct QuoteInput {
        const Flight* f; Date date;
        vector<shared_ptr<Passenger>> pax; vector<string> seats;
        int addons = 0; string coupon;
    };
    bool firstBooking() {
        return bookings_.findIf([&](const Booking& b) { return util::lower(b.account()) == user_; }) == nullptr;
    }
    Money couponValue(const string& code, Money subtotal, string& msg) {
        string c = util::upper(code);
        if (c.empty()) { msg = ""; return Money(0); }
        if (c == "SKY10") { msg = "SKY10: 10% off (max Rs. 1,500)"; return min(subtotal * 0.10, Money(1500)); }
        if (c == "STUDENT") { msg = "STUDENT: 7% off (max Rs. 1,000)"; return min(subtotal * 0.07, Money(1000)); }
        if (c == "WELCOME") {
            if (firstBooking()) { msg = "WELCOME: flat Rs. 500 off your first trip"; return Money(500); }
            msg = "WELCOME works only on a first booking."; return Money(0);
        }
        msg = "Unknown coupon code."; return Money(0);
    }
    Fare quote(const QuoteInput& q, string* couponMsg = nullptr) {
        Fare fa;
        Money unit = unitPrice(*q.f, q.date);
        Money gross, disc, legroom;
        for (size_t i = 0; i < q.pax.size(); ++i) {
            Cabin cab = i < q.seats.size() ? cabinOfSeat(q.seats[i]) : Cabin::Economy;
            Money seatFare = unit * cabinMultiplier(cab);
            gross += seatFare;
            disc += seatFare * q.pax[i]->discount();
            if (i < q.seats.size()) { int r; char c; if (parseSeat(q.seats[i], r, c) && extraLegroom(r)) legroom += LEGROOM_FEE; }
        }
        Money addons;
        for (auto& a : addOnTable()) if (q.addons & a.bit) addons += a.price * (int)q.pax.size();
        Money afterPax = gross - disc;
        string msg;
        Money coup = couponValue(q.coupon, afterPax, msg);
        if (couponMsg) *couponMsg = msg;
        double td = tierFor(accounts_[user_].miles).discount;
        Money loy = (afterPax - coup) * td;
        Money taxable = afterPax - coup - loy + legroom + addons;
        fa.base = gross; fa.paxDiscount = disc; fa.seatFees = legroom; fa.addons = addons; fa.coupon = coup; fa.loyalty = loy;
        fa.taxes = taxable * q.f->taxRate();
        fa.total = taxable + fa.taxes;
        fa.miles = (int)(fa.total.value() / 10);
        return fa;
    }

    // ---------------- UI pieces ----------------
    void showSeatMap(const set<string>& taken, const vector<string>& chosen, const Flight& f, const Date& d) {
        Money unit = unitPrice(f, d);
        cout << "\n      " << ui::bold("A  B  C      D  E  F") << "        " << ui::dim("<- cockpit") << "\n";
        Cabin last = Cabin::Economy; bool first = true;
        for (int r = 1; r <= ROWS; ++r) {
            Cabin cab = cabinOfRow(r);
            if (first || cab != last) {
                string tag = cabinName(cab) + "  (" + (unit * cabinMultiplier(cab)).str() + " per seat)";
                cout << "  " << ui::yellow("-- " + tag + " --") << "\n";
                last = cab; first = false;
            }
            cout << "  " << setw(2) << r << "  ";
            for (int c = 0; c < 6; ++c) {
                string label = to_string(r) + COLS[c];
                bool isChosen = find(chosen.begin(), chosen.end(), label) != chosen.end();
                if (isChosen) cout << ui::green("[*]");
                else if (taken.count(label)) cout << ui::red("[X]");
                else cout << "[ ]";
                if (c == 2) cout << "   ";
            }
            if (extraLegroom(r)) cout << "  " << ui::cyan("extra legroom +" + LEGROOM_FEE.str());
            cout << "\n";
        }
        cout << "  " << ui::dim("[ ] free   ") << ui::red("[X] taken   ") << ui::green("[*] your pick") << "\n\n";
    }

    const Airport* findAirport(const string& q) {
        string uq = util::upper(q), lq = util::lower(q);
        for (auto& [c, a] : airports_) if (c == uq || util::lower(a.city) == lq) return &a;
        vector<const Airport*> m;
        for (auto& [c, a] : airports_) if (util::lower(a.city).find(lq) != string::npos) m.push_back(&a);
        if (m.size() == 1) return m[0];
        if (m.size() > 1) {
            string s; for (auto* a : m) s += a->city + " (" + a->code + ")  ";
            ui::warn("Did you mean: " + s);
        } else ui::err("No airport matches '" + q + "'. Type ? to see all airports.");
        return nullptr;
    }
    const Airport* askAirport(const string& prompt, const Airport* avoid = nullptr) {
        while (true) {
            string q = ask(prompt);
            if (q.empty()) continue;
            if (q == "?") {
                for (auto& [c, a] : airports_) cout << "    " << c << "  " << util::pad(a.city, 15) << a.country << "\n";
                continue;
            }
            const Airport* a = findAirport(q);
            if (!a) continue;
            if (avoid && a == avoid) { ui::err("Origin and destination must be different."); continue; }
            return a;
        }
    }
    Date askDate() {
        Date today = Date::today();
        while (true) {
            string s = util::lower(ask("  Date (YYYY-MM-DD, 't' = tomorrow, '+N' = N days from now): "));
            Date d;
            if (s == "t") d = today.plusDays(1);
            else if (s.size() > 1 && s[0] == '+' && all_of(s.begin() + 1, s.end(), ::isdigit)) d = today.plusDays(stoi(s.substr(1)));
            else if (!Date::parse(s, d)) { ui::err("Invalid date. Example: " + today.plusDays(7).str()); continue; }
            if (d < today) { ui::err("That date is in the past."); continue; }
            if (d.daysFrom(today) > 330) { ui::err("Bookings open up to 330 days ahead."); continue; }
            return d;
        }
    }

    // ---------------- flows ----------------
    void login() {
        ui::title("Sign in");
        string name = askNonEmpty("  Your name: ");
        user_ = util::lower(util::clean(name));
        if (!accounts_.count(user_)) {
            accounts_[user_] = {util::clean(name), 0};
            ui::ok("New Skyward account created. Use coupon WELCOME on your first trip!");
        } else {
            Tier t = tierFor(accounts_[user_].miles);
            ui::ok("Welcome back, " + accounts_[user_].display + "  [" + t.name + " | " + to_string(accounts_[user_].miles) + " miles]");
        }
    }

    void searchAndBook() {
        ui::title("Search flights");
        const Airport* from = askAirport("  From (city or code, ? = list): ");
        const Airport* to = askAirport("  To   (city or code, ? = list): ", from);
        Date date = askDate();
        int npax = askInt("  Passengers (1-4): ", 1, 4);
        browseFlights(*from, *to, date, npax);
    }

    void printFlightTable(const vector<const Flight*>& list, const Date& date) {
        cout << "\n  " << ui::bold(util::pad("#", 3) + util::pad("Flight", 9) + util::pad("Airline", 19) + util::pad("Dep", 7) +
                                   util::pad("Arr", 8) + util::pad("Duration", 10) + util::pad("Stops", 10) +
                                   util::pad("Seats", 6) + "Economy fare") << "\n";
        cout << "  " << string(88, '-') << "\n";
        Money cheapest(1LL << 60);
        for (auto* f : list) cheapest = min(cheapest, unitPrice(*f, date));
        for (size_t i = 0; i < list.size(); ++i) {
            const Flight& f = *list[i];
            int left = CAPACITY - (int)occupiedSeats(f, date).size();
            Money p = unitPrice(f, date);
            string row = util::pad(to_string(i + 1), 3) + util::pad(f.code(), 9) + util::pad(f.airlineName(), 19) +
                         util::pad(f.depStr(), 7) + util::pad(f.arrStr(), 8) + util::pad(f.durStr(), 10) +
                         util::pad(f.stopsStr(), 10) + util::pad(to_string(left), 6) + p.str();
            cout << "  " << (left <= 8 ? ui::yellow(row) : row);
            if (!(cheapest < p)) cout << "  " << ui::green("<- cheapest");
            if (left <= 8) cout << "  " << ui::red("only " + to_string(left) + " left!");
            cout << "\n";
        }
    }

    void priceCalendar(const Airport& from, const Airport& to, const Date& start) {
        ui::title("Price calendar  " + from.code + " -> " + to.code);
        vector<pair<Date, Money>> days;
        for (int i = 0; i < 7; ++i) {
            Date d = start.plusDays(i);
            Money best(1LL << 60); bool any = false;
            for (auto& f : flights_)
                if (&f->from() == &from && &f->to() == &to && !departed(*f, d)) { best = min(best, unitPrice(*f, d)); any = true; }
            if (any) days.push_back({d, best});
        }
        if (days.empty()) { ui::warn("No flights found."); return; }
        Money lo = days[0].second, hi = days[0].second;
        for (auto& p : days) { lo = min(lo, p.second); hi = max(hi, p.second); }
        for (auto& [d, p] : days) {
            int bar = (int)(30.0 * p.value() / hi.value());
            cout << "  " << util::pad(d.pretty(), 17) << util::padLeft(p.str(), 12) << "  "
                 << (p.value() == lo.value() ? ui::green(string(bar, '#')) : string(bar, '#'))
                 << (p.value() == lo.value() ? ui::green("  <- cheapest day") : "") << "\n";
        }
        ui::info("Prices rise as the departure date nears and as the cabin fills up.");
    }

    void browseFlights(const Airport& from, const Airport& to, const Date& date, int npax) {
        string sort = "price"; bool nonstopOnly = false;
        while (true) {
            vector<const Flight*> list;
            for (auto& f : flights_)
                if (&f->from() == &from && &f->to() == &to && !departed(*f, date) && (!nonstopOnly || f->stops() == 0))
                    list.push_back(f.get());
            if (list.empty()) {
                ui::warn("No flights available for " + from.city + " -> " + to.city + " on " + date.pretty() + ".");
                ui::info("Tip: try another date or turn off the non-stop filter.");
                if (!nonstopOnly) return;
                nonstopOnly = false; continue;
            }
            if (sort == "price") std::sort(list.begin(), list.end(), [&](const Flight* a, const Flight* b) { return unitPrice(*a, date) < unitPrice(*b, date); });
            else if (sort == "duration") std::sort(list.begin(), list.end(), [](const Flight* a, const Flight* b) { return a->duration() < b->duration(); });
            else std::sort(list.begin(), list.end(), [](const Flight* a, const Flight* b) { return a->depMinutes() < b->depMinutes(); });

            ui::title(from.city + " (" + from.code + ") -> " + to.city + " (" + to.code + ")   " + date.pretty() +
                      "   " + to_string(npax) + " pax   [" + list[0]->category() + "]");
            printFlightTable(list, date);
            cout << "\n  Enter flight # to book | " << ui::bold("P") << "rice " << ui::bold("D") << "uration "
                 << ui::bold("T") << "ime sort | " << ui::bold("N") << "on-stop filter (" << (nonstopOnly ? "on" : "off") << ") | "
                 << ui::bold("C") << "alendar | " << ui::bold("B") << "ack\n";
            string c = util::lower(ask("  > "));
            if (c == "b" || c == "back") return;
            else if (c == "p") sort = "price";
            else if (c == "d") sort = "duration";
            else if (c == "t") sort = "time";
            else if (c == "n") nonstopOnly = !nonstopOnly;
            else if (c == "c") priceCalendar(from, to, date);
            else {
                try {
                    size_t pos; int n = stoi(c, &pos);
                    if (pos == c.size() && n >= 1 && n <= (int)list.size()) { bookFlow(*list[n - 1], date, npax); return; }
                } catch (...) {}
                ui::err("Pick a flight number from the list or one of the letters.");
            }
        }
    }

    void dealFinder() {
        ui::title("Deal finder - cheapest places to fly");
        const Airport* from = askAirport("  Flying from (city or code): ");
        Date date = askDate();
        map<string, Money> best;
        for (auto& f : flights_)
            if (&f->from() == from && !departed(*f, date)) {
                Money p = unitPrice(*f, date);
                auto it = best.find(f->to().city);
                if (it == best.end() || p < it->second) best[f->to().city] = p;
            }
        vector<pair<string, Money>> v(best.begin(), best.end());
        std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second < b.second; });
        cout << "\n";
        for (size_t i = 0; i < v.size() && i < 8; ++i)
            cout << "   " << i + 1 << ". " << util::pad(v[i].first, 16) << util::padLeft(v[i].second.str(), 12) << "\n";
        if (v.empty()) ui::warn("No departures found for that day.");
        else ui::info("Use 'Search & book' to reserve any of these.");
    }

    string generatePnr() {
        static const string alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
        while (true) {
            string p;
            for (int i = 0; i < 6; ++i) p += alphabet[rng_() % alphabet.size()];
            if (!bookings_.findIf([&](const Booking& b) { return b.pnr() == p; })) return p;
        }
    }

    void bookFlow(const Flight& f, const Date& date, int npax) {
        ui::title("Booking " + f.code() + "  " + f.from().code + " -> " + f.to().code + "  " + date.pretty());
        ui::info(f.category() + " flight | " + f.aircraft() + " | baggage " + to_string(f.baggageKg()) + " kg");

        // 1. passengers
        vector<shared_ptr<Passenger>> pax;
        for (int i = 0; i < npax; ++i) {
            cout << "\n  " << ui::bold("Passenger " + to_string(i + 1) + " of " + to_string(npax)) << "\n";
            string name = util::clean(askNonEmpty("  Full name: "));
            if (name.empty()) { --i; ui::err("Name cannot be empty."); continue; }
            int age = askInt("  Age: ", 0, 110);
            string passport;
            if (f.needsPassport()) {
                while (true) {
                    passport = util::upper(ask("  Passport number (6-9 letters/digits): "));
                    bool okp = passport.size() >= 6 && passport.size() <= 9 &&
                               all_of(passport.begin(), passport.end(), [](char c) { return isalnum((unsigned char)c); });
                    if (okp) break;
                    ui::err("International flights need a valid passport number.");
                }
            }
            auto p = Passenger::create(name, age, passport);
            if (p->discount() > 0) ui::info(p->kind() + " fare: " + to_string((int)(p->discount() * 100)) + "% off");
            pax.push_back(p);
        }

        // 2. seats
        set<string> taken = occupiedSeats(f, date);
        vector<string> chosen;
        ui::title("Choose seats  (type e.g. 7C, or 'auto')");
        for (int i = 0; i < npax; ++i) {
            while (true) {
                showSeatMap(taken, chosen, f, date);
                string s = util::upper(ask("  Seat for " + pax[i]->name() + ": "));
                try {
                    if (s == "AUTO") {
                        string pick;
                        for (int r = 5; r <= ROWS && pick.empty(); ++r)
                            for (char c : string("AFCDBE")) { string l = to_string(r) + c; if (!taken.count(l) && find(chosen.begin(), chosen.end(), l) == chosen.end()) { pick = l; break; } }
                        if (pick.empty()) for (auto& l : allSeats()) if (!taken.count(l) && find(chosen.begin(), chosen.end(), l) == chosen.end()) { pick = l; break; }
                        if (pick.empty()) throw SeatUnavailable("The cabin is full.");
                        s = pick;
                    }
                    int r; char c;
                    if (!parseSeat(s, r, c)) throw InvalidInput("'" + s + "' is not a seat. Rows are 1-" + to_string(ROWS) + ", columns A-F.");
                    s = to_string(r) + c;
                    if (taken.count(s)) throw SeatUnavailable("Seat " + s + " is already taken.");
                    if (find(chosen.begin(), chosen.end(), s) != chosen.end()) throw SeatUnavailable("You already picked " + s + ".");
                    chosen.push_back(s);
                    ui::ok(pax[i]->name() + " -> " + s + " (" + cabinName(cabinOfRow(r)) + ")");
                    break;
                } catch (const BookingError& e) { ui::err(e.what()); }
            }
        }

        // 3. meal & extras
        ui::title("Meal & extras");
        static const vector<string> meals = {"Standard", "Vegetarian", "Vegan", "Jain", "Gluten-free", "Child meal"};
        for (size_t i = 0; i < meals.size(); ++i) cout << "    " << i + 1 << ". " << meals[i] << "\n";
        string meal = meals[askInt("  Meal preference: ", 1, (int)meals.size()) - 1];
        int addons = 0;
        cout << "\n";
        for (auto& a : addOnTable())
            if (askYesNo("  Add " + a.name + " for " + a.price.str() + " per passenger?")) addons |= a.bit;

        // 4. coupon & summary
        QuoteInput q{&f, date, pax, chosen, addons, ""};
        string msg;
        string code = ask("\n  Coupon code (SKY10 / STUDENT / WELCOME, Enter to skip): ");
        q.coupon = code;
        quote(q, &msg);
        if (!code.empty()) {
            Money probe = couponValue(code, Money(100000), msg);
            if (probe.value() > 0) ui::ok(msg); else { ui::warn(msg); q.coupon = ""; }
        }
        Fare fare = quote(q);
        ui::title("Review your booking");
        cout << "    " << f.airlineName() << " " << f.code() << "  " << f.from().city << " -> " << f.to().city << "\n";
        cout << "    " << date.pretty() << "  " << f.depStr() << " - " << f.arrStr() << "  (" << f.durStr() << ", " << f.stopsStr() << ")\n";
        for (size_t i = 0; i < pax.size(); ++i)
            cout << "    " << pax[i]->name() << " (" << pax[i]->kind() << ")  seat " << chosen[i] << "  " << cabinName(cabinOfSeat(chosen[i])) << "\n";
        cout << "    Meal: " << meal << "\n\n" << fare;
        Tier t = tierFor(accounts_[user_].miles);
        if (t.discount > 0) ui::info("Your " + t.name + " status already saved you " + fare.loyalty.str() + ".");
        if (!askYesNo("\n  Proceed to payment?")) { ui::warn("Booking abandoned. No seats were reserved."); return; }

        // 5. payment (runtime polymorphism)
        ui::title("Payment");
        cout << "    1. Card\n    2. UPI\n    3. Net banking\n";
        unique_ptr<Payment> pay;
        switch (askInt("  Choose method: ", 1, 3)) {
            case 1: pay = make_unique<CardPayment>(); break;
            case 2: pay = make_unique<UpiPayment>(); break;
            default: pay = make_unique<NetBankingPayment>(); break;
        }
        if (!pay->collect(fare.total)) { ui::err("Payment was not completed. Your seats were not booked."); return; }

        // 6. final availability re-check (another session could have booked meanwhile)
        set<string> nowTaken = occupiedSeats(f, date);
        for (auto& s : chosen) if (nowTaken.count(s)) { ui::err("Seat " + s + " was just taken. Please book again."); return; }

        Booking& b = bookings_.add(Booking(generatePnr(), accounts_[user_].display, f.code(), date, pax, chosen, meal, addons, pay->label(), fare.total));
        accounts_[user_].miles += fare.miles;
        save();
        ui::ok("Payment received via " + pay->label() + ".  PNR: " + ui::bold(b.pnr()));
        for (size_t i = 0; i < pax.size(); ++i) {
            ostringstream ss; renderTicket(ss, b, i, &f);
            cout << ss.str();
        }
        string file = "ticket_" + b.pnr() + ".txt";
        ofstream out(file);
        for (size_t i = 0; i < pax.size(); ++i) renderTicket(out, b, i, &f);
        ui::ok("E-ticket saved to " + file + "   (+" + to_string(fare.miles) + " miles)");
        Tier nt = tierFor(accounts_[user_].miles);
        if (nt.name != t.name) ui::ok("Congratulations! You reached " + nt.name + " status.");
    }

    void renderTicket(ostream& os, const Booking& b, size_t i, const Flight* f) {
        const int W = 70;
        auto border = [&]() { os << "+" << string(W - 2, '=') << "+\n"; };
        auto row = [&](const string& s) { os << "| " << util::pad(s, W - 4) << " |\n"; };
        const auto& p = *b.passengers()[i];
        unsigned h = util::fnv(b.pnr() + p.name());
        int seatRow = 0; char col; parseSeat(b.seats()[i], seatRow, col);
        Cabin cab = cabinOfRow(seatRow);
        string gate = string(1, (char)('A' + h % 6)) + to_string(1 + (h / 7) % 30);
        string group = cab == Cabin::First ? "1" : cab == Cabin::Business ? "2" : (seatRow >= 10 ? "4" : "3");
        os << "\n";
        border();
        row("SKYWARD AIRWAYS                                BOARDING PASS");
        border();
        if (f) {
            row(f->from().city + " (" + f->from().code + ")   -->   " + f->to().city + " (" + f->to().code + ")");
            row("");
            row("Flight " + f->code() + "  " + f->airlineName() + "  " + f->aircraft());
            row("Date " + b.date().pretty() + "    Departs " + f->depStr() + "    Arrives " + f->arrStr());
            int board = f->depMinutes() - 45;
            row("Boarding " + util::hhmm(board) + "   Gate " + gate + "   Terminal T" + to_string(1 + h % 3) + "   Group " + group);
        } else row("Flight " + b.flightCode() + "  " + b.date().pretty());
        row("");
        row("Passenger : " + p.name() + " (" + p.kind() + ")");
        row("Seat      : " + b.seats()[i] + "  " + cabinName(cab) + (extraLegroom(seatRow) ? " - extra legroom" : ""));
        row("Meal      : " + b.meal());
        if (!p.passport().empty()) row("Passport  : " + p.passport());
        row("PNR       : " + b.pnr() + "     Status: " + (b.confirmed() ? "CONFIRMED" : "CANCELLED"));
        row("");
        for (auto& line : qrArt(b.pnr() + p.name() + b.seats()[i])) row("   " + line);
        string bars;
        for (char c : b.pnr()) bars += string(1 + (c % 3), '|') + ' ';
        row("");
        row("   " + bars + "  SKW" + b.pnr());
        border();
    }

    vector<Booking*> myBookings() {
        return bookings_.where([&](const Booking& b) { return util::lower(b.account()) == user_; });
    }

    void printBookingLine(const Booking& b) {
        const Flight* f = findFlight(b.flightCode());
        string route = f ? f->from().code + "->" + f->to().code : "???";
        string seats; for (size_t i = 0; i < b.seats().size(); ++i) seats += (i ? "," : "") + b.seats()[i];
        string row = util::pad(b.pnr(), 8) + util::pad(b.flightCode(), 9) + util::pad(route, 10) + util::pad(b.date().str(), 12) +
                     util::pad(seats, 14) + util::padLeft(b.total().str(), 14) + "  ";
        cout << "  " << row << (b.confirmed() ? ui::green("Confirmed") : ui::red("Cancelled")) << "\n";
    }

    void showTrips() {
        ui::title("My trips");
        auto mine = myBookings();
        if (mine.empty()) { ui::info("No bookings yet. Choose 'Search & book' from the menu."); return; }
        cout << "  " << ui::bold(util::pad("PNR", 8) + util::pad("Flight", 9) + util::pad("Route", 10) + util::pad("Date", 12) + util::pad("Seats", 14) + util::padLeft("Total", 14) + "  Status") << "\n";
        cout << "  " << string(78, '-') << "\n";
        for (auto* b : mine) printBookingLine(*b);
        string p = util::upper(ask("\n  Enter a PNR to view its tickets (Enter = back): "));
        if (p.empty()) return;
        Booking* b = bookings_.findIf([&](const Booking& x) { return x.pnr() == p && util::lower(x.account()) == user_; });
        if (!b) { ui::err("That PNR is not on your account."); return; }
        const Flight* f = findFlight(b->flightCode());
        for (size_t i = 0; i < b->passengers().size(); ++i) { renderTicket(cout, *b, i, f); }
        if (!b->confirmed()) ui::info("Refunded " + b->refund().str() + " to " + b->payment());
    }

    void pnrStatus() {
        ui::title("PNR status");
        string p = util::upper(askNonEmpty("  Enter PNR: "));
        Booking* b = bookings_.findIf([&](const Booking& x) { return x.pnr() == p; });
        if (!b) throw NotFound("No booking found with PNR " + p + ".");
        const Flight* f = findFlight(b->flightCode());
        cout << "\n    PNR " << ui::bold(b->pnr()) << "  -  " << (b->confirmed() ? ui::green("CONFIRMED") : ui::red("CANCELLED")) << "\n";
        if (f) cout << "    " << f->airlineName() << " " << f->code() << "  " << f->from().city << " -> " << f->to().city
                    << "  " << b->date().pretty() << "  " << f->depStr() << "\n";
        for (size_t i = 0; i < b->passengers().size(); ++i)
            cout << "    " << b->passengers()[i]->name() << "  seat " << b->seats()[i] << "\n";
        cout << "    Paid " << b->total() << " via " << b->payment() << "\n";
        int days = b->date().daysFrom(Date::today());
        if (b->confirmed()) cout << "    " << (days > 0 ? "Departs in " + to_string(days) + " day(s)" : days == 0 ? "Departs today" : "Flight has departed") << "\n";
    }

    // Refund policy: >=7 days 90%, 2-6 days 60%, <2 days 25%.
    static double refundRate(int days) { return days >= 7 ? 0.90 : days >= 2 ? 0.60 : 0.25; }

    void cancelFlow() {
        ui::title("Cancel a booking");
        auto live = bookings_.where([&](const Booking& b) { return b.confirmed() && util::lower(b.account()) == user_ && !(b.date() < Date::today()); });
        if (live.empty()) { ui::info("You have no upcoming confirmed bookings."); return; }
        for (auto* b : live) printBookingLine(*b);
        string p = util::upper(askNonEmpty("\n  PNR to cancel: "));
        Booking* b = bookings_.findIf([&](const Booking& x) { return x.pnr() == p && x.confirmed() && util::lower(x.account()) == user_; });
        if (!b) throw NotFound("No upcoming confirmed booking with that PNR on your account.");
        int days = b->date().daysFrom(Date::today());
        double rate = refundRate(days);
        Money refund = b->total() * rate;
        cout << "\n    Departure in " << days << " day(s) -> refund " << int(rate * 100) << "% = " << refund << "\n";
        ui::info("Policy: 7+ days 90%, 2-6 days 60%, under 2 days 25%.");
        if (!askYesNo("  Cancel this booking?")) { ui::info("Nothing changed."); return; }
        int earned = (int)(b->total().value() / 10);
        accounts_[user_].miles = max(0, accounts_[user_].miles - earned);
        b->cancel(refund);
        save();
        ui::ok("Booking " + b->pnr() + " cancelled. " + refund.str() + " will return to " + b->payment() + ". Seats released.");
    }

    void profile() {
        ui::title("Skyward Miles");
        Account& a = accounts_[user_];
        Tier t = tierFor(a.miles);
        cout << "    Member  : " << a.display << "\n    Tier    : " << ui::bold(t.name) << "  (" << int(t.discount * 100) << "% off every fare)\n";
        cout << "    Miles   : " << a.miles << "\n";
        if (t.nextAt) {
            int pct = min(20, a.miles * 20 / t.nextAt);
            cout << "    Progress: [" << string(pct, '#') << string(20 - pct, '.') << "] " << t.nextAt - a.miles << " miles to next tier\n";
        } else cout << "    You are at the highest tier. Thank you!\n";
        auto mine = myBookings();
        Money spent; int trips = 0;
        for (auto* b : mine) if (b->confirmed()) { spent += b->total(); ++trips; }
        cout << "    Trips   : " << trips << "   Total spent: " << spent << "\n";
        ui::info("Tiers: Blue 0 | Silver 5,000 | Gold 15,000 | Platinum 40,000 miles.");
    }

    void settings() {
        ui::title("Settings");
        cout << "    1. Toggle colours (currently " << (ui::color ? "on" : "off") << ")\n    2. Switch account\n    3. Back\n";
        int c = askInt("  > ", 1, 3);
        if (c == 1) { ui::color = !ui::color; ui::ok(string("Colours ") + (ui::color ? "on" : "off")); }
        else if (c == 2) login();
    }

    void adminPanel() {
        ui::title("Admin panel");
        bool ok = false;
        for (int i = 0; i < 3 && !ok; ++i) ok = (ask("  Admin password: ") == ADMIN_PASSWORD);
        if (!ok) { ui::err("Access denied."); return; }
        while (true) {
            cout << "\n    1. Revenue & statistics\n    2. All bookings\n    3. Add a flight\n    4. Back\n";
            int c = askInt("  admin> ", 1, 4);
            if (c == 4) return;
            if (c == 1) {
                Money revenue, refunds; int conf = 0, canc = 0;
                for (auto& b : bookings_.all()) {
                    if (b.confirmed()) { revenue += b.total(); ++conf; } else { revenue += b.total() - b.refund(); refunds += b.refund(); ++canc; }
                }
                cout << "\n    Bookings: " << conf << " confirmed, " << canc << " cancelled\n    Net revenue: " << revenue << "   Refunded: " << refunds << "\n";
                auto routes = bookings_.countBy([&](const Booking& b) {
                    const Flight* f = findFlight(b.flightCode());
                    return f ? f->from().code + "->" + f->to().code : string("???");
                });
                vector<pair<string, int>> v(routes.begin(), routes.end());
                std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
                cout << "    Top routes:";
                for (size_t i = 0; i < v.size() && i < 5; ++i) cout << "  " << v[i].first << " (" << v[i].second << ")";
                cout << "\n    Flights in network: " << flights_.size() << "   Registered users: " << accounts_.size() << "\n";
            } else if (c == 2) {
                if (bookings_.size() == 0) ui::info("No bookings yet.");
                for (auto& b : bookings_.all()) { cout << "  " << util::pad(b.account(), 12); printBookingLine(b); }
            } else {
                try {
                    string code = util::upper(askNonEmpty("  New flight code (e.g. 6E-999): "));
                    if (findFlight(code)) throw InvalidInput("That flight code already exists.");
                    string al = util::upper(askNonEmpty("  Airline code (6E/AI/QP/IX/SG/EK/SQ/QR): "));
                    bool known = any_of(airlineTable().begin(), airlineTable().end(), [&](const AirlineInfo& a) { return a.code == al; });
                    if (!known) throw InvalidInput("Unknown airline code.");
                    const Airport* from = askAirport("  From: ");
                    const Airport* to = askAirport("  To: ", from);
                    string t = askNonEmpty("  Departure time (HH:MM): ");
                    if (t.size() != 5 || t[2] != ':') throw InvalidInput("Use HH:MM.");
                    int hh = stoi(t.substr(0, 2)), mm = stoi(t.substr(3, 2));
                    if (hh > 23 || mm > 59) throw InvalidInput("Time out of range.");
                    int price = askInt("  Base economy price (Rs.): ", 500, 500000);
                    double km = distanceKm(*from, *to);
                    bool dom = from->domestic() && to->domestic();
                    int dur = (int)(km / (dom ? 780.0 : 830.0) * 60) + 35;
                    string ac = km > 1800 ? "Boeing 787-9" : "Airbus A320";
                    if (!addFlight(code, al, from->code, to->code, hh * 60 + mm, dur, Money(price), ac, 0)) throw InvalidInput("Could not add flight.");
                    customLines_.push_back(code + "|" + al + "|" + from->code + "|" + to->code + "|" + to_string(hh * 60 + mm) + "|" + to_string(dur) + "|" + to_string(price) + "|" + ac + "|0");
                    save();
                    ui::ok("Flight " + code + " added (" + string(dom ? "domestic" : "international") + ") and will run daily.");
                } catch (const InvalidInput& e) { ui::err(e.what()); }
                catch (const invalid_argument&) { ui::err("Invalid number."); }
            }
        }
    }

    void banner() {
        cout << ui::cyan(R"(
        __|__
 --o--o--(_)--o--o--        S K Y W A R D   A I R W A Y S
)") << ui::bold("          Fly better. Book smarter.") << "\n";
    }

public:
    BookingSystem() { buildAirports(); buildNetwork(); load(); }

    void run() {
        banner();
        login();
        while (true) {
            Account& a = accounts_[user_];
            Tier t = tierFor(a.miles);
            cout << "\n" << string(72, '=') << "\n  " << ui::bold("MAIN MENU") << "   " << a.display << " | " << t.name << " | " << a.miles << " miles\n" << string(72, '=') << "\n";
            cout << "   1. Search & book flights\n   2. Deal finder (cheapest destinations)\n   3. My trips & tickets\n"
                    "   4. PNR status\n   5. Cancel a booking\n   6. Skyward Miles profile\n   7. Settings\n   8. Admin\n   0. Exit\n";
            int c = askInt("  Choose: ", 0, 8);
            try {
                switch (c) {
                    case 1: searchAndBook(); break;
                    case 2: dealFinder(); break;
                    case 3: showTrips(); break;
                    case 4: pnrStatus(); break;
                    case 5: cancelFlow(); break;
                    case 6: profile(); break;
                    case 7: settings(); break;
                    case 8: adminPanel(); break;
                    case 0: save(); cout << "\n  Thank you for flying Skyward. Safe travels!\n\n"; return;
                }
            } catch (const BookingError& e) { ui::err(e.what()); }
        }
    }
};

// ----------------------------------------------------------------------------
int main() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) SetConsoleMode(h, mode | 0x0004);  // enable ANSI colours
#endif
    try {
        BookingSystem app;
        app.run();
    } catch (const InputClosed&) {
        cout << "\n  Input closed. Goodbye!\n";
    } catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
