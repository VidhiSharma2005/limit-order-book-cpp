
#include <gtest/gtest.h>

#include "OrderBook.h"

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






