#!/usr/bin/env python3
"""
Stress test for Thunder JsonRpcMuxer plugin with detailed timing metrics
Tests threadpool overload and concurrent batch handling
Useful for comparing atomic vs no-atomic implementations
"""

import json
import requests
import threading
import time
import argparse
import statistics
from typing import List, Dict, Any, Tuple
from dataclasses import dataclass, field
from collections import defaultdict


@dataclass
class TimingStats:
    """Statistics for timing measurements"""
    latencies: List[float] = field(default_factory=list)
    start_time: float = 0.0
    end_time: float = 0.0
    
    def add_latency(self, latency_ms: float):
        """Add a latency measurement"""
        self.latencies.append(latency_ms)
    
    def get_stats(self) -> Dict[str, float]:
        """Calculate statistics"""
        if not self.latencies:
            return {
                "count": 0,
                "min_ms": 0.0,
                "max_ms": 0.0,
                "mean_ms": 0.0,
                "median_ms": 0.0,
                "p90_ms": 0.0,
                "p95_ms": 0.0,
                "p99_ms": 0.0,
                "stdev_ms": 0.0,
                "total_time_s": 0.0,
                "throughput_rps": 0.0
            }
        
        sorted_latencies = sorted(self.latencies)
        count = len(sorted_latencies)
        
        total_time = self.end_time - self.start_time if self.end_time > self.start_time else 1.0
        
        return {
            "count": count,
            "min_ms": sorted_latencies[0],
            "max_ms": sorted_latencies[-1],
            "mean_ms": statistics.mean(sorted_latencies),
            "median_ms": statistics.median(sorted_latencies),
            "p90_ms": sorted_latencies[int(count * 0.90)] if count > 0 else 0.0,
            "p95_ms": sorted_latencies[int(count * 0.95)] if count > 0 else 0.0,
            "p99_ms": sorted_latencies[int(count * 0.99)] if count > 0 else 0.0,
            "stdev_ms": statistics.stdev(sorted_latencies) if count > 1 else 0.0,
            "total_time_s": total_time,
            "throughput_rps": count / total_time if total_time > 0 else 0.0
        }


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
    CYAN = '\033[96m'
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


def print_timing(label: str, value: float, unit: str = "ms"):
    """Print timing information"""
    print(f"{Colors.CYAN}⏱{Colors.RESET}  {label}: {value:.3f}{unit}")


def print_stats(stats: Dict[str, float], prefix: str = ""):
    """Print detailed statistics"""
    print(f"\n{Colors.BOLD}Performance Metrics:{Colors.RESET}")
    print_timing(f"{prefix}Count", stats["count"], " requests")
    print_timing(f"{prefix}Total Time", stats["total_time_s"], "s")
    print_timing(f"{prefix}Throughput", stats["throughput_rps"], " req/s")
    print(f"\n{Colors.BOLD}Latency Distribution:{Colors.RESET}")
    print_timing(f"{prefix}Min", stats["min_ms"])
    print_timing(f"{prefix}Mean", stats["mean_ms"])
    print_timing(f"{prefix}Median", stats["median_ms"])
    print_timing(f"{prefix}P90", stats["p90_ms"])
    print_timing(f"{prefix}P95", stats["p95_ms"])
    print_timing(f"{prefix}P99", stats["p99_ms"])
    print_timing(f"{prefix}Max", stats["max_ms"])
    print_timing(f"{prefix}StdDev", stats["stdev_ms"])


