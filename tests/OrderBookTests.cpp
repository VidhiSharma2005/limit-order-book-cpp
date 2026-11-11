#include <gtest/gtest.h>
#include "OrderBook.h"
#include "ReplayEngine.h"
#include "Event.h"
#include <random>
#include <algorithm>

// Test 1: A non-crossing BUY order should remain in the book.
TEST(OrderBookTests, NonCrossingBuyOrderRestsInBook) {

    OrderBook book;

    book.addOrder(OrderSide::SELL, 10500, 10);
    book.addOrder(OrderSide::BUY, 10300, 5);

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::BUY, 10300), 5);
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 10);
    EXPECT_TRUE(book.getTrades().empty());
}


// Test 2: A crossing BUY order should completely fill a SELL order.
TEST(OrderBookTests, CompleteFill) {

    OrderBook book;

    book.addOrder(OrderSide::SELL, 10500, 10);
    book.addOrder(OrderSide::BUY, 10500, 10);

    ASSERT_EQ(book.getTrades().size(), 1);

    EXPECT_EQ(book.getTrades()[0].buyOrderId, 2);
    EXPECT_EQ(book.getTrades()[0].sellOrderId, 1);
    EXPECT_EQ(book.getTrades()[0].executionPrice, 10500);
    EXPECT_EQ(book.getTrades()[0].quantity, 10);

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 0);
}


// Test 3: A smaller incoming order should partially fill a resting order.
TEST(OrderBookTests, PartialFill) {

    OrderBook book;

    book.addOrder(OrderSide::SELL, 10500, 20);
    book.addOrder(OrderSide::BUY, 10500, 8);

    ASSERT_EQ(book.getTrades().size(), 1);

    EXPECT_EQ(book.getTrades()[0].quantity, 8);
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 12);
}


// Test 4: Incoming BUY should match the lowest ask first.
TEST(OrderBookTests, BestAskGetsPriority) {

    OrderBook book;

    book.addOrder(OrderSide::SELL, 10600, 10);
    book.addOrder(OrderSide::SELL, 10500, 10);

    book.addOrder(OrderSide::BUY, 10600, 5);

    ASSERT_EQ(book.getTrades().size(), 1);

    EXPECT_EQ(book.getTrades()[0].executionPrice, 10500);
    EXPECT_EQ(book.getTrades()[0].sellOrderId, 2);
}


// Test 5: An invalid order should be rejected.
TEST(OrderBookTests, RejectsInvalidOrders) {

    OrderBook book;

    EXPECT_EQ(book.addOrder(OrderSide::BUY, 0, 10), -1);
    EXPECT_EQ(book.addOrder(OrderSide::SELL, 10500, 0), -1);

    EXPECT_TRUE(book.getTrades().empty());
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::BUY, 10500), 0);
}


// Test 6: Older orders at the same price must execute first.
TEST(OrderBookTests, TimePriorityAtSamePrice) {

    OrderBook book;

    // Seller A arrives first.
    int sellerA = book.addOrder(OrderSide::SELL, 10500, 10);

    // Seller B arrives second at the same price.
    int sellerB = book.addOrder(OrderSide::SELL, 10500, 20);

    // Buyer wants 15 units.
    book.addOrder(OrderSide::BUY, 10500, 15);

    ASSERT_EQ(book.getTrades().size(), 2);

    // Older seller must execute first.
    EXPECT_EQ(book.getTrades()[0].sellOrderId, sellerA);
    EXPECT_EQ(book.getTrades()[0].quantity, 10);

    // Remaining 5 units must come from seller B.
    EXPECT_EQ(book.getTrades()[1].sellOrderId, sellerB);
    EXPECT_EQ(book.getTrades()[1].quantity, 5);

    // Seller B should have 15 units remaining.
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 15);
}


// Test 7: Incoming SELL should match against the highest bid first.
TEST(OrderBookTests, BestBidGetsPriority) {
    OrderBook book;

    int buyerA = book.addOrder(OrderSide::BUY, 10400, 10);
    int buyerB = book.addOrder(OrderSide::BUY, 10500, 10);

    book.addOrder(OrderSide::SELL, 10400, 5);

    ASSERT_EQ(book.getTrades().size(), 1);

    EXPECT_EQ(book.getTrades()[0].buyOrderId, buyerB);
    EXPECT_EQ(book.getTrades()[0].executionPrice, 10500);
    EXPECT_EQ(book.getTrades()[0].quantity, 5);

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::BUY, 10500), 5);
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::BUY, 10400), 10);
}


 // Test 8: Unfilled incoming quantity should rest in the order book.
TEST(OrderBookTests, IncomingOrderRestsWithRemainingQuantity) {
    OrderBook book;

    book.addOrder(OrderSide::SELL, 10500, 10);

    int buyerId = book.addOrder(OrderSide::BUY, 10500, 15);

    ASSERT_EQ(book.getTrades().size(), 1);

    EXPECT_EQ(book.getTrades()[0].buyOrderId, buyerId);
    EXPECT_EQ(book.getTrades()[0].quantity, 10);

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 0);
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::BUY, 10500), 5);
}


