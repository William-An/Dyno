# Simple HTTP server

## Structure

1. Core
   1. Receive request
   2. Route redirecting
   3. Middlewares
      1. Header processing
      2. Request and response
      3. Template engine
      4. Logger
   4. Send response
2. User
   1. Registrate handlers

## TODO

1. [ ] HTTP/1.1 Protocol Support
2. [ ] Route Handler
   1. [ ] Handler function signature
   2. [ ] Dir query parsing
   3. [ ] GET, POST, PUT
   4. [ ] Dir handler registration
      1. [ ] Maintain a global route:handler func pair
      2. [ ] Distribute the incoming request in the core engine
3. [ ] HTML Template engine
   1. [ ] Mustache?
4. [ ] Middlewares
   1. [ ] Request and response
      1. [ ] Header info store in hashtable
   2. [ ] ERROR handler?
   3. [ ] Cookie jar opener
5. [ ] HTTPS Support
6. [ ] Better logger