class StressTester:
    """Stress test harness for JsonRpcMuxer with detailed timing"""
    
    def __init__(self, config: TestConfig):
        self.config = config
        self.http_url = f"http://{config.host}:{config.port}/jsonrpc"
        self.passed = 0
        self.failed = 0
        self.all_timings: Dict[str, TimingStats] = {}
    
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
                    {"method": "Controller.1.version", "params": {}},
                    {"method": "Controller.1.buildinfo", "params": {}},
                    {"method": "Controller.1.environment@Controller", "params": {}},
                    {"method": "Controller.1.status", "params": {}},
                    {"method": "Controller.1.subsystems", "params": {}},
                    {"method": "Controller.1.links", "params": {}},
                    {"method": "Controller.1.configuration@Controller", "params": {}},
                ]
                method_data = methods[i % len(methods)]
            
            batch.append({
                "id": i + 1,
                "jsonrpc": "2.0",
                "method": method_data["method"],
                "params": method_data["params"]
            })
        return batch
    
    def test_maxbatches_limit(self, method: str = "sequential"):
        """Test that maxbatches limit is enforced with timing"""
        print_test(f"Maxbatches limit: {self.config.max_batches + 3} concurrent batches (mode: {method})")
        print_info(f"Using TimeSync.synchronize for slow requests (~100-500ms each)")
        print_info(f"Expecting ~{self.config.max_batches} to succeed, rest to be rejected")
        
        timing_stats = TimingStats()
        timing_stats.start_time = time.time()
        
        results = []
        errors = []
        accepted = 0
        rejected = 0
        lock = threading.Lock()
        
        def send_batch(batch_id: int):
            nonlocal accepted, rejected
            
            request_start = time.time()
            
            try:
                # Use slow requests (TimeSync) for this test
                batch = self.create_batch(self.config.max_batch_size, use_slow_requests=False)
                payload = {
                    "jsonrpc": "2.0",
                    "id": 400 + batch_id,
                    "method": "JsonRpcMuxer.1." + method,
                    "params": batch
                }
                
                response = requests.post(
                    self.http_url,
                    json=payload,
                    timeout=self.config.timeout / 1000
                )
                
                latency_ms = (time.time() - request_start) * 1000
                
                data = response.json()
                
                with lock:
                    timing_stats.add_latency(latency_ms)
                    
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
                latency_ms = (time.time() - request_start) * 1000
                with lock:
                    timing_stats.add_latency(latency_ms)
                    errors.append(f"Batch {batch_id}: Timeout after {latency_ms:.0f}ms")
            except Exception as e:
                latency_ms = (time.time() - request_start) * 1000
                with lock:
                    timing_stats.add_latency(latency_ms)
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
        
        timing_stats.end_time = time.time()
        
        print_info(f"Accepted: {accepted}, Rejected: {rejected}, Errors: {len(errors)}")
        
        if errors:
            for error in errors[:5]:
                print_info(f"  {error}")
            if len(errors) > 5:
                print_info(f"  ... and {len(errors) - 5} more errors")
        
        # Print timing statistics
        stats = timing_stats.get_stats()
        print_stats(stats)
        self.all_timings["maxbatches_limit"] = timing_stats
        
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
    
    def test_sustained_load(self, duration_seconds: int = 10, batches_per_second: int = 2, method: str = "sequential"):
        """Test sustained load over time with timing"""
        print_test(f"Sustained load: {batches_per_second} batches/sec for {duration_seconds}s (mode: {method})")
        
        timing_stats = TimingStats()
        timing_stats.start_time = time.time()
        
        total_sent = 0
        total_accepted = 0
        total_rejected = 0
        total_errors = 0
        lock = threading.Lock()
        
        start_time = time.time()
        interval = 1.0 / batches_per_second
        
        def send_batch():
            nonlocal total_sent, total_accepted, total_rejected, total_errors
            
            request_start = time.time()
            
            with lock:
                total_sent += 1
                batch_id = total_sent
            
            batch = self.create_batch(5)  # Smaller batches for sustained load
            payload = {
                "jsonrpc": "2.0",
                "id": 500 + batch_id,
                "method": "JsonRpcMuxer.1." + method,
                "params": batch
            }
            
            try:
                response = requests.post(
                    self.http_url,
                    json=payload,
                    timeout=self.config.timeout / 1000
                )
                
                latency_ms = (time.time() - request_start) * 1000
                
                data = response.json()
                
                with lock:
                    timing_stats.add_latency(latency_ms)
                    
                    if "error" in data:
                        if "Too many concurrent batches" in data["error"].get("message", ""):
                            total_rejected += 1
                        else:
                            total_errors += 1
                    elif "result" in data:
                        total_accepted += 1
                    
            except Exception:
                latency_ms = (time.time() - request_start) * 1000
                with lock:
                    timing_stats.add_latency(latency_ms)
                    total_errors += 1
        
        print_info(f"Sending batches...")
        
        threads = []
        while time.time() - start_time < duration_seconds:
            t = threading.Thread(target=send_batch)
            t.start()
            threads.append(t)
            time.sleep(interval)
        
        # Wait for all threads to complete
        for t in threads:
            t.join()
        
        timing_stats.end_time = time.time()
        elapsed = timing_stats.end_time - timing_stats.start_time
        
        print_info(f"Duration: {elapsed:.1f}s")
        print_info(f"Sent: {total_sent}, Accepted: {total_accepted}, Rejected: {total_rejected}, Errors: {total_errors}")
        print_info(f"Acceptance rate: {100 * total_accepted / total_sent:.1f}%")
        
        # Print timing statistics
        stats = timing_stats.get_stats()
        print_stats(stats)
        self.all_timings["sustained_load"] = timing_stats
        
        if total_errors > total_sent * 0.1:  # More than 10% errors is bad
            print_fail(f"Too many errors: {total_errors} ({100 * total_errors / total_sent:.1f}%)")
            self.failed += 1
        elif total_accepted == 0:
            print_fail("No batches were accepted")
            self.failed += 1
        else:
            print_pass(f"System handled sustained load successfully")
            self.passed += 1
    
    def test_rapid_fire(self, count: int = 20, method: str = "sequential"):
        """Test rapid-fire batch submission with timing"""
        print_test(f"Rapid fire: {count} batches as fast as possible (mode: {method})")
        
        timing_stats = TimingStats()
        timing_stats.start_time = time.time()
        
        results = []
        lock = threading.Lock()
        
        def send_batch(batch_id: int):
            request_start = time.time()
            
            try:
                batch = self.create_batch(3)
                payload = {
                    "jsonrpc": "2.0",
                    "id": 600 + batch_id,
                    "method": "JsonRpcMuxer.1." + method,
                    "params": batch
                }
                
                response = requests.post(
                    self.http_url,
                    json=payload,
                    timeout=self.config.timeout / 1000
                )
                
                latency_ms = (time.time() - request_start) * 1000
                
                with lock:
                    timing_stats.add_latency(latency_ms)
                    results.append(response.json())
                
            except Exception as e:
                latency_ms = (time.time() - request_start) * 1000
                with lock:
                    timing_stats.add_latency(latency_ms)
                    results.append({"error": str(e)})
        
        threads = []
        for i in range(count):
            t = threading.Thread(target=send_batch, args=(i,))
            threads.append(t)
            t.start()
        
        for t in threads:
            t.join()
        
        timing_stats.end_time = time.time()
        elapsed = timing_stats.end_time - timing_stats.start_time
        
        accepted = sum(1 for r in results if "result" in r)
        rejected = sum(1 for r in results if "error" in r and isinstance(r.get("error"), dict))
        errors = len(results) - accepted - rejected
        
        print_info(f"Completed in {elapsed:.3f}s")
        print_info(f"Accepted: {accepted}, Rejected: {rejected}, Errors: {errors}")
        
        # Print timing statistics
        stats = timing_stats.get_stats()
        print_stats(stats)
        self.all_timings["rapid_fire"] = timing_stats
        
        if errors > count * 0.2:  # More than 20% errors
            print_fail(f"Too many errors: {errors}")
            self.failed += 1
        else:
            print_pass(f"Rapid fire handled: {accepted}/{count} accepted")
            self.passed += 1
    
    def test_high_concurrency(self, num_concurrent: int = 50, batch_size: int = 5, method: str = "sequential"):
        """Test high concurrency stress with timing"""
        print_test(f"High concurrency: {num_concurrent} concurrent batches (mode: {method})")
        
        timing_stats = TimingStats()
        timing_stats.start_time = time.time()
        
        results = []
        lock = threading.Lock()
        
        def send_batch(batch_id: int):
            request_start = time.time()
            
            try:
                batch = self.create_batch(batch_size)
                payload = {
                    "jsonrpc": "2.0",
                    "id": 700 + batch_id,
                    "method": "JsonRpcMuxer.1." + method,
                    "params": batch
                }
                
                response = requests.post(
                    self.http_url,
                    json=payload,
                    timeout=self.config.timeout / 1000
                )
                
                latency_ms = (time.time() - request_start) * 1000
                
                with lock:
                    timing_stats.add_latency(latency_ms)
                    results.append(response.json())
                
            except Exception as e:
                latency_ms = (time.time() - request_start) * 1000
                with lock:
                    timing_stats.add_latency(latency_ms)
                    results.append({"error": str(e)})
        
        threads = []
        for i in range(num_concurrent):
            t = threading.Thread(target=send_batch, args=(i,))
            threads.append(t)
            t.start()
        
        for t in threads:
            t.join()
        
        timing_stats.end_time = time.time()
        
        accepted = sum(1 for r in results if "result" in r)
        rejected = sum(1 for r in results if "error" in r and isinstance(r.get("error"), dict))
        errors = len(results) - accepted - rejected
        
        print_info(f"Accepted: {accepted}, Rejected: {rejected}, Errors: {errors}")
        
        # Print timing statistics
        stats = timing_stats.get_stats()
        print_stats(stats)
        self.all_timings["high_concurrency"] = timing_stats
        
        if errors > num_concurrent * 0.1:  # More than 10% errors
            print_fail(f"Too many errors: {errors}")
            self.failed += 1
        elif accepted == 0:
            print_fail("No batches accepted")
            self.failed += 1
        else:
            print_pass(f"High concurrency handled: {accepted}/{num_concurrent} accepted")
            self.passed += 1
    
    def print_comparison_summary(self):
        """Print summary suitable for comparing implementations"""
        print(f"\n{Colors.BOLD}{'='*60}{Colors.RESET}")
        print(f"{Colors.BOLD}PERFORMANCE COMPARISON SUMMARY{Colors.RESET}")
        print(f"{Colors.BOLD}{'='*60}{Colors.RESET}")
        
        for test_name, timing_stats in self.all_timings.items():
            stats = timing_stats.get_stats()
            print(f"\n{Colors.BOLD}{test_name.upper().replace('_', ' ')}:{Colors.RESET}")
            print(f"  Throughput:  {stats['throughput_rps']:8.2f} req/s")
            print(f"  Mean Lat:    {stats['mean_ms']:8.3f} ms")
            print(f"  Median Lat:  {stats['median_ms']:8.3f} ms")
            print(f"  P95 Lat:     {stats['p95_ms']:8.3f} ms")
            print(f"  P99 Lat:     {stats['p99_ms']:8.3f} ms")
            print(f"  Max Lat:     {stats['max_ms']:8.3f} ms")
        
        print(f"\n{Colors.BOLD}{'='*60}{Colors.RESET}")
        print(f"{Colors.CYAN}Copy these values to compare implementations:{Colors.RESET}")
        print(f"Implementation: [atomic/no-atomic]")
        for test_name, timing_stats in self.all_timings.items():
            stats = timing_stats.get_stats()
            print(f"  {test_name}: throughput={stats['throughput_rps']:.2f} mean={stats['mean_ms']:.3f} p95={stats['p95_ms']:.3f}")
    
    def run_all_tests(self, method: str = "sequential"):
        """Run all stress tests"""
        print(f"\n{Colors.BOLD}JsonRpcMuxer Stress Test Suite (with Timing){Colors.RESET}")
        print(f"Target: {self.config.host}:{self.config.port}")
        print(f"Config: maxbatchsize={self.config.max_batch_size}, maxbatches={self.config.max_batches}")
        print(f"Mode: {method}")
        
        self.test_rapid_fire(count=10, method=method)
        self.test_high_concurrency(num_concurrent=10, batch_size=5, method=method)
        self.test_sustained_load(duration_seconds=5, batches_per_second=3, method=method)
        self.test_maxbatches_limit(method=method)
        
        print(f"\n{Colors.BOLD}=== Test Results Summary ==={Colors.RESET}")
        total = self.passed + self.failed
        print(f"Total: {total}")
        print(f"{Colors.GREEN}Passed: {self.passed}{Colors.RESET}")
        print(f"{Colors.RED}Failed: {self.failed}{Colors.RESET}")
        
        self.print_comparison_summary()
        
        if self.failed == 0:
            print(f"\n{Colors.GREEN}{Colors.BOLD}✓ All stress tests passed!{Colors.RESET}")
            return 0
        else:
            print(f"\n{Colors.RED}{Colors.BOLD}✗ Some stress tests failed{Colors.RESET}")
            return 1


def main():
    parser = argparse.ArgumentParser(description="Stress test Thunder JsonRpcMuxer plugin with detailed timing")
    parser.add_argument("--host", default="localhost", help="Thunder host (default: localhost)")
    parser.add_argument("--port", type=int, default=55555, help="Thunder port (default: 55555)")
    parser.add_argument("--timeout", type=int, default=5000, help="Timeout in ms (default: 5000)")
    parser.add_argument("--max-batch-size", type=int, default=10, help="Expected max batch size (default: 10)")
    parser.add_argument("--max-batches", type=int, default=5, help="Expected max concurrent batches (default: 5)")
    parser.add_argument("--mode", choices=["parallel", "sequential"], default="sequential", help="Batch execution mode (default: sequential)")
    
    args = parser.parse_args()
    
    config = TestConfig(
        host=args.host,
        port=args.port,
        timeout=args.timeout,
        max_batch_size=args.max_batch_size,
        max_batches=args.max_batches
    )
    
    tester = StressTester(config)
    exit_code = tester.run_all_tests(method=args.mode)
    
    exit(exit_code)


if __name__ == "__main__":
    main()