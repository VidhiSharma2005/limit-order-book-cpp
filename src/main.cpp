#include "OrderBook.h"

#include <iostream>

int main() {

    OrderBook book;

    std::cout << "====================================\n";
    std::cout << "       MINI EXCHANGE SIMULATOR      \n";
    std::cout << "====================================\n";

    // Add SELL orders.
    std::cout << "\n--- Adding SELL orders ---\n";

    book.addOrder(OrderSide::SELL, 10500, 10);
    book.addOrder(OrderSide::SELL, 10600, 20);
    book.addOrder(OrderSide::SELL, 10700, 15);

    // Add BUY orders.
    std::cout << "\n--- Adding BUY orders ---\n";

    book.addOrder(OrderSide::BUY, 10300, 10);
    book.addOrder(OrderSide::BUY, 10400, 20);

    // Display the order book before matching.
    std::cout << "\n--- Order Book Before Trade ---\n";
    book.displayOrderBook();

    // This order should match with existing SELL orders.
    std::cout << "\n--- Submitting Incoming BUY order ---\n";

    book.addOrder(OrderSide::BUY, 10600, 15);

    // Display the updated order book.
    std::cout << "\n--- Order Book After Trade ---\n";
    book.displayOrderBook();

    // Display executed trades.
    book.displayTrades();

    return 0;
}
