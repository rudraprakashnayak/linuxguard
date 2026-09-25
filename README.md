# LinuxGuard

A lightweight Linux process/service supervisor written in C++17.

## Features

- **Process Management**: Start, stop, and monitor Linux processes
- **Automatic Restart**: Configurable restart with exponential backoff
- **Dependency Graph**: Topological startup order with cycle detection
- **Resource Monitoring**: CPU/RAM monitoring via /proc filesystem
- **Unix Socket IPC**: Thread-safe command-response protocol
- **CLI Control**: Simple command-line interface
- **systemd Integration**: Ready-to-use service file
- **Zero External Dependencies**: Only Linux/POSIX APIs and STL

## Architecture

```
linuxguard/
├── src/
│   ├── main.cpp                    # Entry point
│   ├── daemon/                     # Background service
│   ├── process/                    # Process lifecycle management
│   ├── config/                     # Configuration parser
│   ├── monitor/                    # Resource monitoring
│   ├── dependency/                 # Dependency graph
│   ├── ipc/                        # Unix domain socket IPC
│   ├── logging/                    # Thread-safe logger
│   └── cli/                        # CLI client
├── tests/                          # Unit tests
├── configs/                        # Sample configurations
└── docs/                           # Documentation
```

See [docs/architecture.md](docs/architecture.md) for detailed design.

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

### Start the daemon

```bash
./linuxguard --daemon /path/to/config.service
```

### CLI commands

```bash
# List all services
./linuxguard list

# Show service status
./linuxguard status <service_name>

# Start a service
./linuxguard start <service_name>

# Stop a service
./linuxguard stop <service_name>

# Restart a service
./linuxguard restart <service_name>

# Shutdown the daemon
./linuxguard shutdown
```

### Override socket path

```bash
./linuxguard --socket /custom/path.sock list
```

## Configuration

Example configuration (`configs/demo.service`):

```ini
[global]
socket_path = /tmp/linuxguard.sock
pid_file = /tmp/linuxguard.pid
log_file = /tmp/linuxguard.log
log_level = info
monitor_interval_sec = 5

[service sleeper]
name = sleeper
command = sleep 3600
working_dir = /
auto_restart = true
max_restarts = 3
restart_backoff_sec = 2
restart_backoff_max_sec = 30

[service counter]
name = counter
command = sh -c "while true; do echo tick; sleep 1; done"
working_dir = /tmp
auto_restart = true
max_restarts = 5
restart_backoff_sec = 3
restart_backoff_max_sec = 60
depends_on = sleeper
```

### Configuration Options

**Global:**
- `socket_path`: Unix socket path for IPC (default: /tmp/linuxguard.sock)
- `pid_file`: PID file path (default: /tmp/linuxguard.pid)
- `log_file`: Log file path (optional, defaults to stderr)
- `log_level`: debug, info, warn, error (default: info)
- `monitor_interval_sec`: Health check interval in seconds (default: 5)

**Service:**
- `name`: Service name (required)
- `command`: Command to execute (required)
- `working_dir`: Working directory (default: /)
- `user`: Run as user (optional)
- `auto_restart`: Enable automatic restart (default: true)
- `max_restarts`: Maximum restart attempts (default: 5)
- `restart_backoff_sec`: Initial backoff in seconds (default: 3)
- `restart_backoff_max_sec`: Maximum backoff in seconds (default: 60)
- `cpu_limit`: CPU usage limit in seconds (0 = unlimited)
- `ram_limit_mb`: RAM limit in MB (0 = unlimited)
- `depends_on`: Comma-separated dependency list

## systemd Integration

Install the service file:

```bash
sudo cp configs/linuxguard.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable linuxguard
sudo systemctl start linuxguard
```

## Testing

```bash
cd build
ctest
```

Or run tests directly:

```bash
./linuxguard_tests
```

## Demo

1. Start the daemon with demo config:
   ```bash
   ./linuxguard --daemon ../configs/demo.service
   ```

2. In another terminal, list services:
   ```bash
   ./linuxguard list
   ```

3. Check status:
   ```bash
   ./linuxguard status sleeper
   ```

4. Kill a process externally:
   ```bash
   pkill -f "sleep 3600"
   ```

5. Verify automatic restart:
   ```bash
   ./linuxguard status sleeper
   ```

6. Stop a service:
   ```bash
   ./linuxguard stop counter
   ```

7. Shutdown daemon:
   ```bash
   ./linuxguard shutdown
   ```

## Requirements

- Linux (tested on Ubuntu 20.04+)
- C++17 compiler (GCC 7+ or Clang 5+)
- CMake 3.14+

## License

MIT
