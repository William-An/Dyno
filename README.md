# Simple HTTP server

## TODO

1. [x] Basic HTTP server
2. [x] HTTP Auth
3. [x] Concurrency
   1. [x] Check `getopt` for argument parsing
4. [ ] Advance HTTP
   1. [ ] Match the full HTTP 1.1 protocol 
   2. [ ] Dir query parsing
   3. [ ] Header info store in hashtable
   4. [ ] GET, POST, PUT
   5. [ ] Dir handler registration
   6. [ ] HTML Template engine
   7. [ ] ERROR handler?
   8. [ ] Cookie jar opener
   9. [ ] HTTPS Support
5. [ ] Better logger
   1. [ ] Write own logger?

## Development Note

1. Why `sockaddr_in` instead of `sockaddr`?
   1. `sockaddr_in` used for internet socket
   2. `sockaddr` is a more generic descriptor
   3. [here](https://stackoverflow.com/questions/21099041/why-do-we-cast-sockaddr-in-to-sockaddr-when-calling-bind)
2. Difference between `AF_INET` andd `PF_INET`
   1. address family and protocol family
   2. [ref](https://stackoverflow.com/questions/6729366/what-is-the-difference-between-af-inet-and-pf-inet-in-socket-programming)
3. `Telnet` on macOs
   1. Use `nc -vc <HOST> <POST>`
   2. Where `-v` asks for verbose output and `-c` sends `<CRLF>`
4. `backlog` arg in `listen` function
   1. [Check here](https://stackoverflow.com/questions/10002868/what-value-of-backlog-should-i-use)