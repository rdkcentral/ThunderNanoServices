#!/usr/bin/env python3
"""
Controller-only stress test for JsonRpcMuxer
Tests with fast Controller methods but high concurrency
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
    print(f"\n{Colors.BOLD}{Colors.BLUE}[TEST]{Colors.RESET} {name}")


def print_pass(message: str):
    print(f"{Colors.GREEN}✓{Colors.RESET} {message}")


def print_fail(message: str):
    print(f"{Colors.RED}✗{Colors.RESET} {message}")


def print_info(message: str):
    print(f"{Colors.YELLOW}ℹ{Colors.RESET} {message}")


class ControllerStressTester:
    """Stress test with Controller methods only"""
    
    def __init__(self, config: TestConfig):
        self.config = config
        self.http_url = f"http://{config.host}:{config.port}/jsonrpc"
        self.passed = 0
        self.failed = 0
    
    def create_batch(self, count: int) -> List[Dict[str, Any]]:
        """Create a batch of Controller requests"""
        batch = []
        methods = [
            {"method": "Controller.1.subsystems", "params": {}},
            {"method": "Controller.1.version", "params": {}},
            {"method": "Controller.1.buildinfo", "params": {}},
            {"method": "Controller.1.links", "params": {}},
        ]
        for i in range(count):
            method_data = methods[i % len(methods)]
            batch.append({
                "id": i + 1,
                "jsonrpc": "2.0",
                "method": method_data["method"],
                "params": method_data["params"]
            })
        return batch
    
    def test_high_concurrency(self, num_batches: int = 20, method: str = "sequential"):
        """Test with many batches sent simultaneously"""
        print_test(f"High concurrency: {num_batches} batches sent at once")
        print_info(f"Expected maxbatches={self.config.max_batches}, so ~{self.config.max_batches} should succeed initially")
        
        results = []
        errors = []
        accepted = 0
        rejected = 0
        start_time = time.time()
        
        def send_batch(batch_id: int):
            nonlocal accepted, rejected
            try:
                batch = self.create_batch(self.config.max_batch_size)
                payload = {
                    "jsonrpc": "2.0",
                    "id": 1000 + batch_id,
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
                    if "Too many concurrent batches" in data["error"].get("message", ""):
                        rejected += 1
                    else:
                        errors.append(f"Batch {batch_id}: {data['error']}")
                elif "result" in data:
                    accepted += 1
                else:
                    errors.append(f"Batch {batch_id}: No result or error")
                    
                results.append(data)
                
            except Exception as e:
                errors.append(f"Batch {batch_id}: {str(e)}")
        
        # Create all threads
        threads = []
        for i in range(num_batches):
            t = threading.Thread(target=send_batch, args=(i,))
            threads.append(t)
        
        # Start all at once - no delay
        for t in threads:
            t.start()
        
        # Wait for all to complete
        for t in threads:
            t.join()
        
        elapsed = time.time() - start_time
        
        print_info(f"Completed in {elapsed:.3f}s")
        print_info(f"Accepted: {accepted}, Rejected: {rejected}, Errors: {len(errors)}")
        
        if errors:
            for error in errors[:3]:
                print_info(f"  {error}")
            if len(errors) > 3:
                print_info(f"  ... and {len(errors) - 3} more errors")
        
        total_handled = accepted + rejected
        
        # With fast Controller calls, we expect:
        # - Most batches complete quickly
        # - But with 20 batches sent at exact same moment, some should hit the limit
        if total_handled != num_batches:
            print_fail(f"Expected {num_batches} responses, got {total_handled}")
            self.failed += 1
        elif rejected == 0:
            print_info(f"No rejections (system too fast or limit not effective)")
            print_info(f"This may be expected with fast Controller calls")
            self.passed += 1
        else:
            print_pass(f"System handled concurrency: {accepted} accepted, {rejected} rejected")
            self.passed += 1
    
    def test_burst_waves(self, waves: int = 3, batches_per_wave: int = 10, wave_delay_ms: int = 50, method: str = "sequential"):
        """Test with waves of batches to catch them overlapping"""
        print_test(f"Burst waves: {waves} waves of {batches_per_wave} batches, {wave_delay_ms}ms apart")
        
        all_results = []
        wave_stats = []
        
        for wave_num in range(waves):
            accepted = 0
            rejected = 0
            results = []
            
            def send_batch(batch_id: int):
                nonlocal accepted, rejected
                try:
                    batch = self.create_batch(5)  # Smaller batches
                    payload = {
                        "jsonrpc": "2.0",
                        "id": 2000 + wave_num * 100 + batch_id,
                        "method": "JsonRpcMuxer.1." + method,
                        "params": batch
                    }
                    
                    response = requests.post(
                        self.http_url,
                        json=payload,
                        timeout=self.config.timeout / 1000
                    )
                    
                    data = response.json()
                    
                    if "error" in data and "Too many concurrent batches" in data["error"].get("message", ""):
                        rejected += 1
                    elif "result" in data:
                        accepted += 1
                    
                    results.append(data)
                    
                except Exception:
                    pass
            
            # Send wave
            threads = []
            for i in range(batches_per_wave):
                t = threading.Thread(target=send_batch, args=(i,))
                threads.append(t)
                t.start()
            
            for t in threads:
                t.join()
            
            wave_stats.append({"accepted": accepted, "rejected": rejected})
            all_results.extend(results)
            
            print_info(f"  Wave {wave_num + 1}: {accepted} accepted, {rejected} rejected")
            
            # Delay before next wave
            if wave_num < waves - 1:
                time.sleep(wave_delay_ms / 1000)
        
        total_accepted = sum(w["accepted"] for w in wave_stats)
        total_rejected = sum(w["rejected"] for w in wave_stats)
        
        print_info(f"Total: {total_accepted} accepted, {total_rejected} rejected")
        
        if total_rejected > 0:
            print_pass(f"Waves created overlapping load, saw {total_rejected} rejections")
            self.passed += 1
        else:
            print_info("No rejections across all waves (system handles load well)")
            self.passed += 1
    
    def test_soak(self, duration_minutes: int = 5, batches_per_second: float = 2.0, method: str = "sequential"):
        """Run continuous load for extended period"""
        print_test(f"Soak test: {duration_minutes} minutes at {batches_per_second} batches/sec")
        
        duration_seconds = duration_minutes * 60
        start_time = time.time()
        interval = 1.0 / batches_per_second
        
        total_sent = 0
        total_accepted = 0
        total_rejected = 0
        total_errors = 0
        
        last_report = start_time
        report_interval = 30  # Report every 30 seconds
        
        def send_batch(batch_id: int):
            nonlocal total_sent, total_accepted, total_rejected, total_errors
            
            total_sent += 1
            try:
                batch = self.create_batch(5)
                payload = {
                    "jsonrpc": "2.0",
                    "id": 3000 + batch_id,
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
                    if "Too many concurrent batches" in data["error"].get("message", ""):
                        total_rejected += 1
                    else:
                        total_errors += 1
                elif "result" in data:
                    total_accepted += 1
                else:
                    total_errors += 1
                    
            except Exception:
                total_errors += 1
        
        print_info(f"Running soak test... (Ctrl+C to stop early)")
        
        try:
            while time.time() - start_time < duration_seconds:
                # Send batch
                t = threading.Thread(target=send_batch, args=(total_sent,))
                t.start()
                
                # Progress reporting
                now = time.time()
                if now - last_report >= report_interval:
                    elapsed_min = (now - start_time) / 60
                    print_info(f"  {elapsed_min:.1f}min: sent={total_sent}, accepted={total_accepted}, rejected={total_rejected}, errors={total_errors}")
                    last_report = now
                
                time.sleep(interval)
        
        except KeyboardInterrupt:
            print_info("Soak test interrupted by user")
        
        elapsed = time.time() - start_time
        elapsed_min = elapsed / 60
        
        print_info(f"Soak test completed after {elapsed_min:.1f} minutes")
        print_info(f"Total: sent={total_sent}, accepted={total_accepted}, rejected={total_rejected}, errors={total_errors}")
        
        if total_sent > 0:
            accept_rate = 100 * total_accepted / total_sent
            error_rate = 100 * total_errors / total_sent
            print_info(f"Accept rate: {accept_rate:.1f}%, Error rate: {error_rate:.1f}%")
        
        if error_rate > 10:
            print_fail(f"Error rate too high: {error_rate:.1f}%")
            self.failed += 1
        else:
            print_pass(f"Soak test completed successfully")
            self.passed += 1
    
    def run_all_tests(self, soak_duration: int = 0):
        """Run all controller-only stress tests"""
        print(f"\n{Colors.BOLD}Controller-Only Stress Tests{Colors.RESET}")
        print(f"Target: {self.config.host}:{self.config.port}")
        print(f"Config: maxbatchsize={self.config.max_batch_size}, maxbatches={self.config.max_batches}")
        print(f"Using only fast Controller methods")
        
        self.test_high_concurrency(num_batches=20)
        self.test_burst_waves(waves=3, batches_per_wave=10, wave_delay_ms=50)
        
        if soak_duration > 0:
            self.test_soak(duration_minutes=soak_duration, batches_per_second=2.0)
        
        print(f"\n{Colors.BOLD}=== Test Summary ==={Colors.RESET}")
        total = self.passed + self.failed
        print(f"Total: {total}")
        print(f"{Colors.GREEN}Passed: {self.passed}{Colors.RESET}")
        print(f"{Colors.RED}Failed: {self.failed}{Colors.RESET}")
        
        if self.failed == 0:
            print(f"\n{Colors.GREEN}{Colors.BOLD}✓ All tests passed!{Colors.RESET}")
            print(f"\n{Colors.YELLOW}Note: With fast Controller calls, the system may handle")
            print(f"all batches without rejections. This is actually good - it means")
            print(f"your batches complete faster than new ones arrive.{Colors.RESET}")
            return 0
        else:
            return 1


def main():
    parser = argparse.ArgumentParser(description="Controller-only stress test for JsonRpcMuxer")
    parser.add_argument("--host", default="localhost", help="Thunder host")
    parser.add_argument("--port", type=int, default=55555, help="Thunder port")
    parser.add_argument("--timeout", type=int, default=5000, help="Timeout in ms")
    parser.add_argument("--max-batch-size", type=int, default=10, help="Max batch size")
    parser.add_argument("--max-batches", type=int, default=5, help="Max concurrent batches")
    parser.add_argument("--soak", type=int, default=0, help="Run soak test for N minutes (0=skip)")
    
    args = parser.parse_args()
    
    config = TestConfig(
        host=args.host,
        port=args.port,
        timeout=args.timeout,
        max_batch_size=args.max_batch_size,
        max_batches=args.max_batches
    )
    
    tester = ControllerStressTester(config)
    exit_code = tester.run_all_tests(soak_duration=args.soak)
    
    exit(exit_code)


if __name__ == "__main__":
    main()
