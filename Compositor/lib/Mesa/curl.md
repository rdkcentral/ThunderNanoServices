## set resolution

``` bash
curl --request POST \
  --url http://10.33.10.137/jsonrpc \
  --header 'content-type: application/json' \
  --data '{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Compositor.1.resolution",
  "params": "1080p"
}'
```


``` bash
curl --request POST \
  --url http://127.0.0.1:55555/jsonrpc \
  --header 'content-type: application/json' \
  --data '{"jsonrpc": "2.0", "id": 1,"method": "Controller.1.activate","params": {"callsign": "Screensaver"}}'
```

}

``` bash
curl --request GET \
  --url http://10.33.10.137/jsonrpc \
  --header 'content-type: application/json' \
  --data '{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Compositor.1.resolution"
}'
```