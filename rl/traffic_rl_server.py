#!/usr/bin/env python3
"""TCP server bridge: receives C++ traffic state and returns RL action.

Protocol (single line, space-separated):
carsN carsS carsE carsW phase greenDuration reward

Response:
action\n
Actions:
0 keep current phase
1 switch NS <-> EW
2 increase green duration
3 decrease green duration
"""

import argparse
import socket
from pathlib import Path

import numpy as np

try:
    from stable_baselines3 import PPO
except Exception:  # pragma: no cover
    PPO = None


class HeuristicPolicy:
    def predict(self, obs, deterministic=True):
        n, s, e, w = obs
        ns = n + s
        ew = e + w
        if abs(ns - ew) > 3:
            if ew > ns:
                return 1, None
        if max(ns, ew) > 14:
            return 2, None
        if max(ns, ew) < 4:
            return 3, None
        return 0, None


def load_policy(model_path: str):
    if PPO is None:
        print("stable-baselines3 not available, using heuristic policy")
        return HeuristicPolicy()

    p = Path(model_path)
    if p.exists():
        print(f"Loading PPO model from {p}")
        return PPO.load(str(p))

    print("PPO model not found, using heuristic policy")
    return HeuristicPolicy()


def parse_state(line: str):
    parts = line.strip().split()
    if len(parts) < 7:
        raise ValueError("Invalid input line")
    n, s, e, w = map(float, parts[:4])
    return np.array([n, s, e, w], dtype=np.float32)


def clamp_action(a: int) -> int:
    return max(0, min(3, int(a)))


def serve(host: str, port: int, model_path: str):
    policy = load_policy(model_path)

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(8)
    print(f"RL server listening on {host}:{port}")

    while True:
        conn, addr = server.accept()
        print(f"Client connected: {addr}")
        with conn:
            data_buffer = ""
            while True:
                chunk = conn.recv(4096)
                if not chunk:
                    break
                data_buffer += chunk.decode("utf-8", errors="ignore")

                while "\n" in data_buffer:
                    line, data_buffer = data_buffer.split("\n", 1)
                    if not line.strip():
                        continue
                    try:
                        obs = parse_state(line)
                        action, _ = policy.predict(obs, deterministic=True)
                        action = clamp_action(action)
                    except Exception:
                        action = 0

                    conn.sendall(f"{action}\n".encode("utf-8"))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=5555)
    parser.add_argument("--model", default="rl/ppo_traffic.zip")
    args = parser.parse_args()
    serve(args.host, args.port, args.model)
