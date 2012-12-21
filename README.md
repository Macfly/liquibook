liquibook =========

Open source limit order book matching engine from [OCI](http://ociweb.com)

## Features
* Low-level components for order matching and aggregate depth tracking
* Memory-efficiency: minimal copying of data to internal structures
* Speed: up to __1.5 million__ inserts per second without depth tracking.  See full [performance history](liquibook/blob/master/PERFORMANCE.md).

## Flexibility
* Works with or without aggregate depth tracking
* Depth tracking to any number of levels (static)
* Works with smart or regular pointers

## Works with Your Design
* Preserves your order model, requiring only trivial interface
* Preserves your identifiers for securities, accounts, exchanges, orders, fills
* Use your threading system (or be single-threaded)
* Use your synchronization method
