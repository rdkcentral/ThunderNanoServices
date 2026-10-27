# SimpleRichError example

This plugin implements `Example::ISimpleRichError` and demonstrates how a method can return Thunder's framework error together with independently optional JSON-RPC error details.

Build NanoServices with `EXAMPLE_SIMPLERICHERROR=ON`. The plugin is activated by default with the `SimpleRichError` callsign.

## Demonstrated responses

- Call `makeCoffee` before `restart` to receive explicit `code`, `message`, and typed diagnostic `data`.
- Call `restart` once to start the simulated machine. Calling it again returns a message-only rich error while preserving Thunder's code.
- Request `espresso` to receive a successful duration.
- Request `cappuccino` to receive data-only details while preserving Thunder's default code and message.
- Request `latte` to receive message and diagnostic data while preserving Thunder's default code.
- Request `ristretto` to receive Thunder default message and code only (error details not set).

Example requests:

```json
{"jsonrpc":"2.0","id":1,"method":"SimpleRichError.1.restart"}
```

```json
{"jsonrpc":"2.0","id":2,"method":"SimpleRichError.1.makeCoffee","params":{"type":"cappucino"}}
```