TEST(OrderBookTests, RestingOrdersAreRegisteredInLookup) {
    OrderBook book;

    int sellId = book.addOrder(OrderSide::SELL, 10500, 10);
    int buyId = book.addOrder(OrderSide::BUY, 10300, 5);

    EXPECT_TRUE(book.hasOrder(sellId));
    EXPECT_TRUE(book.hasOrder(buyId));

    EXPECT_FALSE(book.hasOrder(999));
}

TEST(OrderBookTests, CancelMiddleOrderPreservesTimePriority) {
    OrderBook book;

    int sellerA = book.addOrder(OrderSide::SELL, 10500, 10);
    int sellerB = book.addOrder(OrderSide::SELL, 10500, 20);
    int sellerC = book.addOrder(OrderSide::SELL, 10500, 30);

    EXPECT_TRUE(book.cancelOrder(sellerB));

    EXPECT_FALSE(book.hasOrder(sellerB));
    EXPECT_TRUE(book.hasOrder(sellerA));
    EXPECT_TRUE(book.hasOrder(sellerC));

    // The remaining orders must still execute in FIFO order.
    book.addOrder(OrderSide::BUY, 10500, 15);

    ASSERT_EQ(book.getTrades().size(), 2);

    EXPECT_EQ(book.getTrades()[0].sellOrderId, sellerA);
    EXPECT_EQ(book.getTrades()[0].quantity, 10);

    EXPECT_EQ(book.getTrades()[1].sellOrderId, sellerC);
    EXPECT_EQ(book.getTrades()[1].quantity, 5);

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 25);
}

TEST(OrderBookTests, CancelNonexistentOrder) {
    OrderBook book;

    EXPECT_FALSE(book.cancelOrder(999));
    EXPECT_FALSE(book.hasOrder(999));
}

TEST(OrderBookTests, CancelPartiallyFilledOrder) {
    OrderBook book;

    int sellerId = book.addOrder(OrderSide::SELL, 10500, 20);

    book.addOrder(OrderSide::BUY, 10500, 8);

    EXPECT_TRUE(book.hasOrder(sellerId));
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 12);

    EXPECT_TRUE(book.cancelOrder(sellerId));

    EXPECT_FALSE(book.hasOrder(sellerId));
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 0);
    EXPECT_EQ(book.getTrades().size(), 1);
}

TEST(OrderBookTests, CannotCancelFullyExecutedOrder) {
    OrderBook book;

    int sellerId = book.addOrder(OrderSide::SELL, 10500, 10);

    book.addOrder(OrderSide::BUY, 10500, 10);

    EXPECT_FALSE(book.hasOrder(sellerId));
    EXPECT_FALSE(book.cancelOrder(sellerId));

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 0);
    EXPECT_EQ(book.getTrades().size(), 1);
}

TEST(OrderBookTests, CancelLastOrderAtPriceLevel) {
    OrderBook book;

    int buyerId = book.addOrder(OrderSide::BUY, 10400, 10);

    EXPECT_TRUE(book.cancelOrder(buyerId));

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::BUY, 10400), 0);
    EXPECT_FALSE(book.hasOrder(buyerId));

    // Add another order at the same price to verify the level
    // can be recreated correctly.
    int newBuyerId = book.addOrder(OrderSide::BUY, 10400, 15);

    EXPECT_TRUE(book.hasOrder(newBuyerId));
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::BUY, 10400), 15);
}


TEST(OrderBookTests, QuantityReductionPreservesOrderPriority) {
    OrderBook book;

    int sellerA = book.addOrder(OrderSide::SELL, 10500, 20);
    int sellerB = book.addOrder(OrderSide::SELL, 10500, 30);

    int modifiedId = book.modifyOrder(sellerA, 10500, 10);

    EXPECT_EQ(modifiedId, sellerA);
    EXPECT_TRUE(book.hasOrder(sellerA));

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 40);

    // A should still execute before B.
    book.addOrder(OrderSide::BUY, 10500, 15);

    ASSERT_EQ(book.getTrades().size(), 2);

    EXPECT_EQ(book.getTrades()[0].sellOrderId, sellerA);
    EXPECT_EQ(book.getTrades()[0].quantity, 10);

    EXPECT_EQ(book.getTrades()[1].sellOrderId, sellerB);
    EXPECT_EQ(book.getTrades()[1].quantity, 5);
}

TEST(OrderBookTests, PriceChangeCreatesNewOrder) {
    OrderBook book;

    int sellerId = book.addOrder(OrderSide::SELL, 10500, 20);

    int modifiedId = book.modifyOrder(sellerId, 10600, 20);

    EXPECT_NE(modifiedId, sellerId);

    EXPECT_FALSE(book.hasOrder(sellerId));
    EXPECT_TRUE(book.hasOrder(modifiedId));

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 0);
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10600), 20);
}

