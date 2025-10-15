#!/usr/bin/env python3
"""
Test script for Thunder JsonRpcMuxer plugin
Tests HTTP JSON-RPC interface
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
                {"method": "Controller.1.links", "params": {}},
                {"method": "Controller.1.environment@Controller", "params": {}},
                {"method": "Controller.1.configuration@Controller", "params": {}},
                {"method": "Controller.1.environment@JsonRpcMuxer", "params": {}},
                {"method": "Controller.1.configuration@JsonRpcMuxer", "params": {}},
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

    def test_http_single_batch(self, method):
        """Test HTTP interface with a single batch"""
        print_test("HTTP: Single batch (4 requests)")
        
        batch = self.create_batch(4)
        payload = {
            "jsonrpc": "2.0",
            "id": 100,
            "method": "JsonRpcMuxer.1." + method,
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
    
    def test_http_max_batch_size(self, method):
        """Test HTTP interface with maximum batch size"""
        print_test(f"HTTP: Maximum batch size ({self.config.max_batch_size} requests)")
        
        batch = self.create_batch(self.config.max_batch_size)
        payload = {
            "jsonrpc": "2.0",
            "id": 101,
            "method": "JsonRpcMuxer.1." + method,
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
    
    def test_http_exceeds_batch_size(self, method):
        """Test that batches exceeding max size are rejected"""
        print_test(f"HTTP: Exceed batch size limit (should reject)")
        
        batch = self.create_batch(self.config.max_batch_size + 1)
        payload = {
            "jsonrpc": "2.0",
            "id": 102,
            "method": "JsonRpcMuxer.1." + method,
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
    
    def test_http_empty_batch(self, method):
        """Test that empty batches are rejected"""
        print_test("HTTP: Empty batch (should reject)")
        
        payload = {
            "jsonrpc": "2.0",
            "id": 103,
            "method": "JsonRpcMuxer.1." + method,
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
    
    def test_http_concurrent_batches(self, method):
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
                    "method": "JsonRpcMuxer.1." + method,
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
    
    def test_http_cancel_batches(self, method):
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
                    "method": "JsonRpcMuxer.1." + method,
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
    
    def test_http_burst_batches(self, total_runs, method):
        """Test multiple concurrent batches"""
        print_test(f"HTTP: Concurrent batches ({total_runs} runs × {self.config.max_batches} connections)")
        
        results = []
        errors = []

        for r in range(total_runs):
            threads = []
            trigger = threading.Event()

            def send_batch(batch_id: int, run_id: int):
                try:
                    batch = self.create_batch(self.config.max_batch_size)
                    payload = {
                        "jsonrpc": "2.0",
                        "id": 2000 + run_id * 100 + batch_id,
                        "method": "JsonRpcMuxer.1." + method,
                        "params": batch
                    }

                    trigger.wait()

                    response = requests.post(
                        self.http_url,
                        json=payload,
                        timeout=self.config.timeout / 1000
                    )

                    results.append(response.json())

                except Exception as e:
                    errors.append(f"run={run_id} batch={batch_id}: {e}")

            for i in range(self.config.max_batches):
                t = threading.Thread(target=send_batch, args=(i, r))
                threads.append(t)
                t.start()

            trigger.set()

            for t in threads:
                t.join()

        if errors:
            print_fail(f"Errors occurred: {errors}")
            self.failed += 1
        elif len(results) == total_runs * self.config.max_batches:
            print_pass(f"All {len(results)} batches completed successfully")
            self.passed += 1
        else:
            print_fail(f"Expected {total_runs * self.config.max_batches} results, got {len(results)}")
            self.failed += 1

    def run_all_tests(self):
        """Run all tests"""
        print(f"\n{Colors.BOLD}JsonRpcMuxer Test Suite{Colors.RESET}")
        print(f"Target: {self.config.host}:{self.config.port}")
        print(f"Config: maxbatchsize={self.config.max_batch_size}, maxbatches={self.config.max_batches}, timeout={self.config.timeout}ms")
        
        # HTTP tests
        print(f"\n{Colors.BOLD}=== HTTP Tests ==={Colors.RESET}")
        self.test_http_single_batch("parallel")
        self.test_http_max_batch_size("parallel")
        self.test_http_exceeds_batch_size("parallel")
        self.test_http_empty_batch("parallel")
        self.test_http_concurrent_batches("parallel")
        self.test_http_cancel_batches("parallel")
        self.test_http_burst_batches(1000, "parallel")

        self.test_http_single_batch("sequential")
        self.test_http_max_batch_size("sequential")
        self.test_http_exceeds_batch_size("sequential")
        self.test_http_empty_batch("sequential")
        self.test_http_concurrent_batches("sequential")
        self.test_http_cancel_batches("sequential")
        self.test_http_burst_batches(1000, "sequential")
        
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
