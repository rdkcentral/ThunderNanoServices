# Thunder Web Security Test

The html page in this folder can be used to test the Thunder Web Security with the SecurityAgent plugin as found in ThunderNanoServicesRDK:

- build the SecurityAgent plugin with SECURITY_TESTING_MODE enabled
- add a value for the testtoken in the SecurityAgent plugin config (and for windows make sure to add a valid ip:port to the token provider key)
  example:
```json
 {
        "callsign": "SecurityAgent",
        "locator": "libSecurityAgent.so",
        "classname": "SecurityAgent",
        "startmode": "Activated",
        "configuration": {
            "connector": "127.0.0.1:25556",
            "testtoken" :  "<TokenTestKeyName>"
        }
    }
 ```
- start Thunder and activate the SecurityAgent plugin (if not set to automatic)
- Open the html page, fill in Thunder web ip and port, for the testtoken fill the value that was added for testtoken in the config (so in the example above that would be <TokenTestKeyName> )
- fill in the url and/or user you would like to have in the token payload (of course make sure it aligns with the content of the security agent acl file depending on what you want to test) Note: you can of course create just a new token after changing the url or user by clicking the Create Token button again.
- click the Create Token button and a token should appear in the "token to be used" edit box
- if you want this token to be used for the websocket tests and/or XHR request just enable the "Use Token for requests" checkbox.
