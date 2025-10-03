#!/usr/bin/env python3
"""
Stress test for Thunder JsonRpcMuxer plugin
Tests threadpool overload and concurrent batch handling
"""

import json
import requests
import threading
import time
import argparse
from typing import List, Dict, Any
from dataclasses import dataclass


@dataclass
class TestConfig:
    """Configuration for stress tests"""
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


class StressTester:
    """Stress test harness for JsonRpcMuxer"""
    
    def __init__(self, config: TestConfig):
        self.config = config
        self.http_url = f"http://{config.host}:{config.port}/jsonrpc"
        self.passed = 0
        self.failed = 0
    
    def create_batch(self, count: int, use_slow_requests: bool = False) -> List[Dict[str, Any]]:
        """Create a batch of test requests"""
        batch = []
        for i in range(count):
            if use_slow_requests:
                # Use TimeSync.synchronize which does actual NTP work (~100-500ms per call)
                method_data = {"method": "TimeSync.1.synchronize", "params": {}}
            else:
                # Fast Controller methods
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
    
    def test_maxbatches_limit(self):
        """Test that maxbatches limit is enforced"""
        print_test(f"Maxbatches limit: {self.config.max_batches + 3} concurrent batches")
        print_info(f"Using TimeSync.synchronize for slow requests (~100-500ms each)")
        print_info(f"Expecting ~{self.config.max_batches} to succeed, rest to be rejected")
        
        results = []
        errors = []
        accepted = 0
        rejected = 0
        
        def send_batch(batch_id: int):
            nonlocal accepted, rejected
            try:
                # Use slow requests (TimeSync) for this test
                batch = self.create_batch(self.config.max_batch_size, use_slow_requests=True)
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
                    if "Too many concurrent batches" in data["error"].get("message", ""):
                        rejected += 1
                    else:
                        errors.append(f"Batch {batch_id}: Unexpected error: {data['error']}")
                elif "result" in data:
                    accepted += 1
                else:
                    errors.append(f"Batch {batch_id}: No result or error in response")
                    
                results.append(data)
                
            except requests.exceptions.Timeout:
                errors.append(f"Batch {batch_id}: Timeout")
            except Exception as e:
                errors.append(f"Batch {batch_id}: {str(e)}")
        
        num_batches = self.config.max_batches + 3
        threads = []
        
        # Create all threads first
        for i in range(num_batches):
            t = threading.Thread(target=send_batch, args=(i,))
            threads.append(t)
        
        # Start all at once (no stagger)
        for t in threads:
            t.start()
        
        for t in threads:
            t.join()
        
        print_info(f"Accepted: {accepted}, Rejected: {rejected}, Errors: {len(errors)}")
        
        if errors:
            for error in errors[:5]:
                print_info(f"  {error}")
            if len(errors) > 5:
                print_info(f"  ... and {len(errors) - 5} more errors")
        
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
    
    def test_sustained_load(self, duration_seconds: int = 10, batches_per_second: int = 2):
        """Test sustained load over time"""
        print_test(f"Sustained load: {batches_per_second} batches/sec for {duration_seconds}s")
        
        total_sent = 0
        total_accepted = 0
        total_rejected = 0
        total_errors = 0
        
        start_time = time.time()
        interval = 1.0 / batches_per_second
        
        def send_batch():
            nonlocal total_sent, total_accepted, total_rejected, total_errors
            
            total_sent += 1
            batch = self.create_batch(5)  # Smaller batches for sustained load
            payload = {
                "jsonrpc": "2.0",
                "id": 500 + total_sent,
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
                
                if "error" in data:
                    if "Too many concurrent batches" in data["error"].get("message", ""):
                        total_rejected += 1
                    else:
                        total_errors += 1
                elif "result" in data:
                    total_accepted += 1
                    
            except Exception:
                total_errors += 1
        
        print_info(f"Sending batches...")
        
        while time.time() - start_time < duration_seconds:
            send_batch()
            time.sleep(interval)
        
        elapsed = time.time() - start_time
        
        print_info(f"Duration: {elapsed:.1f}s")
        print_info(f"Sent: {total_sent}, Accepted: {total_accepted}, Rejected: {total_rejected}, Errors: {total_errors}")
        print_info(f"Acceptance rate: {100 * total_accepted / total_sent:.1f}%")
        
        if total_errors > total_sent * 0.1:  # More than 10% errors is bad
            print_fail(f"Too many errors: {total_errors} ({100 * total_errors / total_sent:.1f}%)")
            self.failed += 1
        elif total_accepted == 0:
            print_fail("No batches were accepted")
            self.failed += 1
        else:
            print_pass(f"System handled sustained load successfully")
            self.passed += 1
    
    def test_rapid_fire(self, count: int = 20):
        """Test rapid-fire batch submission"""
        print_test(f"Rapid fire: {count} batches as fast as possible")
        
        results = []
        start_time = time.time()
        
        def send_batch(batch_id: int):
            try:
                batch = self.create_batch(3)
                payload = {
                    "jsonrpc": "2.0",
                    "id": 600 + batch_id,
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
                results.append({"error": str(e)})
        
        threads = []
        for i in range(count):
            t = threading.Thread(target=send_batch, args=(i,))
            threads.append(t)
            t.start()
        
        for t in threads:
            t.join()
        
        elapsed = time.time() - start_time
        
        accepted = sum(1 for r in results if "result" in r)
        rejected = sum(1 for r in results if "error" in r and isinstance(r["error"], dict))
        errors = len(results) - accepted - rejected
        
        print_info(f"Completed in {elapsed:.3f}s")
        print_info(f"Throughput: {count / elapsed:.1f} batches/sec")
        print_info(f"Accepted: {accepted}, Rejected: {rejected}, Errors: {errors}")
        
        if errors > count * 0.2:  # More than 20% errors
            print_fail(f"Too many errors: {errors}")
            self.failed += 1
        else:
            print_pass(f"Rapid fire handled: {accepted}/{count} accepted")
            self.passed += 1
    
    def run_all_tests(self):
        """Run all stress tests"""
        print(f"\n{Colors.BOLD}JsonRpcMuxer Stress Test Suite{Colors.RESET}")
        print(f"Target: {self.config.host}:{self.config.port}")
        print(f"Config: maxbatchsize={self.config.max_batch_size}, maxbatches={self.config.max_batches}")
        
        self.test_maxbatches_limit()
        self.test_rapid_fire(count=20)
        self.test_sustained_load(duration_seconds=5, batches_per_second=3)
        
        print(f"\n{Colors.BOLD}=== Stress Test Summary ==={Colors.RESET}")
        total = self.passed + self.failed
        print(f"Total: {total}")
        print(f"{Colors.GREEN}Passed: {self.passed}{Colors.RESET}")
        print(f"{Colors.RED}Failed: {self.failed}{Colors.RESET}")
        
        if self.failed == 0:
            print(f"\n{Colors.GREEN}{Colors.BOLD}✓ All stress tests passed!{Colors.RESET}")
            return 0
        else:
            print(f"\n{Colors.RED}{Colors.BOLD}✗ Some stress tests failed{Colors.RESET}")
            return 1


def main():
    parser = argparse.ArgumentParser(description="Stress test Thunder JsonRpcMuxer plugin")
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
    
    tester = StressTester(config)
    exit_code = tester.run_all_tests()
    
    exit(exit_code)


if __name__ == "__main__":
    main()