TEST(OrderBookTests, QuantityIncreaseCreatesNewOrder) {
    OrderBook book;

    int sellerA = book.addOrder(OrderSide::SELL, 10500, 20);
    int sellerB = book.addOrder(OrderSide::SELL, 10500, 30);

    int modifiedId = book.modifyOrder(sellerA, 10500, 40);

    EXPECT_NE(modifiedId, sellerA);

    EXPECT_FALSE(book.hasOrder(sellerA));
    EXPECT_TRUE(book.hasOrder(modifiedId));

    // B retains priority over the replacement order.
    book.addOrder(OrderSide::BUY, 10500, 35);

    ASSERT_EQ(book.getTrades().size(), 2);

    EXPECT_EQ(book.getTrades()[0].sellOrderId, sellerB);
    EXPECT_EQ(book.getTrades()[0].quantity, 30);

    EXPECT_EQ(book.getTrades()[1].sellOrderId, modifiedId);
    EXPECT_EQ(book.getTrades()[1].quantity, 5);
}

TEST(OrderBookTests, CannotModifyNonexistentOrder) {
    OrderBook book;

    int result = book.modifyOrder(999, 10500, 20);

    EXPECT_EQ(result, -1);
}

TEST(OrderBookTests, InvalidPriceDoesNotModifyOrder) {
    OrderBook book;

    int sellerId = book.addOrder(OrderSide::SELL, 10500, 20);

    int result = book.modifyOrder(sellerId, 0, 30);

    EXPECT_EQ(result, -1);
    EXPECT_TRUE(book.hasOrder(sellerId));
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 20);
}

TEST(OrderBookTests, InvalidQuantityDoesNotModifyOrder) {
    OrderBook book;

    int sellerId = book.addOrder(OrderSide::SELL, 10500, 20);

    int result = book.modifyOrder(sellerId, 10500, 0);

    EXPECT_EQ(result, -1);
    EXPECT_TRUE(book.hasOrder(sellerId));
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 20);
}

TEST(OrderBookTests, PriceModificationTriggersImmediateExecution) {
    OrderBook book;

    int sellerId = book.addOrder(OrderSide::SELL, 10500, 20);
    int buyerId = book.addOrder(OrderSide::BUY, 10400, 10);

    // Increase the buy price so that it crosses the best ask.
    int modifiedId = book.modifyOrder(buyerId, 10500, 10);

    EXPECT_NE(modifiedId, buyerId);

    // The original buy order has been replaced.
    EXPECT_FALSE(book.hasOrder(buyerId));

    // The replacement was completely executed, so it is no longer resting.
    EXPECT_FALSE(book.hasOrder(modifiedId));

    // The seller has 10 units remaining.
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 10);

    // The old buy price level should be empty.
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::BUY, 10400), 0);

    ASSERT_EQ(book.getTrades().size(), 1);

    const Trade& trade = book.getTrades()[0];

    EXPECT_EQ(trade.buyOrderId, modifiedId);
    EXPECT_EQ(trade.sellOrderId, sellerId);
    EXPECT_EQ(trade.executionPrice, 10500);
    EXPECT_EQ(trade.quantity, 10);
}

TEST(OrderBookTests, EmptyOrderBookIsValid) {
    OrderBook book;

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, InvariantsHoldAcrossOrderLifecycle) {
    OrderBook book;

    // Add two sell orders at the same price.
    int sellerA = book.addOrder(OrderSide::SELL, 10500, 20);
    int sellerB = book.addOrder(OrderSide::SELL, 10500, 30);

    EXPECT_TRUE(book.validate());

    // Partially fill the first sell order.
    book.addOrder(OrderSide::BUY, 10500, 10);

    EXPECT_TRUE(book.validate());

    // Cancel the second sell order.
    EXPECT_TRUE(book.cancelOrder(sellerB));

    EXPECT_TRUE(book.validate());

    // Reduce the remaining quantity of the first seller.
    int modifiedId = book.modifyOrder(sellerA, 10500, 5);

    EXPECT_EQ(modifiedId, sellerA);
    EXPECT_TRUE(book.validate());

    // Fully execute the remaining sell order.
    book.addOrder(OrderSide::BUY, 10500, 5);

    EXPECT_TRUE(book.validate());

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 0);
}

