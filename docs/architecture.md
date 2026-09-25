# LinuxGuard Architecture

## Overview

LinuxGuard is a lightweight process/service supervisor for Linux systems. It manages configured processes, monitors their health and resource usage, automatically restarts failed processes, and provides a CLI for control.

## Architecture Diagram

```
─────────────────────────────────────────────────────────────┐
│                        LinuxGuard Daemon                     │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │   Config     │  │  Dependency  │  │     Process      │  │
│  │   Parser     │──│    Graph     │──│    Manager       │  │
│  └──────────────┘  └──────────────┘  ──────────────────┘  │
│         │                   │                      │         │
│         ▼                   ▼                      ▼         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │   Logger     │  │  Resource    │  │  Restart Logic   │  │
│  │              │  │  Monitor     │  │                  │  │
│  └──────────────  └──────────────┘  └──────────────────┘  │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              Unix Domain Socket IPC                   │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
                    ┌──────────────┐
                    │     CLI      │
                    │  (Client)    │
                    └──────────────┘
```

## Components

### 1. Daemon (`src/daemon/`)
- Background service with graceful shutdown
- Signal handling (SIGTERM, SIGINT)
- PID file management
- Coordinates all other components

### 2. Process Manager (`src/process/`)
- Process lifecycle management using fork/exec/waitpid/kill
- PID tracking and state management
- States: STOPPED, STARTING, RUNNING, STOPPING, FAILED, BACKOFF
- Automatic restart with configurable backoff

### 3. Configuration Parser (`src/config/`)
- Human-readable INI-style configuration
- Service definitions with dependencies
- Resource limits (CPU, RAM)
- Restart policies

### 4. Resource Monitor (`src/monitor/`)
- Reads /proc filesystem for process stats
- CPU usage estimation
- RSS and virtual memory tracking
- Thread count monitoring

### 5. Dependency Graph (`src/dependency/`)
- Topological sort for startup order
- Cycle detection
- Reverse order for shutdown

### 6. IPC (`src/ipc/`)
- Unix domain socket server/client
- Command-response protocol
- Thread-safe communication

### 7. Logger (`src/logging/`)
- Thread-safe timestamped logging
- File or stderr output
- Configurable log levels

### 8. CLI (`src/cli/`)
- Command-line interface
- Connects to daemon via IPC
- Commands: status, list, start, stop, restart, reload, shutdown

## Data Flow

1. **Startup**: Daemon parses config → builds dependency graph → starts services in topological order
2. **Monitoring**: Monitor thread checks process health every N seconds
3. **Failure Detection**: Process manager detects exited processes via waitpid(WNOHANG)
4. **Restart**: Restart thread attempts to restart failed processes with exponential backoff
5. **CLI Commands**: Client sends command via Unix socket → daemon processes → returns response

## Thread Model

- **Main thread**: Runs daemon event loop, handles signals
- **IPC accept thread**: Accepts client connections and processes commands
- **Monitor thread**: Periodically checks process health and resource usage
- **Restart thread**: Attempts to restart failed processes

All shared state is protected by mutexes.

## Configuration Format

```ini
[global]
socket_path = /tmp/linuxguard.sock
pid_file = /tmp/linuxguard.pid
log_file = /tmp/linuxguard.log
log_level = info
monitor_interval_sec = 5

[service myservice]
name = myservice
command = /usr/bin/myapp --flag
working_dir = /opt/myapp
user = appuser
auto_restart = true
max_restarts = 5
restart_backoff_sec = 3
restart_backoff_max_sec = 60
cpu_limit = 80.0
ram_limit_mb = 512
depends_on = dependency1, dependency2
```

## Build System

CMake with C++17, compiled with `-Wall -Wextra -Wpedantic`.

## Dependencies

- Linux/POSIX APIs only
- STL threads/mutex
- No external libraries
