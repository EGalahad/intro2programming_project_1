# project stock report

## implementation ideas

1. To sort the buy order list from large to small, I chose the minus of the price as the key when insert into the set, so I won't be bothered by sorting algorithms
2. To implement "atomic" transactions I "get" a common logger id at the beginning of each transaction 

## experience gained

1. write dedicated specific debug message (do not use one #ifdef DEBUG over all messages, but use macros like DEBUG_BUY to gain granularity in controlling the debug behaviour)
2. write illustrative debug messages (specify where the message it output from, its behaviour, etc)
3. write checker functions in Market class to check the state of the whole market