TEST(OrderBookTests, NoOpModificationPreservesPriority) {
    OrderBook book;

    int sellerA = book.addOrder(OrderSide::SELL, 10500, 20);
    int sellerB = book.addOrder(OrderSide::SELL, 10500, 30);

    int modifiedId = book.modifyOrder(sellerA, 10500, 20);

    EXPECT_EQ(modifiedId, sellerA);
    EXPECT_TRUE(book.hasOrder(sellerA));

    // Both orders should remain in the book.
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 50);

    // Seller A must still have priority.
    book.addOrder(OrderSide::BUY, 10500, 25);

    ASSERT_EQ(book.getTrades().size(), 2);

    EXPECT_EQ(book.getTrades()[0].sellOrderId, sellerA);
    EXPECT_EQ(book.getTrades()[0].quantity, 20);

    EXPECT_EQ(book.getTrades()[1].sellOrderId, sellerB);
    EXPECT_EQ(book.getTrades()[1].quantity, 5);

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ModifyOrderAfterPartialFill) {
    OrderBook book;

    int sellerA = book.addOrder(OrderSide::SELL, 10500, 20);
    int sellerB = book.addOrder(OrderSide::SELL, 10500, 30);

    // Partially fill seller A: 8 of 20 units.
    book.addOrder(OrderSide::BUY, 10500, 8);

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 42);

    // Seller A now has 12 units remaining.
    // Reduce that remaining quantity to 10.
    int modifiedId = book.modifyOrder(sellerA, 10500, 10);

    EXPECT_EQ(modifiedId, sellerA);
    EXPECT_TRUE(book.hasOrder(sellerA));

    // 10 units from A + 30 units from B.
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 40);

    // Check that seller A still has FIFO priority.
    book.addOrder(OrderSide::BUY, 10500, 15);

    ASSERT_EQ(book.getTrades().size(), 3);

    EXPECT_EQ(book.getTrades()[1].sellOrderId, sellerA);
    EXPECT_EQ(book.getTrades()[1].quantity, 10);

    EXPECT_EQ(book.getTrades()[2].sellOrderId, sellerB);
    EXPECT_EQ(book.getTrades()[2].quantity, 5);

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, CannotModifyCancelledOrFullyExecutedOrder) {
    OrderBook book;

    // Scenario 1: Cancelled order.
    int cancelledSeller = book.addOrder(OrderSide::SELL, 10500, 20);

    EXPECT_TRUE(book.cancelOrder(cancelledSeller));

    int cancelledResult =
        book.modifyOrder(cancelledSeller, 10600, 30);

    EXPECT_EQ(cancelledResult, -1);
    EXPECT_FALSE(book.hasOrder(cancelledSeller));

    EXPECT_TRUE(book.validate());

    // Scenario 2: Fully executed order.
    int executedSeller = book.addOrder(OrderSide::SELL, 10500, 10);

    book.addOrder(OrderSide::BUY, 10500, 10);

    EXPECT_FALSE(book.hasOrder(executedSeller));

    int executedResult =
        book.modifyOrder(executedSeller, 10600, 20);

    EXPECT_EQ(executedResult, -1);
    EXPECT_FALSE(book.hasOrder(executedSeller));

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ModifiedOrderPartiallyExecutesAndRests) {
    OrderBook book;

    int sellerId = book.addOrder(OrderSide::SELL, 10500, 10);
    int buyerId = book.addOrder(OrderSide::BUY, 10400, 20);

    // Modify the buy order to a price that crosses the best ask.
    int modifiedId = book.modifyOrder(buyerId, 10500, 20);

    // Price change must create a new order ID.
    EXPECT_NE(modifiedId, buyerId);

    // Original order must no longer exist.
    EXPECT_FALSE(book.hasOrder(buyerId));

    // Replacement order should remain active with 10 units.
    EXPECT_TRUE(book.hasOrder(modifiedId));

    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::BUY, 10500), 10);

    // Seller should be fully executed.
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 0);
    EXPECT_FALSE(book.hasOrder(sellerId));

    ASSERT_EQ(book.getTrades().size(), 1);

    const Trade& trade = book.getTrades()[0];

    EXPECT_EQ(trade.buyOrderId, modifiedId);
    EXPECT_EQ(trade.sellOrderId, sellerId);
    EXPECT_EQ(trade.executionPrice, 10500);
    EXPECT_EQ(trade.quantity, 10);

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplaysValidEventSequence) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        // Sequence 1: Create a sell order: ID 1.
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},

        // Sequence 2: Create a non-crossing buy order: ID 2.
        {2, EventType::NEW_ORDER, 2, OrderSide::BUY, 10400, 10, -1},

        // Sequence 3: Modify buy order 2.
        // Price change creates replacement order ID 3.
        {3, EventType::MODIFY_ORDER, 2, OrderSide::BUY, 10500, 10, 3}
    };

    ReplayResult result = engine.replay(events, book);

    // Replay should complete successfully.
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.processedEvents, 3);
    EXPECT_TRUE(result.errorMessage.empty());

    // Original buy order was replaced.
    EXPECT_FALSE(book.hasOrder(2));

    // Replacement was completely executed.
    EXPECT_FALSE(book.hasOrder(3));

    // Seller has 10 units remaining.
    EXPECT_TRUE(book.hasOrder(1));
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::SELL, 10500), 10);

    // No resting buy orders.
    EXPECT_EQ(book.getQuantityAtPrice(OrderSide::BUY, 10500), 0);

    // Verify execution details.
    ASSERT_EQ(book.getTrades().size(), 1);

    const Trade& trade = book.getTrades()[0];

    EXPECT_EQ(trade.buyOrderId, 3);
    EXPECT_EQ(trade.sellOrderId, 1);
    EXPECT_EQ(trade.executionPrice, 10500);
    EXPECT_EQ(trade.quantity, 10);

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayingSameEventsProducesIdenticalResults) {
    OrderBook bookA;
    OrderBook bookB;

    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        // Event 1: Create sell order ID 1.
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},

        // Event 2: Create buy order ID 2.
        {2, EventType::NEW_ORDER, 2, OrderSide::BUY, 10400, 10, -1},

        // Event 3: Modify order 2.
        // Price change creates replacement order ID 3.
        {3, EventType::MODIFY_ORDER, 2, OrderSide::BUY, 10500, 10, 3}
    };

    // Replay the exact same events into two independent books.
    ReplayResult resultA = engine.replay(events, bookA);
    ReplayResult resultB = engine.replay(events, bookB);

    // 1. Compare replay results.
    EXPECT_TRUE(resultA.success);
    EXPECT_TRUE(resultB.success);

    EXPECT_EQ(resultA.processedEvents, resultB.processedEvents);
    EXPECT_EQ(resultA.processedEvents, 3);

    EXPECT_TRUE(resultA.errorMessage.empty());
    EXPECT_TRUE(resultB.errorMessage.empty());

    // 2. Compare active orders.
    EXPECT_EQ(bookA.hasOrder(1), bookB.hasOrder(1));
    EXPECT_EQ(bookA.hasOrder(2), bookB.hasOrder(2));
    EXPECT_EQ(bookA.hasOrder(3), bookB.hasOrder(3));

    EXPECT_TRUE(bookA.hasOrder(1));
    EXPECT_FALSE(bookA.hasOrder(2));
    EXPECT_FALSE(bookA.hasOrder(3));

    // 3. Compare remaining quantities.
    EXPECT_EQ(
        bookA.getQuantityAtPrice(OrderSide::SELL, 10500),
        bookB.getQuantityAtPrice(OrderSide::SELL, 10500)
    );

    EXPECT_EQ(
        bookA.getQuantityAtPrice(OrderSide::BUY, 10400),
        bookB.getQuantityAtPrice(OrderSide::BUY, 10400)
    );

    EXPECT_EQ(
        bookA.getQuantityAtPrice(OrderSide::BUY, 10500),
        bookB.getQuantityAtPrice(OrderSide::BUY, 10500)
    );

    EXPECT_EQ(
        bookA.getQuantityAtPrice(OrderSide::SELL, 10500),
        10
    );

    // 4. Compare generated trades.
    const auto& tradesA = bookA.getTrades();
    const auto& tradesB = bookB.getTrades();

    ASSERT_EQ(tradesA.size(), tradesB.size());
    ASSERT_EQ(tradesA.size(), 1);

    for (std::size_t i = 0; i < tradesA.size(); ++i) {
        EXPECT_EQ(tradesA[i].buyOrderId, tradesB[i].buyOrderId);
        EXPECT_EQ(tradesA[i].sellOrderId, tradesB[i].sellOrderId);
        EXPECT_EQ(tradesA[i].executionPrice, tradesB[i].executionPrice);
        EXPECT_EQ(tradesA[i].quantity, tradesB[i].quantity);
    }

    // Verify the expected trade as well.
    EXPECT_EQ(tradesA[0].buyOrderId, 3);
    EXPECT_EQ(tradesA[0].sellOrderId, 1);
    EXPECT_EQ(tradesA[0].executionPrice, 10500);
    EXPECT_EQ(tradesA[0].quantity, 10);

    // 5. Both books must satisfy their internal invariants.
    EXPECT_TRUE(bookA.validate());
    EXPECT_TRUE(bookB.validate());
}

