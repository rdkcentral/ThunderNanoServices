#!/usr/bin/env python3
"""
Test script for Thunder JsonRpcMuxer plugin
Tests both HTTP JSON-RPC and WebSocket interfaces
"""

import json
import requests
import websocket
import threading
import time
import argparse
from typing import List, Dict, Any
from dataclasses import dataclass


@dataclass
class TestConfig:
    """Configuration for tests"""
    host: str = "localhost"
    port: int = 55555
    timeout: int = 5000
    max_batch_size: int = 10
    max_batches: int = 5


class Colors:
    """ANSI color codes for terminal output"""
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'


def print_test(name: str):
    """Print test name"""
    print(f"\n{Colors.BOLD}{Colors.BLUE}[TEST]{Colors.RESET} {name}")


def print_pass(message: str):
    """Print success message"""
    print(f"{Colors.GREEN}✓{Colors.RESET} {message}")


def print_fail(message: str):
    """Print failure message"""
    print(f"{Colors.RED}✗{Colors.RESET} {message}")


def print_info(message: str):
    """Print info message"""
    print(f"{Colors.YELLOW}ℹ{Colors.RESET} {message}")


class JsonRpcMuxerTester:
    """Test harness for JsonRpcMuxer"""
    
    def __init__(self, config: TestConfig):
        self.config = config
        self.http_url = f"http://{config.host}:{config.port}/jsonrpc"
        self.ws_url = f"ws://{config.host}:{config.port}/Service/JsonRpcMuxer"
        self.passed = 0
        self.failed = 0
    
    def create_batch(self, count: int) -> List[Dict[str, Any]]:
        """Create a batch of test requests"""
        batch = []
        for i in range(count):
            # Alternate between different Controller methods
            methods = [
                {"method": "Controller.1.subsystems", "params": {}},
                {"method": "Controller.1.version", "params": {}},
                {"method": "Controller.1.buildinfo", "params": {}},
                {"method": "Controller.1.status", "params": {}},
            ]
            method_data = methods[i % len(methods)]
            batch.append({
                "id": i + 1,
                "jsonrpc": "2.0",
                "method": method_data["method"],
                "params": method_data["params"]
            })
        return batch
    
    def plugin_state(self, state, wait = 0):
        time.sleep(wait)

        try:
            payload = {
                "jsonrpc": "2.0",
                "id": 405,
                "method": "Controller.1.status@JsonRpcMuxer",
            }
            
            response = requests.post(
                self.http_url,
                json=payload,
                timeout=self.config.timeout / 1000
            )

            return (response.status_code == 200) and (response.json()["result"]["state"] == state)
        except Exception as e:
            return False


    def activate_plugin(self):
        try:
            payload = {
                "jsonrpc": "2.0",
                "id": 405,
                "method": "Controller.1.activate",
                "params": {
                    "callsign": "JsonRpcMuxer"
                }
            }
            
            response = requests.post(
                self.http_url,
                json=payload,
                timeout=self.config.timeout / 1000
            )

            return (response.status_code == 200) and self.plugin_state("Activated", 0.5)
        
        except Exception as e:
            return False

    def deactivate_plugin(self):
        try:
            payload = {
                "jsonrpc": "2.0",
                "id": 404,
                "method": "Controller.1.deactivate",
                "params": {
                    "callsign": "JsonRpcMuxer"
                }
            }
            
            response = requests.post(
                self.http_url,
                json=payload,
                timeout=self.config.timeout / 1000
            )

            return (response.status_code == 200) and self.plugin_state("Deactivated", 0.5)

        except Exception as e:
            return False

    def test_http_single_batch(self):
        """Test HTTP interface with a single batch"""
        print_test("HTTP: Single batch (4 requests)")
        
        batch = self.create_batch(4)
        payload = {
            "jsonrpc": "2.0",
            "id": 100,
            "method": "JsonRpcMuxer.1.invoke",
            "params": batch
        }
        
        try:
            response = requests.post(
                self.http_url,
                json=payload,
                timeout=self.config.timeout / 1000
            )
            
            if response.status_code != 200:
                print_fail(f"HTTP status code: {response.status_code}")
                self.failed += 1
                return
            
            data = response.json()
            
            # Check response structure
            if "result" not in data:
                print_fail("No 'result' field in response")
                self.failed += 1
                return
            
            results = data["result"]
            
            if len(results) != len(batch):
                print_fail(f"Expected {len(batch)} results, got {len(results)}")
                self.failed += 1
                return
            
            # Check all results have either 'result' or 'error'
            for i, result in enumerate(results):
                if "result" not in result and "error" not in result:
                    print_fail(f"Result {i} has neither 'result' nor 'error'")
                    self.failed += 1
                    return
            
            print_pass(f"Received {len(results)} results")
            print_info(f"Response time: {response.elapsed.total_seconds():.3f}s")
            self.passed += 1
            
        except requests.exceptions.Timeout:
            print_fail("Request timed out")
            self.failed += 1
        except Exception as e:
            print_fail(f"Exception: {e}")
            self.failed += 1
    
    def test_http_max_batch_size(self):
        """Test HTTP interface with maximum batch size"""
        print_test(f"HTTP: Maximum batch size ({self.config.max_batch_size} requests)")
        
        batch = self.create_batch(self.config.max_batch_size)
        payload = {
            "jsonrpc": "2.0",
            "id": 101,
            "method": "JsonRpcMuxer.1.invoke",
            "params": batch
        }
        
        try:
            response = requests.post(
                self.http_url,
                json=payload,
                timeout=self.config.timeout / 1000
            )
            
            if response.status_code != 200:
                print_fail(f"HTTP status code: {response.status_code}")
                self.failed += 1
                return
            
            data = response.json()
            
            if "result" not in data:
                print_fail("No 'result' field in response")
                self.failed += 1
                return
            
            results = data["result"]
            
            if len(results) != self.config.max_batch_size:
                print_fail(f"Expected {self.config.max_batch_size} results, got {len(results)}")
                self.failed += 1
                return
            
            print_pass(f"Successfully processed max batch size ({self.config.max_batch_size})")
            self.passed += 1
            
        except Exception as e:
            print_fail(f"Exception: {e}")
            self.failed += 1
    
    def test_http_exceeds_batch_size(self):
        """Test that batches exceeding max size are rejected"""
        print_test(f"HTTP: Exceed batch size limit (should reject)")
        
        batch = self.create_batch(self.config.max_batch_size + 1)
        payload = {
            "jsonrpc": "2.0",
            "id": 102,
            "method": "JsonRpcMuxer.1.invoke",
            "params": batch
        }
        
        try:
            response = requests.post(
                self.http_url,
                json=payload,
                timeout=self.config.timeout / 1000
            )
            
            data = response.json()
            
            # Should get an error
            if "error" in data:
                print_pass(f"Correctly rejected: {data['error'].get('message', 'Unknown error')}")
                self.passed += 1
            else:
                print_fail("Should have been rejected but wasn't")
                self.failed += 1
                
        except Exception as e:
            print_fail(f"Exception: {e}")
            self.failed += 1
    
    def test_http_empty_batch(self):
        """Test that empty batches are rejected"""
        print_test("HTTP: Empty batch (should reject)")
        
        payload = {
            "jsonrpc": "2.0",
            "id": 103,
            "method": "JsonRpcMuxer.1.invoke",
            "params": []
        }
        
        try:
            response = requests.post(
                self.http_url,
                json=payload,
                timeout=self.config.timeout / 1000
            )
            
            data = response.json()
            
            if "error" in data:
                print_pass(f"Correctly rejected: {data['error'].get('message', 'Unknown error')}")
                self.passed += 1
            else:
                print_fail("Should have been rejected but wasn't")
                self.failed += 1
                
        except Exception as e:
            print_fail(f"Exception: {e}")
            self.failed += 1
    
    def test_http_concurrent_batches(self):
        """Test multiple concurrent batches"""
        print_test(f"HTTP: Concurrent batches (3 batches simultaneously)")
        
        results = []
        errors = []
        
        def send_batch(batch_id: int):
            try:
                batch = self.create_batch(3)
                payload = {
                    "jsonrpc": "2.0",
                    "id": 200 + batch_id,
                    "method": "JsonRpcMuxer.1.invoke",
                    "params": batch
                }
                
                response = requests.post(
                    self.http_url,
                    json=payload,
                    timeout=self.config.timeout / 1000
                )
                
                results.append(response.json())
                
            except Exception as e:
                errors.append(str(e))
        
        threads = []
        for i in range(3):
            t = threading.Thread(target=send_batch, args=(i,))
            threads.append(t)
            t.start()
        
        for t in threads:
            t.join()
        
        if errors:
            print_fail(f"Errors occurred: {errors}")
            self.failed += 1
        elif len(results) == 3:
            print_pass(f"All 3 concurrent batches completed successfully")
            self.passed += 1
        else:
            print_fail(f"Expected 3 results, got {len(results)}")
            self.failed += 1
    
    def test_http_cancel_batches(self):
        """Test batch cancellation by plugin deactivation"""
        print_test(f"HTTP: Batch cancellation by plugin deactivation while processing")
        
        results = []
        errors = []
        timeouts = []

        def send_batch(batch_id: int):
            try:
                batch = self.create_batch(self.config.max_batch_size)
                payload = {
                    "jsonrpc": "2.0",
                    "id": 200 + batch_id,
                    "method": "JsonRpcMuxer.1.invoke",
                    "params": batch
                }
                
                response = requests.post(
                    self.http_url,
                    json=payload,
                    timeout=self.config.timeout / 1000
                )

                data = response.json()
                
                if "error" in data:
                    if "Service is not active" in data["error"].get("message", ""):
                        timeouts.append(batch_id)
                    else:
                        errors.append(f"Batch {batch_id}: Unexpected error: {data['error']}")

            except requests.exceptions.Timeout:
                timeouts.append(batch_id)
            except Exception as e:
                print_fail(f"Error Sending batch: {batch_id}")
                errors.append(str(e))
        
        threads = []

        # schedule plugin deactivation
        deactivate = threading.Timer(0.0002, self.deactivate_plugin)
        deactivate.start()

        for i in range(self.config.max_batches):
            t = threading.Thread(target=send_batch, args=(i,))
            threads.append(t)
            t.start()
        
        for t in threads:
            t.join()
        
        time.sleep(0.5)
        deactivate.cancel()

        if self.activate_plugin() == False:
            print_fail(f"Plugin not active")
            self.activate_plugin()
            self.failed += 1
        elif errors:
            print_fail(f"Errors occurred: {errors}")
            self.failed += 1
        elif timeouts:
            print_pass(f"Received {timeouts} timeouts")
            self.passed += 1
        else:
            print_fail(f"Expected {timeouts} got results, {results}")
            self.failed += 1
    
    def test_websocket_single_batch(self):
        """Test WebSocket interface with a single batch"""
        print_test("WebSocket: Single batch (4 requests)")
        
        try:
            ws = websocket.create_connection(
                self.ws_url, 
                timeout=self.config.timeout / 1000,
                subprotocols=["json"]
            )
            
            batch = self.create_batch(4)
            
            # WebSocket message format
            message = {
                "jsonrpc": "2.0",
                "id": 300,
                "params": batch
            }
            
            ws.send(json.dumps(message))
            
            # Wait for response
            response_str = ws.recv()
            response = json.loads(response_str)
            
            ws.close()
            
            # Check response
            if "result" not in response:
                print_fail("No 'result' field in response")
                self.failed += 1
                return
            
            # Result is a JSON string that needs to be parsed
            result_str = response["result"]
            if isinstance(result_str, str):
                results = json.loads(result_str)
            else:
                results = result_str
            
            if len(results) != len(batch):
                print_fail(f"Expected {len(batch)} results, got {len(results)}")
                self.failed += 1
                return
            
            print_pass(f"Received {len(results)} results via WebSocket")
            self.passed += 1
            
        except Exception as e:
            print_fail(f"Exception: {e}")
            self.failed += 1
    
    def test_websocket_max_batch_size(self):
        """Test WebSocket with maximum batch size"""
        print_test(f"WebSocket: Maximum batch size ({self.config.max_batch_size} requests)")
        
        try:
            ws = websocket.create_connection(
                self.ws_url, 
                timeout=self.config.timeout / 1000,
                subprotocols=["json"]
            )
            
            batch = self.create_batch(self.config.max_batch_size)
            
            message = {
                "jsonrpc": "2.0",
                "id": 301,
                "params": batch
            }
            
            ws.send(json.dumps(message))
            response_str = ws.recv()
            response = json.loads(response_str)
            
            ws.close()
            
            if "result" not in response:
                print_fail("No 'result' field in response")
                self.failed += 1
                return
            
            # Result is a JSON string that needs to be parsed
            result_str = response["result"]
            if isinstance(result_str, str):
                results = json.loads(result_str)
            else:
                results = result_str
            
            if len(results) != self.config.max_batch_size:
                print_fail(f"Expected {self.config.max_batch_size} results, got {len(results)}")
                self.failed += 1
                return
            
            print_pass(f"Successfully processed max batch size ({self.config.max_batch_size})")
            self.passed += 1
            
        except Exception as e:
            print_fail(f"Exception: {e}")
            self.failed += 1
    
    def test_websocket_exceeds_batch_size(self):
        """Test that WebSocket rejects batches exceeding max size"""
        print_test("WebSocket: Exceed batch size limit (should reject)")
        
        try:
            ws = websocket.create_connection(
                self.ws_url, 
                timeout=self.config.timeout / 1000,
                subprotocols=["json"]
            )
            
            batch = self.create_batch(self.config.max_batch_size + 1)
            
            message = {
                "jsonrpc": "2.0",
                "id": 302,
                "params": batch
            }
            
            ws.send(json.dumps(message))
            response_str = ws.recv()
            response = json.loads(response_str)
            
            ws.close()
            
            # Should get an error
            if "error" in response:
                print_pass(f"Correctly rejected: {response['error'].get('message', 'Unknown error')}")
                self.passed += 1
            else:
                print_fail("Should have been rejected but wasn't")
                self.failed += 1
                
        except Exception as e:
            print_fail(f"Exception: {e}")
            self.failed += 1
    
    def test_websocket_multiple_connections(self):
        """Test that only one WebSocket connection is allowed"""
        print_test("WebSocket: Multiple connections (should reject second)")
        
        try:
            ws1 = websocket.create_connection(
                self.ws_url, 
                timeout=self.config.timeout / 1000,
                subprotocols=["json"]
            )
            print_info("First WebSocket connected")
            
            try:
                ws2 = websocket.create_connection(
                    self.ws_url, 
                    timeout=2,
                    subprotocols=["json"]
                )
                print_fail("Second WebSocket should have been rejected")
                ws2.close()
                self.failed += 1
            except Exception:
                print_pass("Second WebSocket correctly rejected")
                self.passed += 1
            
            ws1.close()
            
        except Exception as e:
            print_fail(f"Exception: {e}")
            self.failed += 1
    
    def test_websocket_cancel_batches(self):
        """Test that WebSocket cancels batches"""
        print_test("WebSocket: Try to trigger batch cancellation")
        
        try:
            ws = websocket.create_connection(
                self.ws_url, 
                timeout=self.config.timeout / 1000,
                subprotocols=["json"]
            )
            
            batch = self.create_batch(self.config.max_batch_size)
            
            message = {
                "jsonrpc": "2.0",
                "id": 302,
                "params": batch
            }
           
            ws.send(json.dumps(message))
            ws.close()
            
            # plugin not crashed?
            if self.plugin_state("Activated", 2.0):
                print_pass(f"Plugin successfully survived a channel closure while processing")
                self.passed += 1
            else:
                print_fail("restarting plugin... ")
                self.activate_plugin()
                self.failed += 1
                
        except Exception as e:
            print_fail(f"Exception: {e}")
            self.failed += 1

    def test_websocket_disable_plugin(self):
        """Test that WebSocket cancels batches by disabling the plugin mid-flight"""
        print_test("WebSocket: Try to trigger batch cancellation by disabling the plugin mid-flight")
        
        try:
            ws = websocket.create_connection(
                self.ws_url, 
                timeout=self.config.timeout / 1000,
                subprotocols=["json"]
            )
            
            batch = self.create_batch(self.config.max_batch_size)
            
            message = {
                "jsonrpc": "2.0",
                "id": 302,
                "params": batch
            }
           
            # schedule plugin deactivation
            deactivate = threading.Timer(0.00002, self.deactivate_plugin)
            deactivate.start()
           
            ws.send(json.dumps(message))
            ws.close()

            time.sleep(2.0)
            deactivate.cancel()
            
            # plugin caused a crash?
            if self.activate_plugin():
                print_pass(f"Plugin successfully survived a mid-flight deactivation")
                self.passed += 1
            else:
                print_fail("restarting plugin... ")
                self.activate_plugin()
                self.failed += 1
            
        except Exception as e:
            print_fail(f"Exception: {e}")
            self.failed += 1
    
    def test_threadpool_overload(self):
        """Test system behavior under heavy concurrent load"""
        print_test(f"Threadpool overload: {self.config.max_batches + 3} concurrent batches")
        print_info(f"Expecting {self.config.max_batches} to succeed, rest to be rejected")
        
        results = []
        errors = []
        accepted = 0
        rejected = 0
        
        def send_batch(batch_id: int):
            nonlocal accepted, rejected
            try:
                batch = self.create_batch(self.config.max_batch_size)
                payload = {
                    "jsonrpc": "2.0",
                    "id": 400 + batch_id,
                    "method": "JsonRpcMuxer.1.invoke",
                    "params": batch
                }
                
                response = requests.post(
                    self.http_url,
                    json=payload,
                    timeout=self.config.timeout / 1000
                )
                
                data = response.json()
                
                if "error" in data:
                    # Rejected due to maxbatches limit
                    if "Too many concurrent batches" in data["error"].get("message", ""):
                        rejected += 1
                    else:
                        errors.append(f"Batch {batch_id}: Unexpected error: {data['error']}")
                elif "result" in data:
                    # Accepted and processed
                    accepted += 1
                else:
                    errors.append(f"Batch {batch_id}: No result or error in response")
                    
                results.append(data)
                
            except requests.exceptions.Timeout:
                errors.append(f"Batch {batch_id}: Timeout")
            except Exception as e:
                errors.append(f"Batch {batch_id}: {str(e)}")
        
        # Send more batches than maxbatches allows
        num_batches = self.config.max_batches + 3
        threads = []
        
        for i in range(num_batches):
            t = threading.Thread(target=send_batch, args=(i,))
            threads.append(t)
            t.start()
            # Small stagger to simulate realistic arrival pattern
            time.sleep(0.01)
        
        for t in threads:
            t.join()
        
        print_info(f"Accepted: {accepted}, Rejected: {rejected}, Errors: {len(errors)}")
        
        if errors:
            for error in errors[:5]:  # Show first 5 errors
                print_info(f"  {error}")
            if len(errors) > 5:
                print_info(f"  ... and {len(errors) - 5} more errors")
        
        # We expect:
        # - Some batches to be accepted (up to maxbatches)
        # - Some batches to be rejected (the excess ones)
        # - Total = num_batches
        total_handled = accepted + rejected
        
        if total_handled != num_batches:
            print_fail(f"Expected {num_batches} responses, got {total_handled}")
            self.failed += 1
        elif rejected < 1:
            print_fail(f"Expected some rejections due to maxbatches limit, got 0")
            self.failed += 1
        elif accepted < 1:
            print_fail(f"Expected some acceptances, got 0")
            self.failed += 1
        else:
            print_pass(f"System correctly handled overload: {accepted} accepted, {rejected} rejected")
            self.passed += 1
    
    def run_all_tests(self):
        """Run all tests"""
        print(f"\n{Colors.BOLD}JsonRpcMuxer Test Suite{Colors.RESET}")
        print(f"Target: {self.config.host}:{self.config.port}")
        print(f"Config: maxbatchsize={self.config.max_batch_size}, maxbatches={self.config.max_batches}, timeout={self.config.timeout}ms")
        
        # HTTP tests
        print(f"\n{Colors.BOLD}=== HTTP Tests ==={Colors.RESET}")
        self.test_http_single_batch()
        self.test_http_max_batch_size()
        self.test_http_exceeds_batch_size()
        self.test_http_empty_batch()
        self.test_http_concurrent_batches()
        self.test_http_cancel_batches()
        
        # WebSocket tests
        print(f"\n{Colors.BOLD}=== WebSocket Tests ==={Colors.RESET}")
        self.test_websocket_single_batch()
        self.test_websocket_max_batch_size()
        self.test_websocket_exceeds_batch_size()
        self.test_websocket_multiple_connections()
        self.test_websocket_cancel_batches()
        self.test_websocket_disable_plugin()
        
        # Summary
        print(f"\n{Colors.BOLD}=== Test Summary ==={Colors.RESET}")
        total = self.passed + self.failed
        print(f"Total: {total}")
        print(f"{Colors.GREEN}Passed: {self.passed}{Colors.RESET}")
        print(f"{Colors.RED}Failed: {self.failed}{Colors.RESET}")
        
        if self.failed == 0:
            print(f"\n{Colors.GREEN}{Colors.BOLD}✓ All tests passed!{Colors.RESET}")
            return 0
        else:
            print(f"\n{Colors.RED}{Colors.BOLD}✗ Some tests failed{Colors.RESET}")
            return 1


def main():
    parser = argparse.ArgumentParser(description="Test Thunder JsonRpcMuxer plugin")
    parser.add_argument("--host", default="localhost", help="Thunder host (default: localhost)")
    parser.add_argument("--port", type=int, default=55555, help="Thunder port (default: 55555)")
    parser.add_argument("--timeout", type=int, default=5000, help="Timeout in ms (default: 5000)")
    parser.add_argument("--max-batch-size", type=int, default=10, help="Expected max batch size (default: 10)")
    parser.add_argument("--max-batches", type=int, default=5, help="Expected max concurrent batches (default: 5)")
    
    args = parser.parse_args()
    
    config = TestConfig(
        host=args.host,
        port=args.port,
        timeout=args.timeout,
        max_batch_size=args.max_batch_size,
        max_batches=args.max_batches
    )
    
    tester = JsonRpcMuxerTester(config)
    exit_code = tester.run_all_tests()
    
    exit(exit_code)


if __name__ == "__main__":
    main()
