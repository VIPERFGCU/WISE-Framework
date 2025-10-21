#!/usr/bin/env python3
import argparse, json, random, time, threading, sys
from datetime import datetime, timezone
import paho.mqtt.client as mqtt

def now_iso(): return datetime.now(timezone.utc).isoformat()

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--id", required=True)
    ap.add_argument("--broker", default="localhost")
    ap.add_argument("--port", type=int, default=1883)
    args = ap.parse_args()

    device_id = args.id
    control_t = f"devices/{device_id}/control"
    status_t  = f"devices/{device_id}/status"
    data_t    = f"devices/{device_id}/data"
    hb_t      = f"devices/{device_id}/heartbeat"

    state = {"streaming": False, "rate_hz": 10}
    client = mqtt.Client(client_id=f"sim-{device_id}", clean_session=True)
    client.will_set(status_t, json.dumps({"state":"offline","ts":now_iso()}), qos=1, retain=True)

    def on_connect(c, u, flags, rc):
        print("Connected:", rc)
        c.subscribe(control_t, qos=1)
        c.publish(status_t, json.dumps({"state":"stopped","rate_hz":state["rate_hz"],"ts":now_iso()}), qos=1, retain=True)

    def on_message(c, u, msg):
        try: payload = json.loads(msg.payload.decode())
        except Exception as e:
            print("Bad control payload:", e, file=sys.stderr); return
        cmd = payload.get("cmd"); rate = int(payload.get("rate_hz", state["rate_hz"]))
        if cmd == "START":
            state.update(streaming=True, rate_hz=rate)
            c.publish(status_t, json.dumps({"state":"streaming","rate_hz":rate,"ts":now_iso()}), qos=1, retain=True)
            print(f"START {rate} Hz")
        elif cmd == "STOP":
            state["streaming"] = False
            c.publish(status_t, json.dumps({"state":"stopped","rate_hz":state["rate_hz"],"ts":now_iso()}), qos=1, retain=True)
            print("STOP")

    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(args.broker, args.port, keepalive=30)

    def heartbeat():
        while True:
            client.publish(hb_t, json.dumps({"rssi":-50,"uptime_s":int(time.time()),"fw":"sim-1.0","ts":now_iso()}), qos=0)
            time.sleep(15)

    def stream():
        while True:
            if state["streaming"]:
                x,y,z = [random.uniform(-1,1) for _ in range(3)]
                client.publish(data_t, json.dumps({"ts":now_iso(),"x":x,"y":y,"z":z}), qos=1)
                time.sleep(max(0.001, 1/float(state["rate_hz"])))
            else:
                time.sleep(0.1)

    threading.Thread(target=heartbeat, daemon=True).start()
    threading.Thread(target=stream, daemon=True).start()
    client.loop_forever()

if __name__ == "__main__":
    main()