TEST(OrderBookTests, ReplayStopsWhenCancellationFails) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        // Event 1: Create a valid sell order with ID 1.
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},

        // Event 2: Try cancelling an order that doesn't exist.
        {2, EventType::CANCEL_ORDER, 999, OrderSide::BUY, 0, 0, -1}
    };

    ReplayResult result = engine.replay(events, book);

    // Replay must report failure.
    EXPECT_FALSE(result.success);

    // Only the first event was successfully processed.
    EXPECT_EQ(result.processedEvents, 1);

    // The error should identify the failed event.
    EXPECT_EQ(
        result.errorMessage,
        "Event 2: cancellation failed"
    );

    // The first event's order should still exist.
    EXPECT_TRUE(book.hasOrder(1));

    EXPECT_EQ(
        book.getQuantityAtPrice(OrderSide::SELL, 10500),
        20
    );

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsInvalidModifications) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        // Event 1: Create sell order ID 1.
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},

        // Event 2: Modify a nonexistent order.
        {2, EventType::MODIFY_ORDER, 999, OrderSide::BUY, 10500, 10, -1}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 1);

    EXPECT_EQ(
        result.errorMessage,
        "Event 2: modification failed"
    );

    EXPECT_TRUE(book.hasOrder(1));
    EXPECT_EQ(
        book.getQuantityAtPrice(OrderSide::SELL, 10500),
        20
    );

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsDuplicateSequenceNumbers) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},
        {1, EventType::NEW_ORDER, 2, OrderSide::BUY, 10400, 10, -1}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 1);

    EXPECT_EQ(
        result.errorMessage,
        "Event 1: unexpected sequence number (expected 2)"
    );

    EXPECT_TRUE(book.hasOrder(1));
    EXPECT_FALSE(book.hasOrder(2));
    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsMissingSequenceNumbers) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},
        {3, EventType::NEW_ORDER, 2, OrderSide::BUY, 10400, 10, -1}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 1);

    EXPECT_EQ(
        result.errorMessage,
        "Event 3: unexpected sequence number (expected 2)"
    );

    EXPECT_TRUE(book.hasOrder(1));
    EXPECT_FALSE(book.hasOrder(2));
    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsOutOfOrderSequenceNumbers) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},
        {2, EventType::NEW_ORDER, 2, OrderSide::BUY, 10400, 10, -1},
        {1, EventType::CANCEL_ORDER, 1, OrderSide::BUY, 0, 0, -1}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 2);

    EXPECT_EQ(
        result.errorMessage,
        "Event 1: unexpected sequence number (expected 3)"
    );

    // The invalid third event must not cancel order 1.
    EXPECT_TRUE(book.hasOrder(1));
    EXPECT_TRUE(book.hasOrder(2));
    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsInvalidNewOrder) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        // Event 1: Valid order.
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},

        // Event 2: Invalid price.
        {2, EventType::NEW_ORDER, 2, OrderSide::BUY, 0, 10, -1}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 1);

    EXPECT_EQ(
        result.errorMessage,
        "Event 2: new order failed"
    );

    EXPECT_TRUE(book.hasOrder(1));
    EXPECT_FALSE(book.hasOrder(2));

    EXPECT_EQ(
        book.getQuantityAtPrice(OrderSide::SELL, 10500),
        20
    );

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsNegativeOrderQuantity) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},
        {2, EventType::NEW_ORDER, 2, OrderSide::BUY, 10400, -5, -1}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 1);
    EXPECT_EQ(result.errorMessage, "Event 2: new order failed");

    EXPECT_TRUE(book.hasOrder(1));
    EXPECT_FALSE(book.hasOrder(2));
    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsUnknownEventType) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},
        {2, static_cast<EventType>(999), 2, OrderSide::BUY, 10400, 10, -1}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 1);
    EXPECT_EQ(result.errorMessage, "Event 2: unknown event type");

    EXPECT_TRUE(book.hasOrder(1));
    EXPECT_FALSE(book.hasOrder(2));
    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsInvalidOrderSide) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        {1, EventType::NEW_ORDER, 1,
         static_cast<OrderSide>(999), 10500, 20, -1}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 0);
    EXPECT_EQ(result.errorMessage, "Event 1: new order failed");

    EXPECT_FALSE(book.hasOrder(1));
    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsInvalidModificationFields) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},

        // Invalid modification price.
        {2, EventType::MODIFY_ORDER, 1, OrderSide::SELL, 0, 10, -1}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 1);
    EXPECT_EQ(result.errorMessage, "Event 2: modification failed");

    // Original order must remain unchanged.
    EXPECT_TRUE(book.hasOrder(1));
    EXPECT_EQ(
        book.getQuantityAtPrice(OrderSide::SELL, 10500),
        20
    );

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsUnexpectedNewOrderIdBeforeApplying) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        // The book expects ID 1, but the event expects ID 99.
        {1, EventType::NEW_ORDER, 99, OrderSide::BUY, 10400, 10, -1}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 0);

    EXPECT_EQ(
        result.errorMessage,
        "Event 1: generated order ID mismatch"
    );

    // The mismatched event must not modify the book.
    EXPECT_FALSE(book.hasOrder(1));
    EXPECT_FALSE(book.hasOrder(99));
    EXPECT_EQ(book.peekNextOrderId(), 1);

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, ReplayRejectsUnexpectedModifiedOrderIdBeforeApplying) {
    OrderBook book;
    ReplayEngine engine;

    std::vector<OrderEvent> events = {
        {1, EventType::NEW_ORDER, 1, OrderSide::SELL, 10500, 20, -1},

        // Price change should generate ID 2, but event expects ID 99.
        {2, EventType::MODIFY_ORDER, 1, OrderSide::SELL, 10600, 20, 99}
    };

    ReplayResult result = engine.replay(events, book);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.processedEvents, 1);

    EXPECT_EQ(
        result.errorMessage,
        "Event 2: modified order ID mismatch"
    );

    // Original order must remain untouched.
    EXPECT_TRUE(book.hasOrder(1));
    EXPECT_FALSE(book.hasOrder(2));

    EXPECT_EQ(
        book.getQuantityAtPrice(OrderSide::SELL, 10500),
        20
    );

    EXPECT_EQ(
        book.getQuantityAtPrice(OrderSide::SELL, 10600),
        0
    );

    EXPECT_TRUE(book.validate());
}

