#!/usr/bin/env python3
import argparse
import signal
import subprocess
import sys
import time
from pathlib import Path
import paho.mqtt.client as mqtt

def parse_args():
    ap = argparse.ArgumentParser(description="Run WISENET fake device simulators")
    ap.add_argument("--broker", default="wise-net.io")
    ap.add_argument("--port", type=int, default=1883)
    ap.add_argument("--devices", default="sim-device-001,esp32-test-01")
    ap.add_argument("--script", default=str(Path(__file__).resolve().parents[1] / "demos/stream-control-2025-10/simulators/fake_device.py"))
    return ap.parse_args()


def main():
    args = parse_args()
    devices = [d.strip() for d in args.devices.split(",") if d.strip()]
    if not devices:
        print("No devices specified", file=sys.stderr)
        return 1

    procs: dict[str, subprocess.Popen] = {}
    stopping = False

    def publish_stop_all():
        try:
            client = mqtt.Client(client_id="simulator-controller", clean_session=True)
            client.connect(args.broker, args.port, keepalive=10)
            for device_id in devices:
                topic = f"devices/{device_id}/control"
                client.publish(topic, '{"cmd":"STOP"}', qos=1)
                print(f"Sent STOP to {device_id}")
            client.disconnect()
        except Exception as e:
            print(f"Failed to publish STOP: {e}", file=sys.stderr)

    def start_device(device_id: str):
        cmd = [
            sys.executable,
            args.script,
            "--id",
            device_id,
            "--broker",
            args.broker,
            "--port",
            str(args.port),
        ]
        procs[device_id] = subprocess.Popen(cmd)
        print(f"Started simulator for {device_id} (pid={procs[device_id].pid})")

    def shutdown(signum=None, frame=None):
        nonlocal stopping
        stopping = True
        for device_id, p in list(procs.items()):
            if p.poll() is None:
                print(f"Stopping simulator for {device_id} (pid={p.pid})")
                p.terminate()
        # give them a moment to exit
        time.sleep(1)
        for device_id, p in list(procs.items()):
            if p.poll() is None:
                p.kill()
        return 0

    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)

    for d in devices:
        start_device(d)

    # Ensure devices are not streaming until explicitly started
    publish_stop_all()

    try:
        while not stopping:
            for device_id, p in list(procs.items()):
                if p.poll() is not None:
                    print(f"Simulator for {device_id} exited; restarting")
                    start_device(device_id)
                    publish_stop_all()
            time.sleep(2)
    finally:
        shutdown()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
