#!/usr/bin/env python3
"""Minimal PPO training stub for traffic-light control.

This script trains against a lightweight synthetic environment so you can produce
an initial model file consumed by traffic_rl_server.py. Replace this env with a
socket/file bridge env for full online training against the C++ simulator.
"""

import argparse
import numpy as np

import gymnasium as gym
from gymnasium import spaces
from stable_baselines3 import PPO


class SimpleTrafficEnv(gym.Env):
    metadata = {"render_modes": []}

    def __init__(self):
        super().__init__()
        self.observation_space = spaces.Box(low=0, high=200, shape=(4,), dtype=np.float32)
        self.action_space = spaces.Discrete(4)
        self.state = np.zeros(4, dtype=np.float32)
        self.phase = 0
        self.green_duration = 12.0

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        self.state = np.random.randint(0, 20, size=4).astype(np.float32)
        self.phase = 0
        self.green_duration = 12.0
        return self.state.copy(), {}

    def step(self, action):
        if action == 1:
            self.phase = 1 - self.phase
        elif action == 2:
            self.green_duration = min(45.0, self.green_duration + 1.0)
        elif action == 3:
            self.green_duration = max(4.0, self.green_duration - 1.0)

        served = 2.5 + 0.2 * (self.green_duration / 10.0)
        if self.phase == 0:
            self.state[0] = max(0.0, self.state[0] - served)
            self.state[1] = max(0.0, self.state[1] - served)
            self.state[2] += np.random.uniform(0.0, 2.0)
            self.state[3] += np.random.uniform(0.0, 2.0)
        else:
            self.state[2] = max(0.0, self.state[2] - served)
            self.state[3] = max(0.0, self.state[3] - served)
            self.state[0] += np.random.uniform(0.0, 2.0)
            self.state[1] += np.random.uniform(0.0, 2.0)

        reward = -float(np.sum(self.state))
        terminated = False
        truncated = False
        return self.state.copy(), reward, terminated, truncated, {}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--steps", type=int, default=200000)
    parser.add_argument("--out", default="rl/ppo_traffic")
    args = parser.parse_args()

    env = SimpleTrafficEnv()
    model = PPO("MlpPolicy", env, verbose=1)
    model.learn(total_timesteps=args.steps)
    model.save(args.out)
    print(f"Saved model to {args.out}.zip")