TEST(OrderBookTests, RandomizedOperationsPreserveInvariants) {
    OrderBook book;

    // Fixed seed makes failures reproducible.
    std::mt19937 rng(42);

    std::uniform_int_distribution<int> operationDist(0, 99);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> priceDist(9900, 10100);
    std::uniform_int_distribution<int> quantityDist(1, 100);

    std::vector<int> activeOrderIds;

    const int totalOperations = 25000;

    for (int i = 0; i < totalOperations; ++i) {
        int operation = operationDist(rng);

        // 50% new orders, 25% cancellations, 25% modifications.
        if (operation < 50 || activeOrderIds.empty()) {
            OrderSide side = sideDist(rng) == 0
                ? OrderSide::BUY
                : OrderSide::SELL;

            int64_t price = priceDist(rng);
            int quantity = quantityDist(rng);

            int orderId = book.addOrder(side, price, quantity);

            ASSERT_NE(orderId, -1);

            if (book.hasOrder(orderId)) {
                activeOrderIds.push_back(orderId);
            }
        }
        else if (operation < 75) {
            std::uniform_int_distribution<std::size_t> indexDist(
                0, activeOrderIds.size() - 1
            );

            int orderId = activeOrderIds[indexDist(rng)];

            book.cancelOrder(orderId);
        }
        else {
            std::uniform_int_distribution<std::size_t> indexDist(
                0, activeOrderIds.size() - 1
            );

            int orderId = activeOrderIds[indexDist(rng)];

            int64_t newPrice = priceDist(rng);
            int newQuantity = quantityDist(rng);

            int modifiedId = book.modifyOrder(
                orderId,
                newPrice,
                newQuantity
            );

            ASSERT_NE(modifiedId, -1);

            if (book.hasOrder(modifiedId) &&
    std::find(
        activeOrderIds.begin(),
        activeOrderIds.end(),
        modifiedId
    ) == activeOrderIds.end()) {
    activeOrderIds.push_back(modifiedId);
}
        }

        // Remove IDs that were cancelled or fully executed.
        activeOrderIds.erase(
            std::remove_if(
                activeOrderIds.begin(),
                activeOrderIds.end(),
                [&book](int id) {
                    return !book.hasOrder(id);
                }
            ),
            activeOrderIds.end()
        );

        // Verify internal consistency after every operation.
        ASSERT_TRUE(book.validate())
            << "Invariant violation after operation " << i;
    }
    const auto& trades = book.getTrades();

for (const Trade& trade : trades) {
    EXPECT_GT(trade.buyOrderId, 0);
    EXPECT_GT(trade.sellOrderId, 0);
    EXPECT_NE(trade.buyOrderId, trade.sellOrderId);

    EXPECT_GT(trade.executionPrice, 0);
    EXPECT_GT(trade.quantity, 0);
}
    EXPECT_TRUE(book.validate());
}


TEST(OrderBookTests, RandomizedEventReplayIsDeterministic) {
    constexpr int totalEvents = 10000;
    constexpr unsigned int seed = 42;

    std::mt19937 rng(seed);

    std::uniform_int_distribution<int> operationDist(0, 99);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> priceDist(9900, 10100);
    std::uniform_int_distribution<int> quantityDist(1, 100);

    // This book generates a valid event history.
    OrderBook sourceBook;
    std::vector<OrderEvent> events;
    std::vector<int> activeOrderIds;

    for (int i = 0; i < totalEvents; ++i) {
        int operation = operationDist(rng);

        // Generate a new order.
        if (operation < 55 || activeOrderIds.empty()) {
            OrderSide side =
                sideDist(rng) == 0 ? OrderSide::BUY : OrderSide::SELL;

            int64_t price = priceDist(rng);
            int quantity = quantityDist(rng);

            int expectedId = sourceBook.peekNextOrderId();

            events.push_back({
                static_cast<std::uint64_t>(events.size() + 1),
                EventType::NEW_ORDER,
                expectedId,
                side,
                price,
                quantity,
                -1
            });

            int actualId = sourceBook.addOrder(side, price, quantity);

            ASSERT_EQ(actualId, expectedId);

            if (sourceBook.hasOrder(actualId)) {
                activeOrderIds.push_back(actualId);
            }
        }
        // Cancel an existing order.
        else if (operation < 75) {
            std::uniform_int_distribution<int> indexDist(
                0, static_cast<int>(activeOrderIds.size()) - 1
            );

            int orderId = activeOrderIds[indexDist(rng)];

            events.push_back({
                static_cast<std::uint64_t>(events.size() + 1),
                EventType::CANCEL_ORDER,
                orderId,
                OrderSide::BUY,
                0,
                0,
                -1
            });

            ASSERT_TRUE(sourceBook.cancelOrder(orderId));
        }
        // Modify an existing order.
        else {
            std::uniform_int_distribution<int> indexDist(
                0, static_cast<int>(activeOrderIds.size()) - 1
            );

            int orderId = activeOrderIds[indexDist(rng)];
            int64_t newPrice = priceDist(rng);
            int newQuantity = quantityDist(rng);

            int expectedModifiedId = sourceBook.previewModifyOrderId(
                orderId, newPrice, newQuantity
            );

            ASSERT_GT(expectedModifiedId, 0);

            events.push_back({
                static_cast<std::uint64_t>(events.size() + 1),
                EventType::MODIFY_ORDER,
                orderId,
                OrderSide::BUY,
                newPrice,
                newQuantity,
                expectedModifiedId
            });

            int actualModifiedId = sourceBook.modifyOrder(
                orderId, newPrice, newQuantity
            );

            ASSERT_EQ(actualModifiedId, expectedModifiedId);

            if (sourceBook.hasOrder(actualModifiedId) &&
                std::find(
                    activeOrderIds.begin(),
                    activeOrderIds.end(),
                    actualModifiedId
                ) == activeOrderIds.end()) {
                activeOrderIds.push_back(actualModifiedId);
            }
        }

        // Remove orders that have been filled or cancelled.
        activeOrderIds.erase(
            std::remove_if(
                activeOrderIds.begin(),
                activeOrderIds.end(),
                [&sourceBook](int id) {
                    return !sourceBook.hasOrder(id);
                }
            ),
            activeOrderIds.end()
        );

        ASSERT_TRUE(sourceBook.validate());
    }

    // Replay the exact same event history into two fresh books.
    OrderBook bookA;
    OrderBook bookB;

    ReplayEngine replayEngine;

    ReplayResult resultA = replayEngine.replay(events, bookA);
    ReplayResult resultB = replayEngine.replay(events, bookB);

    ASSERT_TRUE(resultA.success) << resultA.errorMessage;
    ASSERT_TRUE(resultB.success) << resultB.errorMessage;

    EXPECT_EQ(resultA.processedEvents, totalEvents);
    EXPECT_EQ(resultB.processedEvents, totalEvents);

    EXPECT_EQ(resultA.processedEvents, totalEvents);
EXPECT_EQ(resultB.processedEvents, totalEvents);

// Verify order-ID generation state is also reproduced.
EXPECT_EQ(bookA.peekNextOrderId(), sourceBook.peekNextOrderId());
EXPECT_EQ(bookB.peekNextOrderId(), sourceBook.peekNextOrderId());

// Both replayed books must satisfy their invariants.
EXPECT_TRUE(bookA.validate());
EXPECT_TRUE(bookB.validate());
    // Both replayed books must satisfy their invariants.
    EXPECT_TRUE(bookA.validate());
    EXPECT_TRUE(bookB.validate());

    // Compare trade histories.
    const auto& tradesA = bookA.getTrades();
    const auto& tradesB = bookB.getTrades();

    ASSERT_EQ(tradesA.size(), tradesB.size());

    for (std::size_t i = 0; i < tradesA.size(); ++i) {
        EXPECT_EQ(tradesA[i].buyOrderId, tradesB[i].buyOrderId);
        EXPECT_EQ(tradesA[i].sellOrderId, tradesB[i].sellOrderId);
        EXPECT_EQ(tradesA[i].executionPrice, tradesB[i].executionPrice);
        EXPECT_EQ(tradesA[i].quantity, tradesB[i].quantity);
    }

    // Compare replayed trades against the original source book.
    const auto& sourceTrades = sourceBook.getTrades();

    ASSERT_EQ(tradesA.size(), sourceTrades.size());

    for (std::size_t i = 0; i < tradesA.size(); ++i) {
    EXPECT_EQ(tradesA[i].buyOrderId, sourceTrades[i].buyOrderId);
    EXPECT_EQ(tradesA[i].sellOrderId, sourceTrades[i].sellOrderId);
    EXPECT_EQ(tradesA[i].executionPrice, sourceTrades[i].executionPrice);
    EXPECT_EQ(tradesA[i].quantity, sourceTrades[i].quantity);
    }

    // Compare final order-book state.
    // Check every generated order ID.
    int lastGeneratedId = sourceBook.peekNextOrderId();

    for (int id = 1; id < lastGeneratedId; ++id) {
        EXPECT_EQ(bookA.hasOrder(id), bookB.hasOrder(id));
        EXPECT_EQ(bookA.hasOrder(id), sourceBook.hasOrder(id));
    }

    // Check aggregate quantities at every tested price.
    for (int64_t price = 9900; price <= 10100; ++price) {
        EXPECT_EQ(
            bookA.getQuantityAtPrice(OrderSide::BUY, price),
            bookB.getQuantityAtPrice(OrderSide::BUY, price)
        );

        EXPECT_EQ(
            bookA.getQuantityAtPrice(OrderSide::SELL, price),
            bookB.getQuantityAtPrice(OrderSide::SELL, price)
        );

        EXPECT_EQ(
            bookA.getQuantityAtPrice(OrderSide::BUY, price),
            sourceBook.getQuantityAtPrice(OrderSide::BUY, price)
        );

        EXPECT_EQ(
            bookA.getQuantityAtPrice(OrderSide::SELL, price),
            sourceBook.getQuantityAtPrice(OrderSide::SELL, price)
        );
    }
}


